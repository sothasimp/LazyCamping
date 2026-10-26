#include "Menu.h"

#include "ScriptObjects.h"

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
			bool         firewoodMechanic{ false };
			float        fireBurnHours{ 6.0f };
			bool         fadeToBlack{ true };
			bool         hideGrassUnderCamp{ true };
			bool         placeCampfireLight{ true };
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
			static RE::FormID quest = 0;
			return ScriptObjects::FindQuestScript("LazyCamping."sv, kScript, quest);
		}

		using ScriptObjects::AutoProperty;

		void Refresh()
		{
			refreshQueued = false;
			Values values;
			auto   object = FindScript();
			auto*  firewood = object ? AutoProperty(object.get(), "FirewoodMechanic") : nullptr;
			auto*  burnHours = object ? AutoProperty(object.get(), "FireBurnHours") : nullptr;
			auto*  fade = object ? AutoProperty(object.get(), "FadeToBlack") : nullptr;
			auto*  grass = object ? AutoProperty(object.get(), "HideGrassUnderCamp") : nullptr;
			auto*  light = object ? AutoProperty(object.get(), "PlaceCampfireLight") : nullptr;
			const bool found = firewood && burnHours && fade && grass && light;
			if (found) {
				values = { firewood->GetBool(), burnHours->GetFloat(), fade->GetBool(), grass->GetBool(), light->GetBool() };
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
				if (auto* firewood = AutoProperty(object.get(), "FirewoodMechanic")) {
					firewood->SetBool(a_values.firewoodMechanic);
				}
				if (auto* burnHours = AutoProperty(object.get(), "FireBurnHours")) {
					burnHours->SetFloat(a_values.fireBurnHours);
				}
				if (auto* fade = AutoProperty(object.get(), "FadeToBlack")) {
					fade->SetBool(a_values.fadeToBlack);
				}
				if (auto* grass = AutoProperty(object.get(), "HideGrassUnderCamp")) {
					grass->SetBool(a_values.hideGrassUnderCamp);
				}
				if (auto* light = AutoProperty(object.get(), "PlaceCampfireLight")) {
					light->SetBool(a_values.placeCampfireLight);
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
			changed |= ImGuiMCP::Checkbox("Firewood mechanic", &values.firewoodMechanic);
			ImGuiMCP::SetItemTooltip("The fire starts out unlit: sneak-activate it with firewood to light it. Adds a chopping block. Takes effect at the next camp.");
			ImGuiMCP::BeginDisabled(!values.firewoodMechanic);
			changed |= ImGuiMCP::SliderFloat("Fire burn time", &values.fireBurnHours, 1.0f, 24.0f, "%.0f hours");
			ImGuiMCP::SetItemTooltip("How long a fire lit with firewood burns, in game hours. Takes effect the next time it's lit.");
			ImGuiMCP::EndDisabled();
			changed |= ImGuiMCP::Checkbox("Fade to black", &values.fadeToBlack);
			ImGuiMCP::SetItemTooltip("Fade the screen to black while the camp is set up or packed up.");
			changed |= ImGuiMCP::Checkbox("Clear grass around the camp", &values.hideGrassUnderCamp);
			ImGuiMCP::SetItemTooltip("Clear the grass in a circle around the camp while it's up. Takes effect at the next camp.");
			changed |= ImGuiMCP::Checkbox("Campfire light marker", &values.placeCampfireLight);
			ImGuiMCP::SetItemTooltip("Place a light with the campfire. Turn off if a Light Placer config already lights it. Takes effect at the next camp.");

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
