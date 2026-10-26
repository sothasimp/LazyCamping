#pragma once

// Hides grass under the camp's objects, with or without No Grass In Objects'
// grass cache. All of these run on the main thread.
namespace GrassHider
{
	// Registers the save-game record and starts the once-a-second check that
	// hides grass loading in around a camp. Call from SKSEPluginLoad.
	void Install();

	// Hides grass under each reference's footprint (its object bounds, turned
	// to its heading, widened by a_margin), replacing any earlier camp's.
	void Hide(const std::vector<RE::TESObjectREFR*>& a_refs, float a_margin);

	// Brings the grass back: the camp's cells rebuild their grass.
	void Show();
}
