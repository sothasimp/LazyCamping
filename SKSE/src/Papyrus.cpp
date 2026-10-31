#include "Papyrus.h"

#include "BedrollSeats.h"
#include "CampLayout.h"
#include "FormNames.h"
#include "GrassHider.h"
#include "Placement.h"

// Native functions, declared Global Native in LZC_CampsiteQuestScript.psc. They are registered as not callable
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
		constexpr auto         kClass = "LZC_CampsiteQuestScript"sv;
		constexpr std::int32_t kVersion = 13;

		constexpr RE::FormID kXMarkerHeading = 0x34;  // Skyrim.esm

		// How far above/below the origin to look for ground.
		constexpr float kSearchUp = 256.0f;
		constexpr float kSearchDown = 1024.0f;

		// Result codes for the ground check (mirrored in LZC_CampsiteQuestScript).
		using AreaResult = Camp::Result;
		using Camp::kBadArguments;
		using Camp::kClear;
		using Camp::kNoGround;
		using Camp::kTooSteep;
		using Camp::kUnderwater;
		using Camp::kUneven;

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

		// Scales a reference the way Papyrus's SetScale does: the reference's
		// own scale, flagged as changed so the save keeps it, and its 3D if
		// that's already loaded (else it loads at the new scale).
		void SetScale(RE::TESObjectREFR* a_ref, std::uint16_t a_percent)
		{
			a_ref->GetReferenceRuntimeData().refScale = a_percent;
			a_ref->AddChange(RE::TESObjectREFR::ChangeFlags::kScale);
			if (auto* node = a_ref->Get3D()) {
				node->local.scale = static_cast<float>(a_percent) / 100.0f;
				RE::NiUpdateData update{};
				node->Update(update);
			}
		}

		struct PendingRef
		{
			RE::TESBoundObject* base{ nullptr };
			RE::NiPoint3        position;
			RE::NiPoint3        angles;
			std::uint16_t       scale{ 100 };
		};

		// The least the ground is sampled out from an object's centre when
		// working out its slope, for objects smaller than this.
		constexpr float kMinSlopeSpan = 32.0f;

		// The slope of the ground under an object's whole footprint (its
		// bounds, facing `a_yaw`, centred on `a_centre`): a plane through the
		// ground at the four corners, so a bump or a single terrain triangle
		// under the centre doesn't tip it. `a_fallback` if a corner finds no
		// ground.
		RE::NiPoint3 FootprintNormal(RE::TESObjectREFR* a_origin, const RE::TESBoundObject* a_base, const RE::NiPoint3& a_centre,
			float a_yaw, const RE::NiPoint3& a_fallback)
		{
			const auto& bounds = a_base->boundData;
			// The bounds in the object's own frame: x right, y forward.
			const float midX = (bounds.boundMin.x + bounds.boundMax.x) * 0.5f;
			const float midY = (bounds.boundMin.y + bounds.boundMax.y) * 0.5f;
			const float halfX = std::max((bounds.boundMax.x - bounds.boundMin.x) * 0.5f, kMinSlopeSpan);
			const float halfY = std::max((bounds.boundMax.y - bounds.boundMin.y) * 0.5f, kMinSlopeSpan);

			const RE::NiPoint3 right{ std::cos(a_yaw), -std::sin(a_yaw), 0.0f };
			const RE::NiPoint3 forward{ std::sin(a_yaw), std::cos(a_yaw), 0.0f };

			// Corners in the order (-x,-y), (+x,-y), (-x,+y), (+x,+y).
			std::array<float, 4> z{};
			for (std::size_t i = 0; i < z.size(); ++i) {
				const float        x = midX + ((i & 1) ? halfX : -halfX);
				const float        y = midY + ((i & 2) ? halfY : -halfY);
				const RE::NiPoint3 corner = a_centre + right * x + forward * y;
				const auto         hit = GroundAt(a_origin, corner.x, corner.y);
				if (!hit) {
					return a_fallback;
				}
				z[i] = hit->point.z;
			}

			// The rise per unit along the object's right and forward.
			const float riseX = ((z[1] + z[3]) - (z[0] + z[2])) / (4.0f * halfX);
			const float riseY = ((z[2] + z[3]) - (z[0] + z[1])) / (4.0f * halfY);
			RE::NiPoint3 normal = RE::NiPoint3{ 0.0f, 0.0f, 1.0f } - right * riseX - forward * riseY;
			normal.Unitize();
			return normal;
		}

		// Finds the ground under (x, y) and works out the angles for an object
		// with rotation `a_relative` in a frame facing `a_yaw` (tilted to the
		// slope under its footprint if asked). Fills in the position's z
		// (ground + lift, or origin height + lift if no ground was found) and
		// the angles.
		void GroundAndTilt(RE::TESObjectREFR* a_origin, PendingRef& a_ref, float a_yaw, const RE::NiMatrix3& a_relative,
			float a_lift, bool a_alignToSlope, float a_maxTiltDegrees)
		{
			RE::NiMatrix3 frame = Placement::MatrixFromAngles({ 0.0f, 0.0f, a_yaw });
			if (const auto hit = GroundAt(a_origin, a_ref.position.x, a_ref.position.y)) {
				a_ref.position.z = hit->point.z;
				if (a_alignToSlope) {
					const RE::NiPoint3 normal = FootprintNormal(a_origin, a_ref.base, hit->point, a_yaw, hit->normal);
					frame = Placement::MatrixFromAngles(
						Placement::AnglesForSurface(a_yaw, Placement::ClampTilt(normal, a_maxTiltDegrees)));
				}
			}
			a_ref.position.z += a_lift;
			a_ref.angles = Placement::AnglesFromMatrix(frame * a_relative);
		}

		// Places every form in the layout on the ground, tilted to the slope if
		// asked, plus the clutter around each layout object.
		//
		// Clutter sets are Creation Kit references in a holding cell: each set
		// has an anchor (a_setAnchors[s]) standing in for its camp object,
		// and items (a_setItems[j], in set a_itemSet[j], 1-based; 0 = unused)
		// arranged around it. A layout object with a_clutterSet[i] = s + 1 gets a
		// copy of set s, with each item's offset and rotation relative to the
		// anchor carried over. The anchor's turn turns the layout object, and the
		// holding cell floor is z = 0: the layout object is raised by its
		// anchor's height above it. The clutter is fixed rigidly to the object,
		// level: from the object's own spot, each item as high above it as it is
		// above the holding cell floor, turned with it, whatever the ground does.
		//
		// The first item found that is one of a_storageMarkers marks where the
		// player's storage goes: a copy of it is placed and returned at index
		// count (or None there), for the script to move the storage onto.
		//
		// Objects marked in `level` stand level even with alignToSlope.
		//
		// Returns the layout's references (None where the form is None), then
		// the storage spot, then the clutter. The ground under every object is
		// found before anything is placed, so nothing lands on a tent's roof.
		// Returns an empty array if the arguments don't line up.
		std::vector<RE::TESObjectREFR*> PlaceImpl(const Camp::Layout& a_layout)
		{
			const auto&       a_forms = a_layout.forms;
			const auto&       a_fwd = a_layout.fwd;
			const auto&       a_side = a_layout.side;
			const auto&       a_lift = a_layout.lift;
			const auto&       a_turn = a_layout.turn;
			const auto&       a_level = a_layout.level;
			const auto&       a_clutterSet = a_layout.clutterSet;
			const auto&       a_setAnchors = a_layout.setAnchors;
			const auto&       a_setItems = a_layout.setItems;
			const auto&       a_itemSet = a_layout.itemSet;
			const auto&       a_storageMarkers = a_layout.storageMarkers;
			auto*             a_origin = a_layout.origin;
			const bool        a_alignToSlope = a_layout.alignToSlope;
			const float       a_maxTiltDegrees = a_layout.maxTiltDegrees;
			const float       a_headingDegrees = a_layout.headingDegrees;
			const std::size_t count = a_forms.size();
			auto*             dataHandler = RE::TESDataHandler::GetSingleton();
			if (!a_origin || !dataHandler || a_fwd.size() < count || a_side.size() < count ||
				a_lift.size() < count || a_turn.size() < count || a_clutterSet.size() < count ||
				a_itemSet.size() < a_setItems.size()) {
				return {};
			}

			const float         heading = ToRadians(a_headingDegrees);
			const RE::NiMatrix3 identity;

			// Index 0..count-1: the layout. count: the storage spot. Then clutter.
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

				const std::int32_t set = a_clutterSet[i] - 1;
				RE::TESObjectREFR* anchor = nullptr;
				if (set >= 0) {
					if (static_cast<std::size_t>(set) < a_setAnchors.size()) {
						anchor = a_setAnchors[set];
					}
					if (!anchor) {
						logger::warn("PlaceCamp: object {} ({:08X}) has clutter set {}, but its anchor isn't set or isn't found", i,
							base->GetFormID(), set + 1);
					}
				}
				// The anchor's turn in the holding cell turns the object (and its
				// clutter with it), and its height above the holding cell floor
				// (z = 0) raises it.
				const float yaw = heading + ToRadians(a_turn[i]) + (anchor ? anchor->GetAngle().z : 0.0f);
				const float lift = a_lift[i] + (anchor ? anchor->GetPosition().z : 0.0f);
				const bool level = i < a_level.size() && a_level[i];
				GroundAndTilt(a_origin, ref, yaw, identity, lift, a_alignToSlope && !level, a_maxTiltDegrees);
				if (!anchor) {
					continue;
				}
				const RE::NiMatrix3 anchorRotation = Placement::MatrixFromAngles(anchor->GetAngle());
				const RE::NiMatrix3 toAnchor = anchorRotation.Transpose();
				// The clutter is fixed rigidly to the object: placed from the
				// object's own spot and turned with it, level, exactly as around
				// the anchor in the holding cell. The ground and any slope tilt
				// don't come into it.
				const RE::NiPoint3  objectGround{ ref.position.x, ref.position.y, ref.position.z - lift };
				const RE::NiMatrix3 levelFrame = Placement::MatrixFromAngles({ 0.0f, 0.0f, yaw });

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

					// Its height is its own above the holding cell floor (z = 0), so
					// moving the anchor up or down doesn't carry the clutter with it.
					const bool          isStorage = std::ranges::find(a_storageMarkers, item) != a_storageMarkers.end();
					const RE::NiMatrix3 frame = levelFrame;
					const RE::NiPoint3  local{ offset.x, offset.y, item->GetPosition().z };

					PendingRef clutter;
					clutter.base = itemBase;
					clutter.scale = item->GetReferenceRuntimeData().refScale;
					clutter.position = objectGround + frame * local;
					clutter.angles = Placement::AnglesFromMatrix(frame * relative);
					if (isStorage) {
						if (!storageFound) {
							// Whatever the marker is (the backpack itself, say), the
							// spot is an invisible marker the script moves the
							// backpack onto, then deletes.
							if (auto* marker = RE::TESForm::LookupByID<RE::TESBoundObject>(kXMarkerHeading)) {
								clutter.base = marker;
							}
							pending[count] = clutter;
							storageFound = true;
						}
						continue;
					}
					pending.push_back(clutter);
				}
			}

			if (!storageFound) {
				logger::warn("PlaceCamp: none of the {} storage markers is in a placed clutter set", a_storageMarkers.size());
			}

			std::vector<RE::TESObjectREFR*> refs(pending.size(), nullptr);
			for (std::size_t i = 0; i < pending.size(); ++i) {
				const auto& ref = pending[i];
				if (!ref.base) {
					continue;
				}
				// Z brought into 0-360 degrees.
				const float        fullTurn = 2.0f * std::numbers::pi_v<float>;
				const RE::NiPoint3 angles{ ref.angles.x, ref.angles.y,
					ref.angles.z - fullTurn * std::floor(ref.angles.z / fullTurn) };
				const auto handle = dataHandler->CreateReferenceAtLocation(ref.base, ref.position, angles,
					CellAt(a_origin, ref.position.x, ref.position.y), a_origin->GetWorldspace(),
					nullptr, nullptr, RE::ObjectRefHandle(), false, true);
				auto* created = handle.get().get();
				if (created && ref.scale != 100) {
					SetScale(created, ref.scale);
				}
				if (!created) {
					logger::warn("PlaceCamp: couldn't create {:08X} (index {})", ref.base->GetFormID(), i);
				}
				refs[i] = created;
			}
			return refs;
		}

		void ShowGrass(RE::StaticFunctionTag*)
		{
			GrassHider::Show();
		}

		void ClearBedrollSeats(RE::StaticFunctionTag*)
		{
			BedrollSeats::Clear();
		}

		// Renames a form, or gives it its own name back if a_name is empty.
		// The name is kept in the save; see FormNames.
		void RenameForm(RE::StaticFunctionTag*, RE::TESForm* a_form, RE::BSFixedString a_name)
		{
			FormNames::Rename(a_form, a_name.c_str());
		}
	}

	namespace Camp
	{
		std::int32_t CheckSite(RE::TESObjectREFR* a_origin, float a_headingDegrees, const std::vector<Footprint>& a_footprints,
			float a_maxStep, float a_maxSlopeDegrees)
		{
			if (!a_origin) {
				return kBadArguments;
			}
			const float heading = ToRadians(a_headingDegrees);
			for (const auto& footprint : a_footprints) {
				const auto centre = LocalToWorld(a_origin, heading, footprint.fwd, footprint.side);
				const auto result = CheckFootprint(a_origin, centre, heading + ToRadians(footprint.turn),
					footprint.halfWidth, footprint.halfDepth, a_maxStep, a_maxSlopeDegrees);
				if (result != kClear) {
					return result;
				}
			}
			return kClear;
		}

		std::vector<RE::TESObjectREFR*> Place(const Layout& a_layout)
		{
			return PlaceImpl(a_layout);
		}
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("GetVersion"sv, kClass, GetVersion);
		a_vm->RegisterFunction("ShowGrass"sv, kClass, ShowGrass);
		a_vm->RegisterFunction("RenameForm"sv, kClass, RenameForm);
		a_vm->RegisterFunction("ClearBedrollSeats"sv, kClass, ClearBedrollSeats);
		CampLayout::Register(a_vm);
		logger::info("Registered {} native functions", kClass);
		return true;
	}
}
