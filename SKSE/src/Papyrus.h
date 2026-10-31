#pragma once

namespace Papyrus
{
	bool Register(RE::BSScript::IVirtualMachine* a_vm);

	// Ground checks and placing, for laying out a camp (see CampLayout). Main
	// thread only. Layouts are offsets from an origin (the player): `fwd` units
	// in front along the heading and `side` units to the right, with `turn`
	// degrees added to the heading for each object's facing.
	namespace Camp
	{
		enum Result : std::int32_t
		{
			kClear = 0,
			kNoGround = 1,
			kTooSteep = 2,
			kUneven = 3,
			kUnderwater = 4,
			kBadArguments = 5
		};

		struct Footprint
		{
			float fwd{ 0.0f };
			float side{ 0.0f };
			float turn{ 0.0f };
			float halfWidth{ 0.0f };
			float halfDepth{ 0.0f };
		};

		// The first problem with the ground under any footprint, or kClear.
		std::int32_t CheckSite(RE::TESObjectREFR* a_origin, float a_headingDegrees, const std::vector<Footprint>& a_footprints,
			float a_maxStep, float a_maxSlopeDegrees);

		struct Layout
		{
			RE::TESObjectREFR*              origin{ nullptr };
			float                           headingDegrees{ 0.0f };
			std::vector<RE::TESForm*>       forms;
			std::vector<float>              fwd;
			std::vector<float>              side;
			std::vector<float>              lift;
			std::vector<float>              turn;
			std::vector<float>              maxTilt;     // per object, degrees; below 0 for maxTiltDegrees, 0 stands level
			std::vector<std::int32_t>       clutterSet;  // 1-based clutter set per object, 0 for none
			std::vector<bool>               tiltClutter; // clutter tilts with its object, not level
			std::vector<RE::TESObjectREFR*> setAnchors;
			std::vector<RE::TESObjectREFR*> setItems;
			std::vector<std::int32_t>       itemSet;  // 1-based set of each item
			std::vector<RE::TESObjectREFR*> storageMarkers;
			bool                            alignToSlope{ true };
			float                           maxTiltDegrees{ 25.0f };
		};

		// Places the layout and its clutter in one frame. Returns the layout's
		// references (nullptr where there's no form), then the storage spot
		// (or nullptr), then the clutter; empty if the layout doesn't line up.
		std::vector<RE::TESObjectREFR*> Place(const Layout& a_layout);
	}
}
