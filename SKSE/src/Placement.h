#pragma once

namespace Placement
{
	struct GroundHit
	{
		RE::NiPoint3 point;
		RE::NiPoint3 normal;  // unit length, pointing away from the surface
	};

	// Casts a ray straight down from (x, y, top) for `length` units against
	// terrain and static geometry. Must be called on the main thread.
	std::optional<GroundHit> CastDown(RE::TESObjectCELL* a_cell, float a_x, float a_y, float a_top, float a_length);

	// Reference angles (radians, X/Y/Z as used by TESObjectREFR) that stand an
	// object upright on a surface with the given normal while facing `a_yaw`.
	RE::NiPoint3 AnglesForSurface(float a_yaw, RE::NiPoint3 a_normal);

	// Rotation matrix for reference angles (radians), and back, as the game
	// builds it (NiMatrix3::SetEulerAnglesXYZ): turned about Z first, then
	// tilted about the world's Y and X. Its columns are the object's right,
	// forward and up axes in world space.
	RE::NiMatrix3 MatrixFromAngles(const RE::NiPoint3& a_angles);
	RE::NiPoint3  AnglesFromMatrix(const RE::NiMatrix3& a_matrix);

	// Slope of a surface in degrees (0 = flat).
	float SlopeDegrees(const RE::NiPoint3& a_normal);

	// Returns the normal tilted toward vertical so its slope is at most
	// `a_maxDegrees`.
	RE::NiPoint3 ClampTilt(RE::NiPoint3 a_normal, float a_maxDegrees);

	// True if the point is below the water surface of its cell.
	bool IsUnderwater(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_point);
}
