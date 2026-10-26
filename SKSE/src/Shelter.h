#pragma once

// Makes the camp's tents warm and rainproof for Frostfall, through Campfire,
// if Campfire.esm is loaded. Neither is a master of LazyCamping.esp.
namespace Shelter
{
	// Starts the once-a-second check of whether the player is in a tent. Call
	// from SKSEPluginLoad.
	void Install();

	// The camp's tents (None entries are skipped), replacing any earlier
	// camp's. Their base objects get Campfire's warm and waterproof keywords.
	void SetTents(const std::vector<RE::TESObjectREFR*>& a_tents);

	// Forgets the tents, leaving Campfire's current tent empty if it was one.
	void ClearTents();

	// The tents in the SKSE co-save. Load returns whether the record was this
	// module's.
	void Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
	void Revert();
}
