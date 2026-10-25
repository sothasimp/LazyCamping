#include "Placement.h"

namespace Placement
{
	namespace
	{
		RE::NiPoint3 ToNiPoint3(const RE::hkVector4& a_vec)
		{
			alignas(16) float v[4];
			_mm_store_ps(v, a_vec.quad);
			return { v[0], v[1], v[2] };
		}

		constexpr float ToDegrees(float a_rad) { return a_rad * 180.0f / std::numbers::pi_v<float>; }
		constexpr float ToRadians(float a_deg) { return a_deg * std::numbers::pi_v<float> / 180.0f; }
	}

	std::optional<GroundHit> CastDown(RE::TESObjectCELL* a_cell, float a_x, float a_y, float a_top, float a_length)
	{
		auto* world = a_cell ? a_cell->GetbhkWorld() : nullptr;
		if (!world) {
			return std::nullopt;
		}

		// Havok works in its own units; game units must be scaled.
		const float        scale = RE::bhkWorld::GetWorldScale();
		const RE::NiPoint3 from{ a_x, a_y, a_top };
		const RE::NiPoint3 to{ a_x, a_y, a_top - a_length };

		RE::bhkPickData pick;
		pick.rayInput.from = RE::hkVector4(from * scale);
		pick.rayInput.to = RE::hkVector4(to * scale);
		// The line-of-sight layer hits terrain, statics, trees and clutter, but not water.
		pick.rayInput.filterInfo = static_cast<std::uint32_t>(RE::COL_LAYER::kLOS);

		{
			RE::BSReadLockGuard lock(world->worldLock);
			world->PickObject(pick);
		}

		if (!pick.rayOutput.HasHit()) {
			return std::nullopt;
		}

		GroundHit hit;
		hit.point = { a_x, a_y, a_top - a_length * pick.rayOutput.hitFraction };
		hit.normal = ToNiPoint3(pick.rayOutput.normal);
		if (hit.normal.Unitize() == 0.0f) {
			hit.normal = { 0.0f, 0.0f, 1.0f };
		}
		return hit;
	}

	RE::NiPoint3 AnglesForSurface(float a_yaw, RE::NiPoint3 a_normal)
	{
		// The game builds a reference's rotation with NiMatrix3::EulerAnglesToAxesZXY,
		// whose columns are the object's right, forward and up axes in world space:
		//   forward = (sin z cos x, cos z cos x, -sin x)
		//   right.z = cos x sin y,  up.z = cos x cos y
		// Build the frame we want, then solve for x, y and z.
		RE::NiPoint3 up = a_normal;
		up.Unitize();

		const RE::NiPoint3 heading{ std::sin(a_yaw), std::cos(a_yaw), 0.0f };
		RE::NiPoint3       forward = heading - up * heading.Dot(up);
		if (forward.Unitize() == 0.0f) {
			return { 0.0f, 0.0f, a_yaw };
		}
		const RE::NiPoint3 right = forward.Cross(up);

		return {
			std::asin(std::clamp(-forward.z, -1.0f, 1.0f)),
			std::atan2(right.z, up.z),
			std::atan2(forward.x, forward.y)
		};
	}

	RE::NiMatrix3 MatrixFromAngles(const RE::NiPoint3& a_angles)
	{
		RE::NiMatrix3 matrix;
		matrix.EulerAnglesToAxesZXY(a_angles);
		return matrix;
	}

	RE::NiPoint3 AnglesFromMatrix(const RE::NiMatrix3& a_matrix)
	{
		// Inverse of EulerAnglesToAxesZXY (see AnglesForSurface).
		return {
			std::asin(std::clamp(-a_matrix.entry[2][1], -1.0f, 1.0f)),
			std::atan2(a_matrix.entry[2][0], a_matrix.entry[2][2]),
			std::atan2(a_matrix.entry[0][1], a_matrix.entry[1][1])
		};
	}

	float SlopeDegrees(const RE::NiPoint3& a_normal)
	{
		return ToDegrees(std::acos(std::clamp(a_normal.z, -1.0f, 1.0f)));
	}

	RE::NiPoint3 ClampTilt(RE::NiPoint3 a_normal, float a_maxDegrees)
	{
		const float slope = SlopeDegrees(a_normal);
		if (slope <= a_maxDegrees || a_normal.z <= 0.0f) {
			return a_normal;
		}

		// Shrink the horizontal component so tan(slope) == tan(max).
		const float factor = std::tan(ToRadians(a_maxDegrees)) / std::tan(ToRadians(slope));
		RE::NiPoint3 clamped{ a_normal.x * factor, a_normal.y * factor, a_normal.z };
		clamped.Unitize();
		return clamped;
	}

	bool IsUnderwater(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_point)
	{
		float waterHeight = 0.0f;
		return a_cell && a_cell->GetWaterHeight(a_point, waterHeight) && waterHeight > a_point.z;
	}
}
