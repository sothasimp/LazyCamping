#include "VendorKits.h"

namespace VendorKits
{
	namespace
	{
		constexpr auto          kPlugin = "Wayfarer - A Convenient Camping Framework.esp"sv;
		constexpr std::uint16_t kLevel = 1;

		// The plugin's only form of this type (LZC_LItemCampingKits, LZC_VendorLists),
		// whatever its FormID: their EditorIDs aren't kept in game.
		template <class T>
		T* OnlyFormInPlugin()
		{
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				return nullptr;
			}
			for (auto* form : dataHandler->GetFormArray<T>()) {
				const auto* file = form ? form->GetFile(0) : nullptr;
				if (file && file->GetFilename() == kPlugin) {
					return form;
				}
			}
			return nullptr;
		}

		// Adds the kits to a_list, keeping the entries in level order as the
		// game expects. Leaves a list alone that already has the kits, or
		// already has entries added by scripts (whose pointers into the array
		// resizing it would break).
		void Add(RE::TESLevItem* a_list, RE::TESLevItem* a_kits)
		{
			auto& entries = a_list->entries;
			if (std::any_of(entries.begin(), entries.begin() + a_list->numEntries, [&](const RE::LEVELED_OBJECT& a_object) { return a_object.form == a_kits; })) {
				return;
			}
			// unk13/unk18: the count of, and pointers to, the entries scripts added.
			if (a_list->unk13 != 0 || a_list->unk18 != nullptr) {
				logger::warn("Leveled list {:08X} already has script-added entries; the kits weren't added to it", a_list->GetFormID());
				return;
			}
			if (a_list->numEntries >= 255) {
				logger::warn("Leveled list {:08X} is full; the kits weren't added", a_list->GetFormID());
				return;
			}
			std::vector<RE::LEVELED_OBJECT> objects(entries.begin(), entries.begin() + a_list->numEntries);
			const auto at = std::ranges::find_if(objects, [](const RE::LEVELED_OBJECT& a_object) { return a_object.level > kLevel; });
			objects.insert(at, RE::LEVELED_OBJECT{ a_kits, 1, kLevel, 0, nullptr });
			entries.resize(objects.size());
			std::ranges::copy(objects, entries.begin());
			a_list->numEntries = static_cast<std::uint8_t>(objects.size());
		}

		RE::TESLevItem* Kits()
		{
			static auto* kits = OnlyFormInPlugin<RE::TESLevItem>();
			return kits;
		}
	}

	void Inject()
	{
		auto* kits = Kits();
		auto* lists = OnlyFormInPlugin<RE::BGSListForm>();
		if (!kits || !lists) {
			return;
		}
		for (auto* form : lists->forms) {
			if (auto* list = form ? form->As<RE::TESLevItem>() : nullptr) {
				Add(list, kits);
			}
		}
	}

	void Apply(bool a_enabled)
	{
		auto* kits = Kits();
		if (!kits) {
			return;
		}
		static const std::int8_t ownChance = kits->chanceNone;
		kits->chanceNone = a_enabled ? ownChance : 100;
	}
}
