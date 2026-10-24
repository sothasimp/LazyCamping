#include "Papyrus.h"

#include "Placement.h"

// Native functions for CSK_Native.psc. They are registered as not callable
// from tasklets, so the VM runs them in sync with the main thread, which is
// required for the Havok raycasts.
namespace Papyrus
{
	namespace
	{
		constexpr auto kClass = "CSK_Native"sv;
		constexpr std::int32_t kVersion = 1;

		// Result codes for IsAreaClear (mirrored in CSK_CampsiteQuestScript).
		enum AreaResult : std::int32_t
		{
			kClear = 0,
			kNoGround = 1,
			kTooSteep = 2,
			kUneven = 3,
			kUnderwater = 4
		};

		constexpr float ToRadians(float a_deg) { return a_deg * std::numbers::pi_v<float> / 180.0f; }
		constexpr float ToDegrees(float a_rad) { return a_rad * 180.0f / std::numbers::pi_v<float>; }

		RE::TESObjectCELL* CellAt(RE::TESObjectREFR* a_origin, float a_x, float a_y)
		{
			if (auto* tes = RE::TES::GetSingleton()) {
				if (auto* cell = tes->GetCell({ a_x, a_y, a_origin->GetPositionZ() })) {
					return cell;
				}
			}
			return a_origin->GetParentCell();
		}

		std::optional<Placement::GroundHit> GroundAt(RE::TESObjectREFR* a_origin, float a_x, float a_y, float a_searchUp, float a_searchDown)
		{
			const float top = a_origin->GetPositionZ() + a_searchUp;
			return Placement::CastDown(CellAt(a_origin, a_x, a_y), a_x, a_y, top, a_searchUp + a_searchDown);
		}

		std::int32_t GetVersion(RE::StaticFunctionTag*)
		{
			return kVersion;
		}

		// Returns [groundZ, angleX, angleY, angleZ, slopeDegrees] (angles in degrees),
		// or an empty array if no ground was found.
		std::vector<float> FindPlacement(RE::StaticFunctionTag*, RE::TESObjectREFR* a_origin,
			float a_x, float a_y, float a_yawDegrees, bool a_alignToSlope, float a_maxTiltDegrees,
			float a_searchUp, float a_searchDown)
		{
			if (!a_origin) {
				return {};
			}

			const auto hit = GroundAt(a_origin, a_x, a_y, a_searchUp, a_searchDown);
			if (!hit) {
				return {};
			}

			const float  yaw = ToRadians(a_yawDegrees);
			RE::NiPoint3 angles{ 0.0f, 0.0f, yaw };
			if (a_alignToSlope) {
				angles = Placement::AnglesForSurface(yaw, Placement::ClampTilt(hit->normal, a_maxTiltDegrees));
			}

			return {
				hit->point.z,
				ToDegrees(angles.x),
				ToDegrees(angles.y),
				ToDegrees(angles.z),
				Placement::SlopeDegrees(hit->normal)
			};
		}

		// Samples the centre and four corners of a rectangular footprint
		// (half-sizes along the object's right and forward axes).
		std::int32_t IsAreaClear(RE::StaticFunctionTag*, RE::TESObjectREFR* a_origin,
			float a_x, float a_y, float a_yawDegrees, float a_halfWidth, float a_halfDepth,
			float a_maxStep, float a_maxSlopeDegrees, float a_searchUp, float a_searchDown)
		{
			if (!a_origin) {
				return kNoGround;
			}

			const float yaw = ToRadians(a_yawDegrees);
			const float sinA = std::sin(yaw);
			const float cosA = std::cos(yaw);

			constexpr std::array<std::pair<float, float>, 5> samples{ { { 0.0f, 0.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f } } };

			float lowest = std::numeric_limits<float>::max();
			float highest = std::numeric_limits<float>::lowest();

			for (std::size_t i = 0; i < samples.size(); ++i) {
				const float r = samples[i].first * a_halfWidth;
				const float f = samples[i].second * a_halfDepth;
				const float x = a_x + f * sinA + r * cosA;
				const float y = a_y + f * cosA - r * sinA;

				const auto hit = GroundAt(a_origin, x, y, a_searchUp, a_searchDown);
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
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("GetVersion"sv, kClass, GetVersion);
		a_vm->RegisterFunction("FindPlacement"sv, kClass, FindPlacement);
		a_vm->RegisterFunction("IsAreaClear"sv, kClass, IsAreaClear);
		logger::info("Registered {} native functions", kClass);
		return true;
	}
}
