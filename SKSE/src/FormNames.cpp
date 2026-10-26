#include "FormNames.h"

namespace FormNames
{
	namespace
	{
		constexpr std::uint32_t kNamesRecord = 'NAME';
		constexpr std::uint32_t kNamesVersion = 1;

		struct Renamed
		{
			std::string original;
			std::string current;
		};

		std::map<RE::FormID, Renamed>& GetRenamed()
		{
			static std::map<RE::FormID, Renamed> renamed;
			return renamed;
		}
	}

	void Rename(RE::TESForm* a_form, std::string_view a_name)
	{
		auto* fullName = a_form ? a_form->As<RE::TESFullName>() : nullptr;
		if (!fullName) {
			return;
		}
		auto&      renamed = GetRenamed();
		const auto found = renamed.find(a_form->GetFormID());
		if (a_name.empty()) {
			if (found != renamed.end()) {
				fullName->fullName = found->second.original;
				renamed.erase(found);
			}
			return;
		}
		if (found == renamed.end()) {
			const char* original = fullName->GetFullName();
			renamed[a_form->GetFormID()] = { original ? original : "", std::string(a_name) };
		} else {
			found->second.current = a_name;
		}
		fullName->fullName = std::string(a_name);
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		const auto& renamed = GetRenamed();
		if (renamed.empty() || !a_intfc->OpenRecord(kNamesRecord, kNamesVersion)) {
			return;
		}
		a_intfc->WriteRecordData(static_cast<std::uint32_t>(renamed.size()));
		for (const auto& [formID, names] : renamed) {
			a_intfc->WriteRecordData(formID);
			a_intfc->WriteRecordData(static_cast<std::uint32_t>(names.current.size()));
			a_intfc->WriteRecordData(names.current.data(), static_cast<std::uint32_t>(names.current.size()));
		}
	}

	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version)
	{
		if (a_type != kNamesRecord) {
			return false;
		}
		std::uint32_t count = 0;
		if (a_version != kNamesVersion || a_intfc->ReadRecordData(count) != sizeof(count)) {
			return true;
		}
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID    formID = 0;
			std::uint32_t length = 0;
			if (a_intfc->ReadRecordData(formID) != sizeof(formID) || a_intfc->ReadRecordData(length) != sizeof(length)) {
				return true;
			}
			std::string name(length, '\0');
			if (length > 0 && a_intfc->ReadRecordData(name.data(), length) != length) {
				return true;
			}
			// A form from a plugin that's gone keeps no name.
			if (a_intfc->ResolveFormID(formID, formID)) {
				Rename(RE::TESForm::LookupByID(formID), name);
			}
		}
		return true;
	}

	void Revert()
	{
		auto& renamed = GetRenamed();
		for (const auto& [formID, names] : renamed) {
			auto* form = RE::TESForm::LookupByID(formID);
			if (auto* fullName = form ? form->As<RE::TESFullName>() : nullptr) {
				fullName->fullName = names.original;
			}
		}
		renamed.clear();
	}
}
