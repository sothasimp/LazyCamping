#pragma once

// The game events the camp reacts to, passed on to the quest scripts instead
// of scripts on the objects themselves: casting Set Up Camp, activating a camp
// object, the player sitting down, and followers stepping into or out of the
// tub (checked once a second around LZC_TubTrigger).
namespace Events
{
	// Call once the game's data has loaded (kDataLoaded).
	void Install();
}
