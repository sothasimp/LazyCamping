#pragma once

// Renames forms in game, keeping the new names in the SKSE co-save so they
// survive reloading (SKSE's own Form.SetName doesn't).
namespace FormNames
{
	// Gives a_form a new name, or its own name back if a_name is empty.
	void Rename(RE::TESForm* a_form, std::string_view a_name);

	// The new names in the co-save. Load returns whether the record was this
	// module's.
	void Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
	void Revert();
}
