#include "VendorKits.h"

namespace VendorKits
{
	namespace
	{
		constexpr auto          kPlugin = "LazyCamping.esp"sv;
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

		// Takes the kits out of a_list, then puts them back in if a_add, keeping
		// the entries in level order as the game expects.
		void Patch(RE::TESLevItem* a_list, RE::TESLevItem* a_kits, bool a_add)
		{
			auto&                            entries = a_list->entries;
			std::vector<RE::LEVELED_OBJECT> objects(entries.begin(), entries.begin() + a_list->numEntries);
			std::erase_if(objects, [&](const RE::LEVELED_OBJECT& a_object) { return a_object.form == a_kits; });
			if (a_add) {
				if (objects.size() >= 255) {
					logger::warn("Leveled list {:08X} is full; the kits weren't added", a_list->GetFormID());
				} else {
					const auto at = std::ranges::find_if(objects, [](const RE::LEVELED_OBJECT& a_object) { return a_object.level > kLevel; });
					objects.insert(at, RE::LEVELED_OBJECT{ a_kits, 1, kLevel, 0, nullptr });
				}
			}
			if (objects.size() == a_list->numEntries) {
				return;
			}
			entries.resize(objects.size());
			std::ranges::copy(objects, entries.begin());
			a_list->numEntries = static_cast<std::uint8_t>(objects.size());
		}
	}

	void Apply(bool a_enabled)
	{
		static auto* kits = OnlyFormInPlugin<RE::TESLevItem>();
		static auto* lists = OnlyFormInPlugin<RE::BGSListForm>();
		if (!kits || !lists) {
			return;
		}
		for (auto* form : lists->forms) {
			if (auto* list = form ? form->As<RE::TESLevItem>() : nullptr) {
				Patch(list, kits, a_enabled);
			}
		}
	}
}
