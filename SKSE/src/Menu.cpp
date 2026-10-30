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
			bool         placeChoppingBlock{ true };
			bool         hideGrassUnderCamp{ true };
			bool         placeCampfireLight{ true };
			bool         placeTub{ true };
			bool         tubInLargeTent{ false };
			bool         alignToSlope{ true };
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
			auto*  block = object ? AutoProperty(object.get(), "PlaceChoppingBlock") : nullptr;
			auto*  grass = object ? AutoProperty(object.get(), "HideGrassUnderCamp") : nullptr;
			auto*  light = object ? AutoProperty(object.get(), "PlaceCampfireLight") : nullptr;
			auto*  tub = object ? AutoProperty(object.get(), "PlaceTub") : nullptr;
			auto*  tubInTent = object ? AutoProperty(object.get(), "TubInLargeTent") : nullptr;
			auto*  slope = object ? AutoProperty(object.get(), "AlignToSlope") : nullptr;
			const bool found = firewood && burnHours && block && grass && light && tub && tubInTent && slope;
			if (found) {
				values = { firewood->GetBool(), burnHours->GetFloat(), block->GetBool(), grass->GetBool(), light->GetBool(), tub->GetBool(),
					tubInTent->GetBool(), slope->GetBool() };
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
				if (auto* block = AutoProperty(object.get(), "PlaceChoppingBlock")) {
					block->SetBool(a_values.placeChoppingBlock);
				}
				if (auto* grass = AutoProperty(object.get(), "HideGrassUnderCamp")) {
					grass->SetBool(a_values.hideGrassUnderCamp);
				}
				if (auto* light = AutoProperty(object.get(), "PlaceCampfireLight")) {
					light->SetBool(a_values.placeCampfireLight);
				}
				if (auto* tub = AutoProperty(object.get(), "PlaceTub")) {
					tub->SetBool(a_values.placeTub);
				}
				if (auto* tubInTent = AutoProperty(object.get(), "TubInLargeTent")) {
					tubInTent->SetBool(a_values.tubInLargeTent);
				}
				if (auto* slope = AutoProperty(object.get(), "AlignToSlope")) {
					slope->SetBool(a_values.alignToSlope);
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
			ImGuiMCP::SetItemTooltip("The fire starts out unlit: activate it with firewood to light it. Takes effect at the next camp.");
			ImGuiMCP::BeginDisabled(!values.firewoodMechanic);
			changed |= ImGuiMCP::SliderFloat("Fire burn time", &values.fireBurnHours, 1.0f, 24.0f, "%.0f hours");
			ImGuiMCP::SetItemTooltip("How long a fire lit with firewood burns, in game hours. Takes effect the next time it's lit.");
			changed |= ImGuiMCP::Checkbox("Wood chopping block", &values.placeChoppingBlock);
			ImGuiMCP::SetItemTooltip("Place a woodcutting block beside the tents, for chopping firewood. Takes effect at the next camp.");
			ImGuiMCP::EndDisabled();
			changed |= ImGuiMCP::Checkbox("Align to slope", &values.alignToSlope);
			ImGuiMCP::SetItemTooltip("Tilt the tents, bedrolls, seats, fire and clutter to match the ground. Takes effect at the next camp.");
			changed |= ImGuiMCP::Checkbox("Clear grass around the camp", &values.hideGrassUnderCamp);
			ImGuiMCP::SetItemTooltip("Clear the grass in a circle around the camp while it's up. Takes effect at the next camp.");
			changed |= ImGuiMCP::Checkbox("Campfire light marker", &values.placeCampfireLight);
			ImGuiMCP::SetItemTooltip("Place a light with the campfire. Turn off if a Light Placer config already lights it. Takes effect at the next camp.");
			changed |= ImGuiMCP::Checkbox("Bathtub", &values.placeTub);
			ImGuiMCP::SetItemTooltip("Place a bathtub of water beside the tents. Takes effect at the next camp.");
			ImGuiMCP::BeginDisabled(!values.placeTub);
			changed |= ImGuiMCP::Checkbox("Bathtub in the large tent", &values.tubInLargeTent);
			ImGuiMCP::SetItemTooltip("Put the bathtub inside the large tent, in place of its third and fourth bedrolls, so it holds 2 at most. Takes effect at the next camp.");
			ImGuiMCP::EndDisabled();

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
