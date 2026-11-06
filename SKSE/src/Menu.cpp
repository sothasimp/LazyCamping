#include "Menu.h"

#include "BedrollKit.h"
#include "VendorKits.h"
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
			int          seatColor{ 0 };
			int          bedrollStyle{ 0 };
			bool         bedrollAltLoaded{ false };  // Bedroll Alternative - BOS Color Variance
			bool         kitsAtMerchants{ true };
			int          firewoodPerFire{ 2 };
			bool         placeMapMarker{ true };
			bool         bedrollsStartRolled{ true };
			bool         placeCampfire{ true };
			bool         placeFireSeats{ true };
			bool         fadeOnSetUp{ false };
			bool         ashNomadLoaded{ false };  // Ash Nomad - Bedrolls - BOS Distributed
			bool         fireOutsideLargeTent{ false };
		};

		constexpr const char* kBedrollStyles[] = { "Vanilla", "Bedroll Alternative - Green", "Bedroll Alternative - Blue",
			"Bedroll Alternative - Red", "Bedroll Alternative - Random Color", "Ash Nomad - Blue", "Ash Nomad - Red", "Ash Nomad - Purple",
			"Ash Nomad - Random Color" };
		constexpr auto        kBedrollAltPlugin = "Bedroll Alternative - BOS Color Variance.esp"sv;
		constexpr auto        kAshNomadBedrollsPlugin = "Ash Nomad - Bedrolls - BOS Distributed.esp"sv;

		bool PluginLoaded(std::string_view a_name)
		{
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			return dataHandler && (dataHandler->LookupLoadedModByName(a_name) || dataHandler->LookupLoadedLightModByName(a_name));
		}

		// The style the bedroll kit's texture follows: only Bedroll Alternative's
		// colours have one.
		int KitStyle(int a_style, bool a_altLoaded) { return a_altLoaded && a_style >= 1 && a_style <= 4 ? a_style : 0; }

		constexpr const char* kSeatColors[] = { "Brown", "White", "Plaid - Green", "Plaid - Yellow", "Plaid - Blue" };

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
			return ScriptObjects::FindQuestScript("Wayfarer - A Convenient Camping Framework."sv, kScript, quest);
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
			auto*  seat = object ? AutoProperty(object.get(), "SeatColor") : nullptr;
			auto*  bedroll = object ? AutoProperty(object.get(), "BedrollStyle") : nullptr;
			auto*  merchants = object ? AutoProperty(object.get(), "KitsAtMerchants") : nullptr;
			auto*  perFire = object ? AutoProperty(object.get(), "FirewoodPerFire") : nullptr;
			auto*  mapMarker = object ? AutoProperty(object.get(), "PlaceMapMarker") : nullptr;
			auto*  rolled = object ? AutoProperty(object.get(), "BedrollsStartRolled") : nullptr;
			auto*  campfire = object ? AutoProperty(object.get(), "PlaceCampfire") : nullptr;
			auto*  fireSeats = object ? AutoProperty(object.get(), "PlaceFireSeats") : nullptr;
			auto*  fade = object ? AutoProperty(object.get(), "FadeOnSetUp") : nullptr;
			auto*  fireOutside = object ? AutoProperty(object.get(), "FireOutsideLargeTent") : nullptr;
			const bool found = firewood && burnHours && block && grass && light && tub && tubInTent && slope && seat && bedroll && merchants && perFire && mapMarker && rolled && campfire && fireSeats && fade && fireOutside;
			if (found) {
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				values = { firewood->GetBool(), burnHours->GetFloat(), block->GetBool(), grass->GetBool(), light->GetBool(), tub->GetBool(),
					tubInTent->GetBool(), slope->GetBool(), seat->GetSInt(), bedroll->GetSInt(),
					PluginLoaded(kBedrollAltPlugin),
					merchants->GetBool(), perFire->GetSInt(), mapMarker->GetBool(), rolled->GetBool(), campfire->GetBool(), fireSeats->GetBool(), fade->GetBool(),
					PluginLoaded(kAshNomadBedrollsPlugin), fireOutside->GetBool() };
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
				if (auto* perFire = AutoProperty(object.get(), "FirewoodPerFire")) {
					perFire->SetSInt(a_values.firewoodPerFire);
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
				if (auto* campfire = AutoProperty(object.get(), "PlaceCampfire")) {
					campfire->SetBool(a_values.placeCampfire);
				}
				if (auto* fireSeats = AutoProperty(object.get(), "PlaceFireSeats")) {
					fireSeats->SetBool(a_values.placeFireSeats);
				}
				if (auto* fireOutside = AutoProperty(object.get(), "FireOutsideLargeTent")) {
					fireOutside->SetBool(a_values.fireOutsideLargeTent);
				}
				if (auto* fade = AutoProperty(object.get(), "FadeOnSetUp")) {
					fade->SetBool(a_values.fadeOnSetUp);
				}
				if (auto* rolled = AutoProperty(object.get(), "BedrollsStartRolled")) {
					rolled->SetBool(a_values.bedrollsStartRolled);
				}
				if (auto* mapMarker = AutoProperty(object.get(), "PlaceMapMarker")) {
					mapMarker->SetBool(a_values.placeMapMarker);
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
				if (auto* seat = AutoProperty(object.get(), "SeatColor")) {
					seat->SetSInt(a_values.seatColor);
				}
				if (auto* bedroll = AutoProperty(object.get(), "BedrollStyle")) {
					bedroll->SetSInt(a_values.bedrollStyle);
				}
				BedrollKit::Apply(object.get(), KitStyle(a_values.bedrollStyle, a_values.bedrollAltLoaded));
				if (auto* merchants = AutoProperty(object.get(), "KitsAtMerchants")) {
					merchants->SetBool(a_values.kitsAtMerchants);
				}
				VendorKits::Apply(a_values.kitsAtMerchants);
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

			bool       changed = false;
			const auto open = ImGuiMCP::ImGuiTreeNodeFlags_DefaultOpen;

			if (ImGuiMCP::CollapsingHeader("Camp", open)) {
				changed |= ImGuiMCP::Checkbox("Align objects to ground slope", &values.alignToSlope);
				ImGuiMCP::SetItemTooltip("Tilt the tents, bedrolls, seats, fire and clutter to match the ground. Takes effect at the next camp.");
				changed |= ImGuiMCP::Checkbox("Clear grass around the camp", &values.hideGrassUnderCamp);
				ImGuiMCP::SetItemTooltip("Clear the grass in a circle around the camp while it's up. Takes effect at the next camp.");
				changed |= ImGuiMCP::Checkbox("Fade to black during setup and pack-up", &values.fadeOnSetUp);
				ImGuiMCP::SetItemTooltip("Fade the screen to black while the camp goes up or is packed up, and back in once it's done.");
				changed |= ImGuiMCP::Checkbox("Map marker", &values.placeMapMarker);
				ImGuiMCP::SetItemTooltip("Show the camp on the world map while it's up, for fast travel. Takes effect at the next camp.");
				changed |= ImGuiMCP::Checkbox("Bathtub", &values.placeTub);
				ImGuiMCP::SetItemTooltip("Place a bathtub of water beside the tents. Takes effect at the next camp.");
				ImGuiMCP::BeginDisabled(!values.placeTub);
				changed |= ImGuiMCP::Checkbox("Bathtub inside the large tent", &values.tubInLargeTent);
				ImGuiMCP::SetItemTooltip("Put the bathtub inside the large tent, in place of its third and fourth bedrolls, so it holds 2 at most. Takes effect at the next camp.");
				ImGuiMCP::EndDisabled();
			}

			if (ImGuiMCP::CollapsingHeader("Bedrolls", open)) {
				changed |= ImGuiMCP::Checkbox("Bedrolls start rolled up", &values.bedrollsStartRolled);
				ImGuiMCP::SetItemTooltip("Each bedroll starts as a rolled-up bedroll on the ground: activate it to unroll it in its place. Followers unroll their own at bedtime. Takes effect at the next camp.");
				// Each mod's styles greyed out unless it's loaded.
				const int styleCount = static_cast<int>(std::size(kBedrollStyles));
				if (ImGuiMCP::BeginCombo("Bedroll style", kBedrollStyles[std::clamp(values.bedrollStyle, 0, styleCount - 1)])) {
					for (int style = 0; style < styleCount; ++style) {
						const bool available = style == 0 || (style <= 4 ? values.bedrollAltLoaded : values.ashNomadLoaded);
						if (ImGuiMCP::Selectable(kBedrollStyles[style], values.bedrollStyle == style, available ? 0 : ImGuiMCP::ImGuiSelectableFlags_Disabled)) {
							values.bedrollStyle = style;
							changed = true;
						}
					}
					ImGuiMCP::EndCombo();
				}
				ImGuiMCP::SetItemTooltip("The camp's bedrolls: vanilla, or with Bedroll Alternative - BOS Color Variance or Ash Nomad - Bedrolls - BOS Distributed installed, one of its colors, or its colors in turn, pairs in a tent matching. Takes effect at the next camp.");
			}

			if (ImGuiMCP::CollapsingHeader("Fire", open)) {
				changed |= ImGuiMCP::Checkbox("Firepit", &values.placeCampfire);
				ImGuiMCP::SetItemTooltip("Place the campfire with its clutter, light and cooking. Without it there's no firewood mechanic or chopping block either. Takes effect at the next camp.");
				ImGuiMCP::BeginDisabled(!values.placeCampfire);
				changed |= ImGuiMCP::Checkbox("Fire outside the large tent", &values.fireOutsideLargeTent);
				ImGuiMCP::SetItemTooltip("Stand the large tent 285 units further back, so the fire is in front of it rather than under it. Takes effect at the next camp.");
				changed |= ImGuiMCP::Checkbox("Light marker", &values.placeCampfireLight);
				ImGuiMCP::SetItemTooltip("Place a light with the campfire. Turn off if a Light Placer config already lights it. Takes effect at the next camp.");
				changed |= ImGuiMCP::Checkbox("Firewood mechanic", &values.firewoodMechanic);
				ImGuiMCP::SetItemTooltip("The fire starts out unlit: activate it with firewood to light it. Takes effect at the next camp.");
				ImGuiMCP::BeginDisabled(!values.firewoodMechanic);
				changed |= ImGuiMCP::SliderFloat("Fire burn time", &values.fireBurnHours, 1.0f, 24.0f, "%.0f hours");
				ImGuiMCP::SetItemTooltip("How long a fire lit with firewood burns, in game hours. Takes effect the next time it's lit.");
				changed |= ImGuiMCP::SliderInt("Firewood to light the fire", &values.firewoodPerFire, 1, 6);
				ImGuiMCP::SetItemTooltip("How many firewood lighting the fire uses up.");
				changed |= ImGuiMCP::Checkbox("Wood chopping block", &values.placeChoppingBlock);
				ImGuiMCP::SetItemTooltip("Place a woodcutting block, for chopping firewood. Needs the firewood mechanic. Takes effect at the next camp.");
				ImGuiMCP::EndDisabled();
				ImGuiMCP::EndDisabled();
				changed |= ImGuiMCP::Checkbox("Fireside cushions", &values.placeFireSeats);
				ImGuiMCP::SetItemTooltip("Place the seats round the fire, the player's and the followers'. The seats on the bedrolls stay either way. Takes effect at the next camp.");
				ImGuiMCP::BeginDisabled(!values.placeFireSeats);
				changed |= ImGuiMCP::Combo("Cushion color", &values.seatColor, kSeatColors, static_cast<int>(std::size(kSeatColors)));
				ImGuiMCP::SetItemTooltip("The color of the fireside cushions, matching the tents. Takes effect at the next camp.");
				ImGuiMCP::EndDisabled();
			}

			if (ImGuiMCP::CollapsingHeader("Leveled lists", open)) {
				changed |= ImGuiMCP::Checkbox("Sell kits at general goods merchants", &values.kitsAtMerchants);
				ImGuiMCP::SetItemTooltip("Merchants can stock the bedroll and tent kits. Shows at their next restock.");
			}

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

	void ApplyLoadedSettings()
	{
		auto  object = FindScript();
		auto* merchants = object ? AutoProperty(object.get(), "KitsAtMerchants") : nullptr;
		VendorKits::Apply(!merchants || merchants->GetBool());

		auto* bedroll = object ? AutoProperty(object.get(), "BedrollStyle") : nullptr;
		BedrollKit::Apply(object.get(), bedroll ? KitStyle(bedroll->GetSInt(), PluginLoaded(kBedrollAltPlugin)) : 0);
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			return;
		}
		SKSEMenuFramework::SetSection("Convenient Camping (Wayfarer)");
		SKSEMenuFramework::AddSectionItem("Settings", Render);
		logger::info("Added the SKSE Menu Framework page");
	}
}
