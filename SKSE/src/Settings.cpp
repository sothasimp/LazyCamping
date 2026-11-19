#include "Settings.h"

#include "Menu.h"
#include "ScriptObjects.h"

#include <SimpleIni.h>

namespace Settings
{
	namespace
	{
		constexpr auto kPath = "Data/SKSE/Plugins/LazyCamping.ini";

		// Missing or invalid keys leave the quest's original property value intact.
		template <class T>
		std::optional<T> ReadValue(const CSimpleIniA& a_ini, const char* a_section, const char* a_key, T a_min, T a_max)
		{
			const auto* text = a_ini.GetValue(a_section, a_key);
			if (!text) {
				return std::nullopt;
			}
			auto value = detail::ParseValue<T>(text, a_min, a_max);
			if (!value) {
				logger::warn("{}: invalid [{}] {}='{}'; keeping the original default", kPath, a_section, a_key, text);
			}
			return value;
		}

		template <class T>
		void ApplyValue(const CSimpleIniA& a_ini, RE::BSScript::Object* a_object, const char* a_section, const char* a_key, T a_min, T a_max)
		{
			if (auto value = ReadValue<T>(a_ini, a_section, a_key, a_min, a_max)) {
				if (auto* property = ScriptObjects::AutoProperty(a_object, a_key)) {
					if constexpr (std::is_same_v<T, bool>) {
						property->SetBool(*value);
					} else if constexpr (std::is_floating_point_v<T>) {
						property->SetFloat(*value);
					} else {
						property->SetSInt(*value);
					}
				} else {
					logger::warn("INI defaults: quest property {} is missing; check the installed campsite script", a_key);
				}
			}
		}
	}

	void ApplyDefaults(RE::TESQuest* a_quest)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
		if (!a_quest || !policy) {
			logger::error("INI defaults: the campsite quest or VM is unavailable");
			return;
		}
		RE::BSTSmartPointer<RE::BSScript::Object> object;
		vm->FindBoundObject(policy->GetHandleForObject(a_quest->GetFormType(), a_quest), "LZC_CampsiteQuestScript", object);
		if (!object) {
			logger::error("INI defaults: the campsite quest script is unavailable");
			return;
		}

		CSimpleIniA ini;
		ini.SetUnicode();
		if (ini.LoadFile(kPath) < 0) {
			logger::info("{} is missing or unreadable; keeping the original defaults", kPath);
		} else {
			constexpr std::pair<const char*, const char*> boolSettings[] = {
				{ "Camp", "AlignToSlope" },
				{ "Camp", "HideGrassUnderCamp" },
				{ "Camp", "FadeOnSetUp" },
				{ "Camp", "TentCollision" },
				{ "Camp", "PlaceTentLanterns" },
				{ "Camp", "PlaceMapMarker" },
				{ "Camp", "PlaceTub" },
				{ "Camp", "TubInLargeTent" },
				{ "Bedrolls", "BedrollsStartRolled" },
				{ "Fire", "PlaceCampfire" },
				{ "Fire", "FireOutsideLargeTent" },
				{ "Fire", "CampfireLightShadows" },
				{ "Fire", "PlaceCookpot" },
				{ "Fire", "FirewoodMechanic" },
				{ "Fire", "PlaceChoppingBlock" },
				{ "Fire", "PlaceFireSeats" },
				{ "LeveledLists", "KitsAtMerchants" }
			};
			for (const auto& [section, key] : boolSettings) {
				ApplyValue<bool>(ini, object.get(), section, key, false, true);
			}
			ApplyValue<int>(ini, object.get(), "Bedrolls", "BedrollStyle", 0, 8);
			ApplyValue<int>(ini, object.get(), "Fire", "SeatColor", 0, 4);
			ApplyValue<int>(ini, object.get(), "Fire", "FirewoodPerFire", 1, 6);
			ApplyValue<float>(ini, object.get(), "Fire", "FireBurnHours", 1.0f, 24.0f);

			// One menu slider controls both the enabled flag and the legacy brightness value.
			if (auto value = ReadValue<int>(ini, "Fire", "CampfireLight", 0, 3)) {
				auto* enabled = ScriptObjects::AutoProperty(object.get(), "PlaceCampfireLight");
				auto* level = ScriptObjects::AutoProperty(object.get(), "CampfireLightLevel");
				if (enabled && level) {
					enabled->SetBool(*value > 0);
					if (*value > 0) {
						level->SetSInt(StoredLightLevel(*value));
					}
				} else {
					logger::warn("INI defaults: campfire light properties are missing; check the installed campsite script");
				}
			}
			logger::info("Initialized Wayfarer defaults from {}", kPath);
		}
		Menu::ApplyLoadedSettings();
	}
}
