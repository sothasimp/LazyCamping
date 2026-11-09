#include "BedrollSeats.h"

namespace BedrollSeats
{
	namespace
	{
		constexpr std::uint32_t kSeatsRecord = 'SEAT';
		constexpr std::uint32_t kSeatsVersion = 1;

		// Bedroll -> its seat. Main thread only.
		std::map<RE::FormID, RE::FormID> seats;

		// Each seat's own collision layer, seen before it was hidden.
		std::map<RE::FormID, RE::COL_LAYER> seatLayers;
		std::atomic<bool>                   anySeats{ false };

		// The seats can only be targeted (and so activated by the player
		// directly) while the player sneaks: otherwise their collision is off,
		// and the crosshair finds the bedroll under them. Followers' AI uses
		// them either way. Checked a few times a second, which also catches a
		// seat's 3D loading again.
		void UpdateSeatTargets()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}
			const bool sneaking = player->IsSneaking();
			for (const auto& [bed, seatID] : seats) {
				auto* seat = RE::TESForm::LookupByID<RE::TESObjectREFR>(seatID);
				auto* root = seat ? seat->Get3D() : nullptr;
				if (!root) {
					continue;
				}
				const auto layer = root->GetCollisionLayer();
				if (layer != RE::COL_LAYER::kNonCollidable && layer != RE::COL_LAYER::kUnidentified) {
					seatLayers[seatID] = layer;
				}
				const auto own = seatLayers.find(seatID);
				if (own == seatLayers.end()) {
					continue;  // no collision of its own to switch
				}
				const auto wanted = sneaking ? own->second : RE::COL_LAYER::kNonCollidable;
				if (layer != wanted) {
					root->SetCollisionLayer(wanted);
				}
			}
		}

		void Sit(RE::FormID a_seat)
		{
			auto* seat = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_seat);
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (seat && player && !seat->IsDisabled()) {
				seat->ActivateRef(player, 0, nullptr, 1, false);
			}
		}

		// The game's own way of opening the Sleep/Wait menu, as a bed does
		// (a_sleep) or the Wait key does.
		void OpenSleepWaitMenu(bool a_sleep)
		{
			using func_t = void (*)(bool);
			static REL::Relocation<func_t> func{ RELOCATION_ID(51618, 52490) };
			func(a_sleep);
		}

		bool PlayerOnCampSeat()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			const auto furniture = player ? player->GetOccupiedFurniture().get() : nullptr;
			if (!furniture) {
				return false;
			}
			const auto id = furniture->GetFormID();
			return std::ranges::any_of(seats, [&](const auto& a_pair) { return a_pair.second == id; });
		}

		// The Wait menu opening while the player lies on a camp bedroll's seat
		// is closed and opened again as the Sleep menu.
		class SleepMenuRedirect : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (!a_event || !a_event->opening || a_event->menuName != RE::SleepWaitMenu::MENU_NAME) {
					return RE::BSEventNotifyControl::kContinue;
				}
				if (reopening.exchange(false)) {
					return RE::BSEventNotifyControl::kContinue;  // the one opened below
				}
				SKSE::GetTaskInterface()->AddTask([] {
					if (!PlayerOnCampSeat()) {
						return;
					}
					if (auto* queue = RE::UIMessageQueue::GetSingleton()) {
						queue->AddMessage(RE::SleepWaitMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
					}
					SKSE::GetTaskInterface()->AddTask([] {
						reopening = true;
						OpenSleepWaitMenu(true);
					});
				});
				return RE::BSEventNotifyControl::kContinue;
			}

			static inline std::atomic<bool> reopening{ false };
		};

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

		std::thread([] {
			for (;;) {
				std::this_thread::sleep_for(250ms);
				if (anySeats) {
					if (auto* tasks = SKSE::GetTaskInterface()) {
						tasks->AddTask(UpdateSeatTargets);
					}
				}
			}
		}).detach();
	}

	void InstallSleepMenu()
	{
		static SleepMenuRedirect redirect;
		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&redirect);
		}
	}

	void Set(const std::vector<RE::TESObjectREFR*>& a_beds, const std::vector<RE::TESObjectREFR*>& a_seats)
	{
		seats.clear();
		for (std::size_t i = 0; i < a_beds.size() && i < a_seats.size(); ++i) {
			if (a_beds[i] && a_seats[i]) {
				seats[a_beds[i]->GetFormID()] = a_seats[i]->GetFormID();
			}
		}
		seatLayers.clear();
		anySeats = !seats.empty();
		UpdateSeatTargets();
	}

	void Clear()
	{
		seats.clear();
		seatLayers.clear();
		anySeats = false;
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
		anySeats = !seats.empty();
		return true;
	}

	void Revert()
	{
		seats.clear();
		seatLayers.clear();
		anySeats = false;
	}
}
