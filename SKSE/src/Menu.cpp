#include "Menu.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "SKSEMenuFramework.h"

// The settings live on LZC_CampsiteQuestScript, in the save. The menu draws
// on the render thread, so it shows a copy of them: the copy is refreshed
// from the script on the main thread every second while the page is open, and
// changes go to the script the same way.
namespace Menu
{
	namespace
	{
		constexpr auto kScript = "LZC_CampsiteQuestScript";

		struct Values
		{
			std::int32_t firewoodRequired{ 0 };
			bool         fadeToBlack{ true };
			bool         hideGrassUnderCamp{ true };
		};

		std::mutex                            lock;
		Values                                shown;
		bool                                  loaded{ false };
		std::atomic<int>                      pendingWrites{ 0 };
		std::atomic<bool>                     refreshQueued{ false };
		std::chrono::steady_clock::time_point lastRefresh;

		// The campsite quest's script, if a game is loaded. Main thread only.
		RE::BSTSmartPointer<RE::BSScript::Object> FindScript()
		{
			static RE::FormID questId = 0;

			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
			if (!policy) {
				return {};
			}
			const auto bound = [&](RE::TESQuest* a_quest) {
				RE::BSTSmartPointer<RE::BSScript::Object> object;
				const auto handle = policy->GetHandleForObject(a_quest->GetFormType(), a_quest);
				if (handle != policy->EmptyHandle()) {
					vm->FindBoundObject(handle, kScript, object);
				}
				return object;
			};

			if (auto* quest = questId ? RE::TESForm::LookupByID<RE::TESQuest>(questId) : nullptr) {
				return bound(quest);
			}
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				return {};
			}
			for (auto* quest : dataHandler->GetFormArray<RE::TESQuest>()) {
				const auto* file = quest ? quest->GetFile(0) : nullptr;
				if (!file || !file->GetFilename().starts_with("LazyCamping."sv)) {
					continue;
				}
				if (auto object = bound(quest)) {
					questId = quest->GetFormID();
					return object;
				}
			}
			return {};
		}

		RE::BSScript::Variable* AutoProperty(RE::BSScript::Object* a_object, std::string_view a_name)
		{
			// An Auto property's value is in a hidden variable named ::Name_var.
			return a_object->GetVariable("::" + std::string(a_name) + "_var");
		}

		void Refresh()
		{
			refreshQueued = false;
			Values values;
			auto   object = FindScript();
			auto*  firewood = object ? AutoProperty(object.get(), "FirewoodRequired") : nullptr;
			auto*  fade = object ? AutoProperty(object.get(), "FadeToBlack") : nullptr;
			auto*  grass = object ? AutoProperty(object.get(), "HideGrassUnderCamp") : nullptr;
			const bool found = firewood && fade && grass;
			if (found) {
				values = { firewood->GetSInt(), fade->GetBool(), grass->GetBool() };
			}

			std::scoped_lock guard(lock);
			// A change still on its way would be undone by the old values.
			if (pendingWrites == 0) {
				shown = values;
				loaded = found;
			}
		}

		void Write(Values a_values)
		{
			if (auto object = FindScript()) {
				if (auto* firewood = AutoProperty(object.get(), "FirewoodRequired")) {
					firewood->SetSInt(a_values.firewoodRequired);
				}
				if (auto* fade = AutoProperty(object.get(), "FadeToBlack")) {
					fade->SetBool(a_values.fadeToBlack);
				}
				if (auto* grass = AutoProperty(object.get(), "HideGrassUnderCamp")) {
					grass->SetBool(a_values.hideGrassUnderCamp);
				}
			}
			--pendingWrites;
		}

		void RequestRefresh()
		{
			const auto now = std::chrono::steady_clock::now();
			if (now - lastRefresh < 1s || refreshQueued.exchange(true)) {
				return;
			}
			lastRefresh = now;
			SKSE::GetTaskInterface()->AddTask(Refresh);
		}

		void __stdcall Render()
		{
			RequestRefresh();

			Values values;
			bool   ready = false;
			{
				std::scoped_lock guard(lock);
				values = shown;
				ready = loaded;
			}
			if (!ready) {
				ImGuiMCP::TextDisabled("Load a game to change these settings.");
				return;
			}

			bool changed = false;
			changed |= ImGuiMCP::SliderInt("Firewood required", &values.firewoodRequired, 0, 10);
			ImGuiMCP::SetItemTooltip("Firewood used up each time a camp is set up. 0 turns the requirement off.");
			changed |= ImGuiMCP::Checkbox("Fade to black", &values.fadeToBlack);
			ImGuiMCP::SetItemTooltip("Fade the screen to black while the camp is set up or packed up.");
			changed |= ImGuiMCP::Checkbox("Clear grass around the camp", &values.hideGrassUnderCamp);
			ImGuiMCP::SetItemTooltip("Clear the grass in a circle around the camp while it's up. Takes effect at the next camp.");

			if (changed) {
				{
					std::scoped_lock guard(lock);
					shown = values;
				}
				++pendingWrites;
				SKSE::GetTaskInterface()->AddTask([values] { Write(values); });
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			return;
		}
		SKSEMenuFramework::SetSection("Lazy Camping");
		SKSEMenuFramework::AddSectionItem("Settings", Render);
		logger::info("Added the SKSE Menu Framework page");
	}
}
