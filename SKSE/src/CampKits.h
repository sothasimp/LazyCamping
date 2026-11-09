#pragma once

// The tent and bedroll kits a camp was set up from are taken out of their
// owners' inventories while the camp is up and given back when it's packed
// up. What was taken from where is kept in the SKSE co-save.
namespace CampKits
{
	// Removes a_count of a_kit from a_container now and remembers it.
	// Returns how many were removed.
	std::int32_t Take(RE::TESObjectREFR* a_container, RE::TESBoundObject* a_kit, std::int32_t a_count);

	// Gives every kit taken back to where it came from (the player, if that
	// container is gone or dead), and forgets them.
	void ReturnAll();

	void Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
	void Revert();
}
