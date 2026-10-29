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
		// The frame we want, as the object's right, forward and up axes in
		// world space, turned into angles.
		RE::NiPoint3 up = a_normal;
		up.Unitize();

		const RE::NiPoint3 heading{ std::sin(a_yaw), std::cos(a_yaw), 0.0f };
		RE::NiPoint3       forward = heading - up * heading.Dot(up);
		if (forward.Unitize() == 0.0f) {
			return { 0.0f, 0.0f, a_yaw };
		}
		const RE::NiPoint3 right = forward.Cross(up);

		RE::NiMatrix3 frame;
		for (std::size_t i = 0; i < 3; ++i) {
			frame.entry[i][0] = right[i];
			frame.entry[i][1] = forward[i];
			frame.entry[i][2] = up[i];
		}
		return AnglesFromMatrix(frame);
	}

	RE::NiMatrix3 MatrixFromAngles(const RE::NiPoint3& a_angles)
	{
		RE::NiMatrix3 matrix;
		matrix.SetEulerAnglesXYZ(a_angles);
		return matrix;
	}

	RE::NiPoint3 AnglesFromMatrix(const RE::NiMatrix3& a_matrix)
	{
		// The inverse of SetEulerAnglesXYZ. Not NiMatrix3::ToEulerAnglesXYZ:
		// the NiFastATan2 it uses gets the sign wrong for half the angles.
		const auto& m = a_matrix.entry;
		const float cosY = std::sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1]);
		const float y = std::atan2(-m[0][2], cosY);
		if (cosY < 1e-6f) {
			// Straight up or down: X and Z turn about the same axis, so Z is 0.
			return { std::atan2(-m[0][2] * m[1][0], m[1][1]), y, 0.0f };
		}
		return { std::atan2(m[1][2], m[2][2]), y, std::atan2(m[0][1], m[0][0]) };
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
