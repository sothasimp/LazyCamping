#pragma once

// The bedroll kit's look follows the camp's bedroll style (see the SKSE menu):
// its "cloth" shape takes one of Lazy Camping's bedroll texture sets.
namespace BedrollKit
{
	// 0 vanilla, 1 green, 2 blue, 3 red, 4 mixed colours (shown green). Main
	// thread only. Changes the kit's base form, so it lasts until the game
	// closes, and has to be applied again after loading a save.
	void Apply(std::int32_t a_style);
}
