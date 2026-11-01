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

	// Unrolls the camp bedroll a_kit stands for, if it's one of the camp's
	// rolled-up bedrolls (see the BedrollsStartRolled setting). Returns
	// whether it was. Main thread only.
	bool UnrollKit(RE::TESObjectREFR* a_kit);

	// Lights or puts out the tent lantern a_ref is (unlit or lit), if it's
	// one of the camp's. Returns whether it was. Main thread only.
	bool ToggleLantern(RE::TESObjectREFR* a_ref);

	// Puts the camp's map marker back in the player's map marker list after
	// loading a save with a camp up (the list isn't kept in the save).
	void ListCampMapMarker();
}
