#include "Papyrus.h"

#include "GrassHider.h"
#include "Placement.h"

// Native functions for LZC_Native.psc. They are registered as not callable
// from tasklets, so the VM runs them in sync with the main thread, which is
// required for the Havok raycasts and for creating references.
//
// Camp layouts arrive as offsets from an origin (the player): `fwd` units in
// front along `heading` and `side` units to the right, with a `turn` added to
// the heading for each object's facing. Angles are in degrees.
namespace Papyrus
{
	namespace
	{
		constexpr auto         kClass = "LZC_Native"sv;
		constexpr std::int32_t kVersion = 4;

		// How far above/below the origin to look for ground.
		constexpr float kSearchUp = 256.0f;
		constexpr float kSearchDown = 1024.0f;

		// Result codes for CheckCampSite (mirrored in LZC_CampsiteQuestScript).
		enum AreaResult : std::int32_t
		{
			kClear = 0,
			kNoGround = 1,
			kTooSteep = 2,
			kUneven = 3,
			kUnderwater = 4,
			kBadArguments = 5
		};

		constexpr float ToRadians(float a_deg) { return a_deg * std::numbers::pi_v<float> / 180.0f; }

		RE::TESObjectCELL* CellAt(RE::TESObjectREFR* a_origin, float a_x, float a_y)
		{
			if (auto* tes = RE::TES::GetSingleton()) {
				if (auto* cell = tes->GetCell({ a_x, a_y, a_origin->GetPositionZ() })) {
					return cell;
				}
			}
			return a_origin->GetParentCell();
		}

		std::optional<Placement::GroundHit> GroundAt(RE::TESObjectREFR* a_origin, float a_x, float a_y)
		{
			const float top = a_origin->GetPositionZ() + kSearchUp;
			return Placement::CastDown(CellAt(a_origin, a_x, a_y), a_x, a_y, top, kSearchUp + kSearchDown);
		}

		// Skyrim heading: 0 = north (+Y), increasing clockwise.
		// forward = (sin h, cos h), right = (cos h, -sin h)
		RE::NiPoint3 LocalToWorld(RE::TESObjectREFR* a_origin, float a_heading, float a_fwd, float a_side)
		{
			const float sinH = std::sin(a_heading);
			const float cosH = std::cos(a_heading);
			return {
				a_origin->GetPositionX() + a_fwd * sinH + a_side * cosH,
				a_origin->GetPositionY() + a_fwd * cosH - a_side * sinH,
				a_origin->GetPositionZ()
			};
		}

		// Samples the centre and four corners of a rectangular footprint
		// (half-sizes along its right and forward axes).
		AreaResult CheckFootprint(RE::TESObjectREFR* a_origin, const RE::NiPoint3& a_centre, float a_yaw,
			float a_halfWidth, float a_halfDepth, float a_maxStep, float a_maxSlopeDegrees)
		{
			const float sinA = std::sin(a_yaw);
			const float cosA = std::cos(a_yaw);

			constexpr std::array<std::pair<float, float>, 5> samples{ { { 0.0f, 0.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f } } };

			float lowest = std::numeric_limits<float>::max();
			float highest = std::numeric_limits<float>::lowest();

			for (std::size_t i = 0; i < samples.size(); ++i) {
				const float r = samples[i].first * a_halfWidth;
				const float f = samples[i].second * a_halfDepth;
				const float x = a_centre.x + f * sinA + r * cosA;
				const float y = a_centre.y + f * cosA - r * sinA;

				const auto hit = GroundAt(a_origin, x, y);
				if (!hit) {
					return kNoGround;
				}
				if (Placement::IsUnderwater(CellAt(a_origin, x, y), hit->point)) {
					return kUnderwater;
				}
				if (i == 0 && Placement::SlopeDegrees(hit->normal) > a_maxSlopeDegrees) {
					return kTooSteep;
				}

				lowest = std::min(lowest, hit->point.z);
				highest = std::max(highest, hit->point.z);
			}

			return (highest - lowest) > a_maxStep ? kUneven : kClear;
		}

		std::int32_t GetVersion(RE::StaticFunctionTag*)
		{
			return kVersion;
		}

		// Checks the ground under the first `count` footprints (the fire and the
		// tents). Returns the first problem found, or kClear.
		std::int32_t CheckCampSite(RE::StaticFunctionTag*, RE::TESObjectREFR* a_origin, float a_headingDegrees,
			std::int32_t a_count, std::vector<float> a_fwd, std::vector<float> a_side, std::vector<float> a_turn,
			std::vector<float> a_halfWidth, std::vector<float> a_halfDepth, float a_maxStep, float a_maxSlopeDegrees)
		{
			const auto count = static_cast<std::size_t>(std::max(a_count, 0));
			if (!a_origin || a_fwd.size() < count || a_side.size() < count || a_turn.size() < count ||
				a_halfWidth.size() < count || a_halfDepth.size() < count) {
				return kBadArguments;
			}

			const float heading = ToRadians(a_headingDegrees);
			for (std::size_t i = 0; i < count; ++i) {
				const auto centre = LocalToWorld(a_origin, heading, a_fwd[i], a_side[i]);
				const auto result = CheckFootprint(a_origin, centre, heading + ToRadians(a_turn[i]),
					a_halfWidth[i], a_halfDepth[i], a_maxStep, a_maxSlopeDegrees);
				if (result != kClear) {
					return result;
				}
			}
			return kClear;
		}

		struct PendingRef
		{
			RE::TESBoundObject* base{ nullptr };
			RE::NiPoint3        position;
			RE::NiPoint3        angles;
			std::uint16_t       scale{ 100 };
		};

		// Finds the ground under (x, y) and works out the angles for an object
		// whose rotation, before any slope tilt, is `a_rotation` (yaw `a_yaw`).
		// Fills in the position's z (ground + lift, or origin height + lift if
		// no ground was found) and the angles.
		void GroundAndTilt(RE::TESObjectREFR* a_origin, PendingRef& a_ref, float a_yaw, const RE::NiMatrix3& a_relative,
			float a_lift, bool a_alignToSlope, float a_maxTiltDegrees)
		{
			RE::NiMatrix3 frame = Placement::MatrixFromAngles({ 0.0f, 0.0f, a_yaw });
			if (const auto hit = GroundAt(a_origin, a_ref.position.x, a_ref.position.y)) {
				a_ref.position.z = hit->point.z;
				if (a_alignToSlope) {
					frame = Placement::MatrixFromAngles(
						Placement::AnglesForSurface(a_yaw, Placement::ClampTilt(hit->normal, a_maxTiltDegrees)));
				}
			}
			a_ref.position.z += a_lift;
			a_ref.angles = Placement::AnglesFromMatrix(frame * a_relative);
		}

		// Places every form in the layout on the ground, tilted to the slope if
		// asked, plus the dressing (clutter) around each layout object.
		//
		// Dressing sets are Creation Kit references in a holding cell: each set
		// has an anchor marker (a_setAnchors[s]) standing in for its camp object,
		// and items (a_setItems[j], in set a_itemSet[j], 1-based; 0 = unused)
		// arranged around it. A layout object with a_dressSet[i] = s + 1 gets a
		// copy of set s, with each item's offset and rotation relative to the
		// anchor carried over. Items sit on the ground at their own spot, raised
		// by their height above the anchor.
		//
		// The first item found that is one of a_storageMarkers marks where the
		// player's storage goes: a copy of it is placed and returned at index
		// count (or None there), for the script to move the storage onto.
		//
		// Returns the layout's references (None where the form is None), then
		// the storage spot, then the dressing. The ground under every object is
		// found before anything is placed, so nothing lands on a tent's roof.
		// Returns an empty array if the arguments don't line up.
		std::vector<RE::TESObjectREFR*> PlaceCamp(RE::StaticFunctionTag*, RE::TESObjectREFR* a_origin, float a_headingDegrees,
			std::vector<RE::TESForm*> a_forms, std::vector<float> a_fwd, std::vector<float> a_side,
			std::vector<float> a_lift, std::vector<float> a_turn, bool a_alignToSlope, float a_maxTiltDegrees,
			std::vector<std::int32_t> a_dressSet, std::vector<RE::TESObjectREFR*> a_setAnchors,
			std::vector<RE::TESObjectREFR*> a_setItems, std::vector<std::int32_t> a_itemSet,
			std::vector<RE::TESObjectREFR*> a_storageMarkers)
		{
			const std::size_t count = a_forms.size();
			auto*             dataHandler = RE::TESDataHandler::GetSingleton();
			if (!a_origin || !dataHandler || a_fwd.size() < count || a_side.size() < count ||
				a_lift.size() < count || a_turn.size() < count || a_dressSet.size() < count ||
				a_itemSet.size() < a_setItems.size()) {
				return {};
			}

			const float         heading = ToRadians(a_headingDegrees);
			const RE::NiMatrix3 identity;

			// Index 0..count-1: the layout. count: the storage spot. Then dressing.
			std::vector<PendingRef> pending(count + 1);
			bool                    storageFound = false;

			for (std::size_t i = 0; i < count; ++i) {
				auto* base = a_forms[i] ? a_forms[i]->As<RE::TESBoundObject>() : nullptr;
				if (!base) {
					continue;
				}
				auto& ref = pending[i];
				ref.base = base;
				ref.position = LocalToWorld(a_origin, heading, a_fwd[i], a_side[i]);
				const float yaw = heading + ToRadians(a_turn[i]);
				GroundAndTilt(a_origin, ref, yaw, identity, a_lift[i], a_alignToSlope, a_maxTiltDegrees);

				const std::int32_t set = a_dressSet[i] - 1;
				if (set < 0 || static_cast<std::size_t>(set) >= a_setAnchors.size() || !a_setAnchors[set]) {
					continue;
				}
				auto*               anchor = a_setAnchors[set];
				const RE::NiMatrix3 anchorRotation = Placement::MatrixFromAngles(anchor->GetAngle());
				const RE::NiMatrix3 toAnchor = anchorRotation.Transpose();
				const float         sinYaw = std::sin(yaw);
				const float         cosYaw = std::cos(yaw);

				for (std::size_t j = 0; j < a_setItems.size(); ++j) {
					auto* item = a_setItems[j];
					if (!item || a_itemSet[j] != set + 1) {
						continue;
					}
					auto* itemBase = item->GetBaseObject();
					if (!itemBase) {
						continue;
					}

					// The item's offset and rotation in the anchor's own frame.
					const RE::NiPoint3  offset = toAnchor * (item->GetPosition() - anchor->GetPosition());
					const RE::NiMatrix3 relative = toAnchor * Placement::MatrixFromAngles(item->GetAngle());

					PendingRef dressing;
					dressing.base = itemBase;
					dressing.scale = item->GetReferenceRuntimeData().refScale;
					dressing.position = {
						ref.position.x + offset.x * cosYaw + offset.y * sinYaw,
						ref.position.y - offset.x * sinYaw + offset.y * cosYaw,
						0.0f
					};
					GroundAndTilt(a_origin, dressing, yaw, relative, offset.z, a_alignToSlope, a_maxTiltDegrees);

					const bool isStorage = std::ranges::find(a_storageMarkers, item) != a_storageMarkers.end();
					if (isStorage) {
						if (!storageFound) {
							pending[count] = dressing;
							storageFound = true;
						}
						continue;
					}
					pending.push_back(dressing);
				}
			}

			std::vector<RE::TESObjectREFR*> refs(pending.size(), nullptr);
			for (std::size_t i = 0; i < pending.size(); ++i) {
				const auto& ref = pending[i];
				if (!ref.base) {
					continue;
				}
				const auto handle = dataHandler->CreateReferenceAtLocation(ref.base, ref.position, ref.angles,
					CellAt(a_origin, ref.position.x, ref.position.y), a_origin->GetWorldspace(),
					nullptr, nullptr, RE::ObjectRefHandle(), false, true);
				auto* created = handle.get().get();
				if (created && ref.scale != 100) {
					created->GetReferenceRuntimeData().refScale = ref.scale;
				}
				refs[i] = created;
			}
			return refs;
		}

		// Hides grass under the references (the camp), replacing any earlier
		// camp's; see GrassHider.
		void HideGrass(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> a_refs)
		{
			GrassHider::Hide(a_refs);
		}

		void ShowGrass(RE::StaticFunctionTag*)
		{
			GrassHider::Show();
		}
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("GetVersion"sv, kClass, GetVersion);
		a_vm->RegisterFunction("CheckCampSite"sv, kClass, CheckCampSite);
		a_vm->RegisterFunction("PlaceCamp"sv, kClass, PlaceCamp);
		a_vm->RegisterFunction("HideGrass"sv, kClass, HideGrass);
		a_vm->RegisterFunction("ShowGrass"sv, kClass, ShowGrass);
		logger::info("Registered {} native functions", kClass);
		return true;
	}
}
