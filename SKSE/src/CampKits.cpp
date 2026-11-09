#include "CampKits.h"

namespace CampKits
{
	namespace
	{
		constexpr std::uint32_t kRecord = 'KITS';
		constexpr std::uint32_t kVersion = 1;

		struct Taken
		{
			RE::FormID   container;
			RE::FormID   kit;
			std::int32_t count;
		};

		std::vector<Taken> taken;  // main thread only
	}

	std::int32_t Take(RE::TESObjectREFR* a_container, RE::TESBoundObject* a_kit, std::int32_t a_count)
	{
		if (!a_container || !a_kit || a_count <= 0) {
			return 0;
		}
		const auto counts = a_container->GetInventoryCounts();
		const auto have = counts.contains(a_kit) ? counts.at(a_kit) : 0;
		const auto count = std::min(a_count, have);
		if (count <= 0) {
			return 0;
		}
		a_container->RemoveItem(a_kit, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		taken.push_back({ a_container->GetFormID(), a_kit->GetFormID(), count });
		logger::info("Kits: took {} x {} from {:08X}", count, a_kit->GetName(), a_container->GetFormID());
		return count;
	}

	void ReturnAll()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		for (const auto& entry : taken) {
			auto* kit = RE::TESForm::LookupByID<RE::TESBoundObject>(entry.kit);
			auto* container = RE::TESForm::LookupByID<RE::TESObjectREFR>(entry.container);
			auto* actor = container ? container->As<RE::Actor>() : nullptr;
			if (!container || container->IsDeleted() || (actor && actor->IsDead())) {
				container = player;
			}
			if (kit && container) {
				container->AddObjectToContainer(kit, nullptr, entry.count, nullptr);
				logger::info("Kits: gave back {} x {} to {:08X}", entry.count, kit->GetName(), container->GetFormID());
			}
		}
		taken.clear();
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (taken.empty() || !a_intfc->OpenRecord(kRecord, kVersion)) {
			return;
		}
		a_intfc->WriteRecordData(static_cast<std::uint32_t>(taken.size()));
		for (const auto& entry : taken) {
			a_intfc->WriteRecordData(entry.container);
			a_intfc->WriteRecordData(entry.kit);
			a_intfc->WriteRecordData(entry.count);
		}
	}

	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version)
	{
		if (a_type != kRecord) {
			return false;
		}
		std::uint32_t size = 0;
		if (a_version != kVersion || a_intfc->ReadRecordData(size) != sizeof(size)) {
			return true;
		}
		for (std::uint32_t i = 0; i < size; ++i) {
			Taken entry{};
			if (a_intfc->ReadRecordData(entry.container) != sizeof(entry.container) || a_intfc->ReadRecordData(entry.kit) != sizeof(entry.kit) ||
				a_intfc->ReadRecordData(entry.count) != sizeof(entry.count)) {
				break;
			}
			if (a_intfc->ResolveFormID(entry.container, entry.container) && a_intfc->ResolveFormID(entry.kit, entry.kit)) {
				taken.push_back(entry);
			}
		}
		return true;
	}

	void Revert()
	{
		taken.clear();
	}
}
