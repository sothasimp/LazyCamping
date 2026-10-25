#include "Papyrus.h"

#include "Placement.h"

// Native functions for CSK_Native.psc. They are registered as not callable
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
		constexpr auto         kClass = "CSK_Native"sv;
		constexpr std::int32_t kVersion = 2;

		// How far above/below the origin to look for ground.
		constexpr float kSearchUp = 256.0f;
		constexpr float kSearchDown = 1024.0f;

		// Result codes for CheckCampSite (mirrored in CSK_CampsiteQuestScript).
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

		// Places every form in the layout on the ground, tilted to the slope if
		// asked, and returns the new references (None where the form is None).
		// The ground under every object is found before anything is placed, so
		// a bedroll's raycast can't hit the tent standing over it. Returns an
		// empty array if the arguments don't line up.
		std::vector<RE::TESObjectREFR*> PlaceCamp(RE::StaticFunctionTag*, RE::TESObjectREFR* a_origin, float a_headingDegrees,
			std::vector<RE::TESForm*> a_forms, std::vector<float> a_fwd, std::vector<float> a_side,
			std::vector<float> a_lift, std::vector<float> a_turn, bool a_alignToSlope, float a_maxTiltDegrees)
		{
			const std::size_t count = a_forms.size();
			auto*             dataHandler = RE::TESDataHandler::GetSingleton();
			if (!a_origin || !dataHandler || a_fwd.size() < count || a_side.size() < count ||
				a_lift.size() < count || a_turn.size() < count) {
				return {};
			}

			const float heading = ToRadians(a_headingDegrees);

			std::vector<RE::NiPoint3> positions(count);
			std::vector<RE::NiPoint3> angles(count);
			for (std::size_t i = 0; i < count; ++i) {
				if (!a_forms[i]) {
					continue;
				}
				const float yaw = heading + ToRadians(a_turn[i]);
				positions[i] = LocalToWorld(a_origin, heading, a_fwd[i], a_side[i]);
				angles[i] = { 0.0f, 0.0f, yaw };

				if (const auto hit = GroundAt(a_origin, positions[i].x, positions[i].y)) {
					positions[i].z = hit->point.z;
					if (a_alignToSlope) {
						angles[i] = Placement::AnglesForSurface(yaw, Placement::ClampTilt(hit->normal, a_maxTiltDegrees));
					}
				}
				positions[i].z += a_lift[i];
			}

			std::vector<RE::TESObjectREFR*> refs(count, nullptr);
			for (std::size_t i = 0; i < count; ++i) {
				auto* base = a_forms[i] ? a_forms[i]->As<RE::TESBoundObject>() : nullptr;
				if (!base) {
					continue;
				}
				const auto handle = dataHandler->CreateReferenceAtLocation(base, positions[i], angles[i],
					CellAt(a_origin, positions[i].x, positions[i].y), a_origin->GetWorldspace(),
					nullptr, nullptr, RE::ObjectRefHandle(), false, true);
				refs[i] = handle.get().get();
			}
			return refs;
		}
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("GetVersion"sv, kClass, GetVersion);
		a_vm->RegisterFunction("CheckCampSite"sv, kClass, CheckCampSite);
		a_vm->RegisterFunction("PlaceCamp"sv, kClass, PlaceCamp);
		logger::info("Registered {} native functions", kClass);
		return true;
	}
}
