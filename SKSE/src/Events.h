#pragma once

// The game events the camp reacts to, passed on to the quest scripts instead
// of scripts on the objects themselves: casting Set Up Camp, activating a camp
// object, and the player sitting down. (The tub's trigger box has its own
// small script.)
namespace Events
{
	// Call once the game's data has loaded (kDataLoaded).
	void Install();
}
