#pragma once

// Clears the grass around the camp, with or without No Grass In Objects'
// grass cache. All of these run on the main thread.
namespace GrassHider
{
	// Starts the once-a-second check that hides grass loading in around a
	// camp. Call from SKSEPluginLoad.
	void Install();

	// The camp's circle in the SKSE co-save. Load returns whether the record
	// was this module's.
	void Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
	void Revert();

	// Hides grass in a circle just big enough to hold every reference (by
	// its object bounds), replacing any earlier camp's.
	void Hide(const std::vector<RE::TESObjectREFR*>& a_refs);

	// Brings the grass back: the camp's cells rebuild their grass.
	void Show();
}
