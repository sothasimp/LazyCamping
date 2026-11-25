#pragma once

// Shrubs and plants in the camp's way are placed objects, not grass: while the
// camp is up, the small trees (bushes, under kMaxShrubHeight) and flora inside
// the circle around it are disabled, and they're enabled again on pack-up.
// Persistent references (quest and scripted objects, as a rule) are left alone.
// What was disabled is kept in the SKSE co-save.
namespace ShrubHider
{
	// a_refs: the camp's placed references (the area they cover is cleared).
	void Hide(const std::vector<RE::TESObjectREFR*>& a_refs);
	void Show();

	void Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
	void Revert();
}
