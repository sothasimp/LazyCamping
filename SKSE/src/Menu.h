#pragma once

// The "Lazy Camping" page in SKSE Menu Framework's Mod Control Panel, if
// that's installed.
namespace Menu
{
	// Call once every SKSE plugin has loaded (kPostLoad).
	void Register();

	// Gives the bedroll kit the look of the bedroll style chosen on the menu
	// (see BedrollKit). Call after a save loads or a new game starts.
	void ApplyBedrollKit();
}
