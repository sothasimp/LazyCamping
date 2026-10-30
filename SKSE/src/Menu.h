#pragma once

// The "Lazy Camping" page in SKSE Menu Framework's Mod Control Panel, if
// that's installed.
namespace Menu
{
	// Call once every SKSE plugin has loaded (kPostLoad).
	void Register();

	// Applies the settings that change base forms: the bedroll kit's look (see
	// BedrollKit) and the kits at merchants (see VendorKits). Call after a save
	// loads or a new game starts.
	void ApplyLoadedSettings();
}
