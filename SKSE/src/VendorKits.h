#pragma once

// Adds Wayfarer - A Convenient Camping Framework's kits (the LZC_LItemCampingKits
// leveled list) to the merchant leveled lists in LZC_VendorLists, while the game
// runs, so no vanilla list is overridden in the plugin.
//
// The merchant lists are only changed once, when the game's data has loaded:
// before any save is loaded, and so before scripts (Papyrus AddForm, from
// other mods) have added entries. The game keeps pointers to those entries
// inside the list's array, which resizing it would leave dangling, crashing
// the next save. Turning the kits off and on afterwards only changes
// LZC_LItemCampingKits' own chance of giving nothing.
namespace VendorKits
{
	// Adds the kits to every list in LZC_VendorLists. Call once, at kDataLoaded.
	void Inject();

	// Whether merchants can roll the kits: LZC_LItemCampingKits' chance none is
	// its own (50%) when enabled, 100% when not. Main thread only; apply again
	// after loading a save.
	void Apply(bool a_enabled);
}
