#include "Shelter.h"

#include "ScriptObjects.h"

// Frostfall asks Campfire which tent the player is in (CampfireAPI's
// CurrentTent) and whether its base object has the isCampfireTentWarm and
// isCampfireTentWaterproof keywords. So while the player is inside one of the
// camp's tents, CurrentTent is set to it, and cleared again when they leave.
// A tent of Campfire's own is never replaced.
//
// The tents aren't added to Campfire's _Camp_WarmBaseTents list: Frostfall
// clears a tent from that list whenever its overhead ray finds no roof, and
// it starts above a low tent's roof.
namespace Shelter
{
	namespace
	{
		constexpr std::uint32_t kTentsRecord = 'TENT';
		constexpr std::uint32_t kTentsVersion = 1;

		// Campfire 1.12's keywords, if their EditorIDs aren't available.
		constexpr RE::FormID kWarmKeyword = 0x03AB3B;
		constexpr RE::FormID kWaterproofKeyword = 0x03AB3A;
		constexpr auto       kCampfire = "Campfire.esm"sv;

		// How far below a tent's bounds the player may stand (on a slope) and
		// still be inside.
		constexpr float kFloorSlack = 64.0f;

		std::vector<RE::ObjectRefHandle> tents;
		std::atomic<bool>                busy{ false };

		RE::BGSKeyword* CampfireKeyword(std::string_view a_editorID, RE::FormID a_localID)
		{
			if (auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(a_editorID)) {
				return keyword;
			}
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			return dataHandler ? dataHandler->LookupForm<RE::BGSKeyword>(a_localID, kCampfire) : nullptr;
		}

		bool CampfireLoaded()
		{
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			return dataHandler && dataHandler->LookupLoadedModByName(kCampfire);
		}

		// Campfire's CurrentTent, if Campfire is running.
		RE::BSScript::Variable* CurrentTent()
		{
			static RE::FormID quest = 0;
			auto object = ScriptObjects::FindQuestScript("Campfire."sv, "CampfireAPI", quest);
			return object ? ScriptObjects::AutoProperty(object.get(), "CurrentTent") : nullptr;
		}

		void AddKeywords(RE::TESObjectREFR* a_tent)
		{
			auto* base = a_tent ? a_tent->GetBaseObject() : nullptr;
			auto* keywords = base ? base->As<RE::BGSKeywordForm>() : nullptr;
			if (!keywords) {
				return;
			}
			for (auto* keyword : { CampfireKeyword("isCampfireTentWarm"sv, kWarmKeyword),
					 CampfireKeyword("isCampfireTentWaterproof"sv, kWaterproofKeyword) }) {
				if (keyword && !keywords->HasKeyword(keyword)) {
					keywords->AddKeyword(keyword);
				}
			}
		}

		bool IsOurs(RE::TESObjectREFR* a_ref)
		{
			return a_ref && std::ranges::any_of(tents, [&](const RE::ObjectRefHandle& a_tent) { return a_tent.get().get() == a_ref; });
		}

		// Inside the tent's bounds, turned to its heading.
		bool IsInside(RE::TESObjectREFR* a_tent, const RE::NiPoint3& a_point)
		{
			auto* base = a_tent->GetBaseObject();
			if (!base) {
				return false;
			}
			const auto& bounds = base->boundData;
			const float scale = a_tent->GetScale();
			const auto  position = a_tent->GetPosition();
			const float yaw = a_tent->GetAngleZ();
			const float dx = a_point.x - position.x;
			const float dy = a_point.y - position.y;
			// Forward is (sin, cos), right is (cos, -sin).
			const float right = dx * std::cos(yaw) - dy * std::sin(yaw);
			const float forward = dx * std::sin(yaw) + dy * std::cos(yaw);
			const float up = a_point.z - position.z;
			return right >= bounds.boundMin.x * scale && right <= bounds.boundMax.x * scale &&
			       forward >= bounds.boundMin.y * scale && forward <= bounds.boundMax.y * scale &&
			       up >= bounds.boundMin.z * scale - kFloorSlack && up <= bounds.boundMax.z * scale;
		}

		void SetCurrentTent(RE::BSScript::Variable* a_current, RE::TESObjectREFR* a_tent)
		{
			if (a_tent) {
				a_current->Pack(a_tent);
			} else {
				a_current->SetNone();
			}
		}

		// Runs on the main thread, once a second while there are tents.
		void Update()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* current = CurrentTent();
			if (!player || !player->Is3DLoaded() || !current) {
				return;
			}

			RE::TESObjectREFR* inside = nullptr;
			for (const auto& handle : tents) {
				auto tent = handle.get();
				if (tent && tent->GetWorldspace() == player->GetWorldspace() && IsInside(tent.get(), player->GetPosition())) {
					inside = tent.get();
					break;
				}
			}

			auto* now = current->Unpack<RE::TESObjectREFR*>();
			if (inside && now != inside && (!now || IsOurs(now))) {
				SetCurrentTent(current, inside);
			} else if (!inside && IsOurs(now)) {
				SetCurrentTent(current, nullptr);
			}
		}

		void Remember(const std::vector<RE::TESObjectREFR*>& a_tents)
		{
			tents.clear();
			for (auto* tent : a_tents) {
				if (tent) {
					tents.push_back(tent->CreateRefHandle());
					AddKeywords(tent);
				}
			}
			busy = !tents.empty();
		}
	}

	void Install()
	{
		std::thread([] {
			for (;;) {
				std::this_thread::sleep_for(1s);
				if (busy) {
					if (auto* tasks = SKSE::GetTaskInterface()) {
						tasks->AddTask(Update);
					}
				}
			}
		}).detach();
	}

	void SetTents(const std::vector<RE::TESObjectREFR*>& a_tents)
	{
		ClearTents();
		if (CampfireLoaded()) {
			Remember(a_tents);
		}
	}

	void ClearTents()
	{
		if (auto* current = busy ? CurrentTent() : nullptr; current && IsOurs(current->Unpack<RE::TESObjectREFR*>())) {
			SetCurrentTent(current, nullptr);
		}
		tents.clear();
		busy = false;
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (tents.empty() || !a_intfc->OpenRecord(kTentsRecord, kTentsVersion)) {
			return;
		}
		a_intfc->WriteRecordData(static_cast<std::uint32_t>(tents.size()));
		for (const auto& handle : tents) {
			auto       tent = handle.get();
			RE::FormID id = tent ? tent->GetFormID() : 0;
			a_intfc->WriteRecordData(id);
		}
	}

	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version)
	{
		if (a_type != kTentsRecord) {
			return false;
		}
		std::uint32_t count = 0;
		if (a_version != kTentsVersion || a_intfc->ReadRecordData(count) != sizeof(count) || !CampfireLoaded()) {
			return true;
		}
		std::vector<RE::TESObjectREFR*> loaded;
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID id = 0;
			if (a_intfc->ReadRecordData(id) != sizeof(id)) {
				break;
			}
			if (id && a_intfc->ResolveFormID(id, id)) {
				loaded.push_back(RE::TESForm::LookupByID<RE::TESObjectREFR>(id));
			}
		}
		Remember(loaded);
		return true;
	}

	void Revert()
	{
		tents.clear();
		busy = false;
	}
}
