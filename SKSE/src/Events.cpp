#include "Events.h"

#include "ScriptObjects.h"

namespace Events
{
	namespace
	{
		constexpr auto kPlugin = "LazyCamping.esp"sv;
		constexpr auto kPluginPrefix = "LazyCamping."sv;

		// A follower counts as in the tub within this far of the tub trigger,
		// sideways and up or down.
		constexpr float kTubReach = 72.0f;
		constexpr float kTubHeight = 100.0f;

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
		// is from Lazy Camping (tents, firepit, cook box, clothes basket).
		class Activate : public RE::BSTEventSink<RE::TESActivateEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*) override
			{
				if (a_event && a_event->objectActivated && a_event->actionRef && a_event->actionRef->IsPlayerRef() &&
					FromPlugin(a_event->objectActivated->GetBaseObject())) {
					RE::TESObjectREFR* target = a_event->objectActivated.get();
					RE::TESObjectREFR* actor = a_event->actionRef.get();
					ScriptObjects::Call(CampScript(), "OnCampObjectActivated", std::move(target), std::move(actor));
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		// The player sitting down, for followers to join them.
		class Furniture : public RE::BSTEventSink<RE::TESFurnitureEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESFurnitureEvent* a_event, RE::BSTEventSource<RE::TESFurnitureEvent>*) override
			{
				if (a_event && a_event->actor && a_event->actor->IsPlayerRef() && a_event->targetFurniture &&
					a_event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter) {
					if (auto followers = FollowersScript()) {
						RE::TESObjectREFR* furniture = a_event->targetFurniture.get();
						ScriptObjects::Call(followers, "OnPlayerSat", std::move(furniture));
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		// Followers in the tub: tells the camp script when one steps in or out.
		std::set<RE::FormID> inTub;

		void CheckTub()
		{
			auto  camp = CampScript();
			auto* form = camp ? Property(camp, "LZC_TubTrigger") : nullptr;
			auto* tub = form ? form->As<RE::TESObjectREFR>() : nullptr;  // the trigger, on the tub
			auto* deployed = camp ? ScriptObjects::AutoProperty(camp.get(), "CampDeployed") : nullptr;
			const bool active = tub && !tub->IsDisabled() && deployed && deployed->IsBool() && deployed->GetBool();

			std::set<RE::FormID> now;
			std::vector<RE::Actor*> entered;
			if (active) {
				if (auto* processes = RE::ProcessLists::GetSingleton()) {
					const auto centre = tub->GetPosition();
					for (auto& handle : processes->highActorHandles) {
						auto  actorPtr = handle.get();
						auto* actor = actorPtr.get();
						if (!actor || actor->IsPlayerRef() || !actor->IsPlayerTeammate() || actor->IsDead()) {
							continue;
						}
						const auto position = actor->GetPosition();
						const float dx = position.x - centre.x;
						const float dy = position.y - centre.y;
						if (dx * dx + dy * dy <= kTubReach * kTubReach && std::abs(position.z - centre.z) <= kTubHeight) {
							now.insert(actor->GetFormID());
							if (!inTub.contains(actor->GetFormID())) {
								entered.push_back(actor);
							}
						}
					}
				}
			}
			for (auto id : inTub) {
				if (!now.contains(id)) {
					if (auto* actor = RE::TESForm::LookupByID<RE::TESObjectREFR>(id)) {
						RE::TESObjectREFR* triggerRef = tub;
						ScriptObjects::Call(camp, "OnCampTrigger", std::move(triggerRef), std::move(actor), false);
					}
				}
			}
			for (auto* actor : entered) {
				RE::TESObjectREFR* triggerRef = tub;
				RE::TESObjectREFR* actorRef = actor;
				ScriptObjects::Call(camp, "OnCampTrigger", std::move(triggerRef), std::move(actorRef), true);
			}
			inTub = std::move(now);
		}
	}

	void Install()
	{
		auto* events = RE::ScriptEventSourceHolder::GetSingleton();
		if (!events) {
			logger::warn("No script event source; camp events won't be handled");
			return;
		}
		static SpellCast spellCast;
		static Activate  activate;
		static Furniture furniture;
		events->AddEventSink<RE::TESSpellCastEvent>(&spellCast);
		events->AddEventSink<RE::TESActivateEvent>(&activate);
		events->AddEventSink<RE::TESFurnitureEvent>(&furniture);

		std::thread([] {
			for (;;) {
				std::this_thread::sleep_for(1s);
				if (auto* tasks = SKSE::GetTaskInterface()) {
					tasks->AddTask(CheckTub);
				}
			}
		}).detach();
		logger::info("Listening for camp events");
	}
}
