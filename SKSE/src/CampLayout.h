#pragma once

// Setting up a camp in one native call: works out the layout from the camp
// quest's properties (which tents and bedrolls, and where everything stands),
// checks the ground, places it all, and settles the pieces that move to the
// camp (cook box, firepit, storage backpack, bedroll seats, grass, map
// marker). LZC_CampsiteQuestScript keeps the events, the follower quest and
// the tub's trigger and water.
namespace CampLayout
{
	void Register(RE::BSScript::IVirtualMachine* a_vm);
}
