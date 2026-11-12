#include "Events.h"

#include "CampLayout.h"
#include "ScriptObjects.h"

namespace Events
{
	namespace
	{
		constexpr auto kPlugin = "Wayfarer - A Convenient Camping Framework.esp"sv;
		constexpr auto kPluginPrefix = "Wayfarer - A Convenient Camping Framework."sv;

		RE::BSTSmartPointer<RE::BSScript::Object> CampScript()
		{
			static RE::FormID quest = 0;
			return ScriptObjects::FindQuestScript(kPluginPrefix, "LZC_CampsiteQuestScript", quest);
		}

		// The follower quest's script, while the quest is running.
		RE::BSTSmartPointer<RE::BSScript::Object> FollowersScript()
		{
			static RE::FormID quest = 0;
			auto              object = ScriptObjects::FindQuestScript(kPluginPrefix, "LZC_CampFollowersScript", quest);
			auto*             form = quest ? RE::TESForm::LookupByID<RE::TESQuest>(quest) : nullptr;
			return form && form->IsRunning() ? object : nullptr;
		}

		RE::TESForm* Property(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, std::string_view a_name)
		{
			return a_object ? ScriptObjects::FormValue(ScriptObjects::AutoProperty(a_object.get(), a_name)) : nullptr;
		}

		bool FromPlugin(const RE::TESForm* a_form)
		{
			const auto* file = a_form ? a_form->GetFile(0) : nullptr;
			return file && file->GetFilename() == kPlugin;
		}

		// Casting Set Up Camp.
		class SpellCast : public RE::BSTEventSink<RE::TESSpellCastEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override
			{
				if (a_event && a_event->object && a_event->object->IsPlayerRef()) {
					auto camp = CampScript();
					auto* power = Property(camp, "LZC_SetUpCampPower");
					if (power && power->GetFormID() == a_event->spell) {
						ScriptObjects::Call(camp, "HandlePowerCast");
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		// The player activating one of the camp's objects: anything whose base
		// is from Wayfarer - A Convenient Camping Framework (tents, firepit, cook box, clothes basket).
		class Activate : public RE::BSTEventSink<RE::TESActivateEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*) override
			{
				if (a_event && a_event->objectActivated && a_event->actionRef && a_event->actionRef->IsPlayerRef() &&
					FromPlugin(a_event->objectActivated->GetBaseObject())) {
					RE::TESObjectREFR* target = a_event->objectActivated.get();
					logger::info("Activated {:08X} (base {:08X})", target->GetFormID(), target->GetBaseObject() ? target->GetBaseObject()->GetFormID() : 0);
					// A rolled-up bedroll unrolls where its bedroll was laid out.
					if (CampLayout::UnrollKit(target)) {
						return RE::BSEventNotifyControl::kContinue;
					}
					RE::TESObjectREFR* actor = a_event->actionRef.get();
					ScriptObjects::Call(CampScript(), "OnCampObjectActivated", std::move(target), std::move(actor));
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		// TESObjectCONT::Activate (vfunc 0x37): the player activating one of the
		// camp's backpacks (a copy of LZC_CampStorage, placed with the camp)
		// opens LZC_CampStorage itself, which stays in the holding cell with
		// the things in it. Anything else passes.
		struct StorageActivate
		{
			static bool thunk(RE::TESObjectCONT* a_this, RE::TESObjectREFR* a_target, RE::TESObjectREFR* a_activator,
				std::uint8_t a_arg3, RE::TESBoundObject* a_object, std::int32_t a_count)
			{
				auto* player = RE::PlayerCharacter::GetSingleton();
				if (a_target && a_activator && a_activator == player && FromPlugin(a_target->GetBaseObject())) {
					auto        camp = CampScript();
					const auto* value = camp ? ScriptObjects::AutoProperty(camp.get(), "LZC_CampStorage") : nullptr;
					auto*       form = value ? ScriptObjects::FormValue(value) : nullptr;
					auto*       storage = form ? form->As<RE::TESObjectREFR>() : nullptr;
					if (storage && storage != a_target && storage->GetBaseObject() == a_target->GetBaseObject()) {
						const auto handle = storage->GetHandle();
						SKSE::GetTaskInterface()->AddTask([handle] {
							auto ref = handle.get();
							auto* player = RE::PlayerCharacter::GetSingleton();
							if (ref && player) {
								ref->ActivateRef(player, 0, nullptr, 1, false);
							}
						});
						return true;
					}
				}
				return func(a_this, a_target, a_activator, a_arg3, a_object, a_count);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		// The camp's seats: furniture from this plugin, or any furniture in the
		// tub's clutter set (whose seats may be vanilla furniture).
		bool IsCampSeat(const RE::TESBoundObject* a_base)
		{
			if (!a_base) {
				return false;
			}
			if (FromPlugin(a_base)) {
				return true;
			}
			auto        camp = CampScript();
			const auto* value = camp ? ScriptObjects::AutoProperty(camp.get(), "LZC_TubClutter") : nullptr;
			const auto  items = value && value->IsArray() ? value->GetArray() : nullptr;
			for (std::size_t i = 0; items && i < items->size(); ++i) {
				auto* form = ScriptObjects::FormValue(&(*items)[i]);
				auto* item = form ? form->As<RE::TESObjectREFR>() : nullptr;
				if (item && item->GetBaseObject() == a_base) {
					return true;
				}
			}
			return false;
		}

		// Sitting cross-legged on the ground puts the third-person camera in
		// the fixed furniture camera. While the player sits on one of the
		// camp's seats, it's switched back to the ordinary third-person camera
		// with free rotation, so it can be turned round them as on a chair.
		namespace SeatCamera
		{
			std::atomic<bool> seated{ false };

			void Update()
			{
				auto* camera = RE::PlayerCamera::GetSingleton();
				if (!seated || !camera || REL::Module::IsVR() || !camera->IsCurrentState(RE::CameraState::kFurniture, RE::CameraState::kFurniture)) {
					return;
				}
				auto* state = camera->GetRuntimeData().cameraStates[RE::CameraState::kThirdPerson].get();
				if (auto* thirdPerson = skyrim_cast<RE::ThirdPersonState*>(state)) {
					thirdPerson->freeRotationEnabled = true;
					camera->SetState(thirdPerson);
				}
			}

			void Install()
			{
				std::thread([] {
					for (;;) {
						std::this_thread::sleep_for(100ms);
						if (seated) {
							if (auto* tasks = SKSE::GetTaskInterface()) {
								tasks->AddTask(Update);
							}
						}
					}
				}).detach();
			}
		}

		// The player sitting down, for followers to join them (and standing up).
		class ObjectLoaded : public RE::BSTEventSink<RE::TESObjectLoadedEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override
			{
				if (a_event && a_event->loaded) {
					// Next frame, with the 3D in place.
					const auto formID = a_event->formID;
					SKSE::GetTaskInterface()->AddTask([formID] {
						if (auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(formID)) {
							CampLayout::OnObjectLoaded(ref);
						}
					});
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class Furniture : public RE::BSTEventSink<RE::TESFurnitureEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESFurnitureEvent* a_event, RE::BSTEventSource<RE::TESFurnitureEvent>*) override
			{
				if (!a_event || !a_event->actor || !a_event->actor->IsPlayerRef() || !a_event->targetFurniture) {
					return RE::BSEventNotifyControl::kContinue;
				}
				if (a_event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
					SeatCamera::seated = IsCampSeat(a_event->targetFurniture->GetBaseObject());
					if (auto followers = FollowersScript()) {
						RE::TESObjectREFR* furniture = a_event->targetFurniture.get();
						ScriptObjects::Call(followers, "OnPlayerSat", std::move(furniture));
					}
				} else {
					SeatCamera::seated = false;
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void Install()
	{
		auto* events = RE::ScriptEventSourceHolder::GetSingleton();
		if (!events) {
			logger::warn("No script event source; camp events won't be handled");
			return;
		}
		SeatCamera::Install();
		REL::Relocation<std::uintptr_t> containerVtable{ RE::VTABLE_TESObjectCONT[0] };
		StorageActivate::func = containerVtable.write_vfunc(0x37, StorageActivate::thunk);
		static SpellCast spellCast;
		static Activate  activate;
		static Furniture furniture;
		static ObjectLoaded objectLoaded;
		events->AddEventSink<RE::TESObjectLoadedEvent>(&objectLoaded);
		events->AddEventSink<RE::TESSpellCastEvent>(&spellCast);
		events->AddEventSink<RE::TESActivateEvent>(&activate);
		events->AddEventSink<RE::TESFurnitureEvent>(&furniture);
		logger::info("Listening for camp events");
	}
}
