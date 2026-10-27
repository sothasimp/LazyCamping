#pragma once

// Sneak-activating a camp bedroll sits the player on its seat (the invisible
// cross-legged marker on it) instead of opening the sleep menu.
namespace BedrollSeats
{
	// Hooks furniture activation. Call from SKSEPluginLoad.
	void Install();

	// The camp's bedrolls and the seat on each (same index; pairs with a
	// None are skipped), replacing any earlier camp's.
	void Set(const std::vector<RE::TESObjectREFR*>& a_beds, const std::vector<RE::TESObjectREFR*>& a_seats);
	void Clear();

	// The pairs in the SKSE co-save. Load returns whether the record was
	// this module's.
	void Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
	void Revert();
}
