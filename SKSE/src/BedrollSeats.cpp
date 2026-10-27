#include "BedrollSeats.h"

namespace BedrollSeats
{
	namespace
	{
		constexpr std::uint32_t kSeatsRecord = 'SEAT';
		constexpr std::uint32_t kSeatsVersion = 1;

		// Bedroll -> its seat. Main thread only.
		std::map<RE::FormID, RE::FormID> seats;

		void Sit(RE::FormID a_seat)
		{
			auto* seat = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_seat);
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (seat && player && !seat->IsDisabled()) {
				seat->ActivateRef(player, 0, nullptr, 1, false);
			}
		}

		// TESFurniture::Activate (vfunc 0x37): for every furniture, so pass
		// anything that isn't the player sneak-activating a camp bedroll.
		struct FurnitureActivate
		{
			static bool thunk(RE::TESFurniture* a_this, RE::TESObjectREFR* a_target, RE::TESObjectREFR* a_activator,
				std::uint8_t a_arg3, RE::TESBoundObject* a_object, std::int32_t a_count)
			{
				auto* player = RE::PlayerCharacter::GetSingleton();
				if (a_target && a_activator && a_activator == player && player->IsSneaking()) {
					if (const auto found = seats.find(a_target->GetFormID()); found != seats.end()) {
						// Sit after this activation has finished, not inside it.
						const auto seat = found->second;
						SKSE::GetTaskInterface()->AddTask([seat] { Sit(seat); });
						return true;
					}
				}
				return func(a_this, a_target, a_activator, a_arg3, a_object, a_count);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_TESFurniture[0] };
		FurnitureActivate::func = vtable.write_vfunc(0x37, FurnitureActivate::thunk);
	}

	void Set(const std::vector<RE::TESObjectREFR*>& a_beds, const std::vector<RE::TESObjectREFR*>& a_seats)
	{
		seats.clear();
		for (std::size_t i = 0; i < a_beds.size() && i < a_seats.size(); ++i) {
			if (a_beds[i] && a_seats[i]) {
				seats[a_beds[i]->GetFormID()] = a_seats[i]->GetFormID();
			}
		}
	}

	void Clear()
	{
		seats.clear();
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (seats.empty() || !a_intfc->OpenRecord(kSeatsRecord, kSeatsVersion)) {
			return;
		}
		a_intfc->WriteRecordData(static_cast<std::uint32_t>(seats.size()));
		for (const auto& [bed, seat] : seats) {
			a_intfc->WriteRecordData(bed);
			a_intfc->WriteRecordData(seat);
		}
	}

	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version)
	{
		if (a_type != kSeatsRecord) {
			return false;
		}
		std::uint32_t count = 0;
		if (a_version != kSeatsVersion || a_intfc->ReadRecordData(count) != sizeof(count)) {
			return true;
		}
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID bed = 0;
			RE::FormID seat = 0;
			if (a_intfc->ReadRecordData(bed) != sizeof(bed) || a_intfc->ReadRecordData(seat) != sizeof(seat)) {
				break;
			}
			if (a_intfc->ResolveFormID(bed, bed) && a_intfc->ResolveFormID(seat, seat)) {
				seats[bed] = seat;
			}
		}
		return true;
	}

	void Revert()
	{
		seats.clear();
	}
}
