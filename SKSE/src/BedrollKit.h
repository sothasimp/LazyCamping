#pragma once

// The bedroll kit's look follows the camp's bedroll style (see the SKSE menu):
// its "cloth" shape takes one of Wayfarer's Rest - A Convenient Camping Mod's bedroll texture sets.
namespace BedrollKit
{
	// 0 vanilla, 1 green, 2 blue, 3 red, 4 mixed colours (shown green). The
	// kit and texture sets come from the campsite quest script's properties
	// (a_script). Main thread only. Changes the kit's base form, so it lasts
	// until the game closes, and has to be applied again after loading a save.
	void Apply(RE::BSScript::Object* a_script, std::int32_t a_style);
}
