#pragma once

// Adds Lazy Camping's kits (the LZC_LItemCampingKits leveled list) to the
// merchant leveled lists in LZC_VendorLists, while the game runs, so no
// vanilla list is overridden in the plugin.
namespace VendorKits
{
	// Adds the kits to every list in LZC_VendorLists, or takes them out again.
	// Main thread only. Changes base forms, so it lasts until the game closes,
	// and has to be applied again after loading a save.
	void Apply(bool a_enabled);
}
