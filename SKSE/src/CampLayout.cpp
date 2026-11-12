#include "CampLayout.h"

#include "CampKits.h"

#include "BedrollSeats.h"
#include "GrassHider.h"
#include "Papyrus.h"
#include "Placement.h"
#include "ScriptObjects.h"

#include <random>

// A port of what LZC_CampsiteQuestScript's SetUpCamp used to work out in
// Papyrus, which only gets a sliver of each frame. Bearings are in degrees,
// clockwise from the player's facing, seen from the fire; "fwd" is along the
// player's facing and "side" to their right.
namespace CampLayout
{
	namespace
	{
		constexpr auto kClass = "LZC_CampsiteQuestScript"sv;
		constexpr auto kScript = "LZC_CampsiteQuestScript";
		constexpr auto kPluginPrefix = "Wayfarer - A Convenient Camping Framework."sv;
		constexpr RE::FormID kXMarker = 0x3B;  // Skyrim.esm XMarker: the fire's place when there's no fire
		constexpr auto kBedrollAltPlugin = "Bedroll Alternative - BOS Color Variance.esp"sv;
		constexpr auto kAshNomadBedrollsPlugin = "Ash Nomad - Bedrolls - BOS Distributed.esp"sv;

		// Slots in the layout (and in the script's placedRefs).
		constexpr std::size_t kCampfire = 0;
		constexpr std::size_t kLight = 1;
		constexpr std::size_t kPlayerSeat = 2;
		constexpr std::size_t kFirstFollowerSeat = 3;  // 3-7
		constexpr std::size_t kFirstTent = 8;          // 8-10
		constexpr std::size_t kFirstBedroll = 11;      // 11-22, the player's first
		constexpr std::size_t kFirstBedrollSeat = 23;  // 23-34, one on each bedroll
		constexpr std::size_t kFirepit = 35;
		constexpr std::size_t kChoppingBlock = 36;
		constexpr std::size_t kCookingStation = 37;
		constexpr std::size_t kTub = 38;
		constexpr std::size_t kSlotCount = 39;

		constexpr std::int32_t kMaxTents = 3;
		constexpr std::int32_t kMaxBedrolls = 12;
		constexpr std::int32_t kRingBedrolls = 6;
		constexpr std::int32_t kOutsideBedrolls = 3;  // most bedrolls outside the tents, once they're full
		constexpr std::int32_t kFullRing = 5;  // from this many tentless bedrolls, a full ring round the fire
		constexpr std::int32_t kMaxFollowerSeats = 5;
		constexpr std::int32_t kSmall = 0;
		constexpr std::int32_t kLarge = 1;

		// Layout, in game units (~70 to the metre) and degrees.
		constexpr float kLightHeight = 40.0f;
		constexpr float kSeatRadius = 110.0f;  // also how far in front of the player the fire goes
		constexpr float kSmallTentDistance = 250.0f;
		constexpr float kLargeTentDistance = 107.0f;
		constexpr float kLargeTentOutside = 285.0f;  // with FireOutsideLargeTent: this much further from the fire
		constexpr float kSideTentOutside = 265.0f;   // and the small tents beside it this much further back too
		constexpr float kSmallTentHalfWidth = 85.0f;
		constexpr float kSmallTentHalfDepth = 85.0f;
		constexpr float kLargeTentHalfWidth = 150.0f;
		constexpr float kLargeTentHalfDepth = 100.0f;
		constexpr float kPairedBedrollSpacing = 60.0f;  // between the two of a pair in a tent
		constexpr float kBedrollSpacing = 214.0f;       // from one pair to the next
		constexpr float kLargeTentBedrollBack = 60.0f;
		constexpr float kBedrollRingRadius = 190.0f;  // without tents
		constexpr float kBedrollRingYaw = 90.0f;
		constexpr float kTubHalfWidth = 110.0f;
		constexpr float kTubInTentRight = 137.0f;
		constexpr float kTubInTentTurn = 20.0f;
		constexpr float kFireHalfSize = 50.0f;
		constexpr float kFanStep = 70.0f;
		constexpr float kRingStep = 50.0f;
		constexpr float kSeatStep = 60.0f;
		constexpr float kChoppingBlockGap = 60.0f;
		constexpr float kChoppingBlockHalfSize = 40.0f;
		constexpr float kChoppingBlockTentlessOut = 60.0f;  // without tents: this much further from the fire than the tub
		constexpr float kChoppingBlockTentlessTurn = -8.0f;  // without tents: on top of kChoppingBlockTurn (negative: anticlockwise), 17 clockwise in all
		constexpr float kChoppingBlockBesideSmallTent = 90.0f;
		constexpr float kChoppingBlockBesideLargeTent = 170.0f;
		constexpr float kChoppingBlockTurn = 25.0f;
		constexpr float kBedrollHalfLength = 100.0f;
		constexpr float kBedrollSeatTurn = 180.0f;  // the marker on each bedroll, clockwise from the bedroll's own facing
		constexpr float kBedrollSeatLift = 3.0f;    // and this far above the ground
		constexpr float kPlayerSeatLift = 8.0f;     // the player's cushioned fire seats, above the ground
		// Small tents beside the large tent, from the fire's centre (negative
		// fwd: toward the player), the right one turned clockwise from facing
		// the player, the left mirrored.
		constexpr float kSideTentForward = -149.0f;
		constexpr float kSideTentSide = 317.0f;
		constexpr float kSideTentTurn = 55.0f;
		// The tub beside a small tent, in the tent's own frame.
		constexpr float kTubBesideSmallForward = 98.0f;
		constexpr float kTubBesideSmallRight = 220.0f;
		constexpr float kTubBesideSmallTurn = 17.0f;
		// Ground limits. Fixed here rather than script properties, whose values
		// stay in the save even after the script's default changes.
		constexpr float kMaxSlope = 30.0f;  // refuse to camp on steeper ground
		constexpr float kMaxTilt = 10.0f;   // most an object tilts with the ground
		constexpr float kMaxStep = 48.0f;   // most the ground under a tent may vary
		constexpr float kTubMaxTilt = 6.0f;  // the tub tilts less than the rest
		constexpr float kRolledDrop = 10.0f;
		constexpr float kDimLightFade = 0.5f;  // "Dim" campfire light: this much of the light's brightness taken off  // a rolled-up bedroll starts this far above its bedroll

		inline float Cos(float a_degrees) { return std::cos(a_degrees * std::numbers::pi_v<float> / 180.0f); }
		inline float Sin(float a_degrees) { return std::sin(a_degrees * std::numbers::pi_v<float> / 180.0f); }

		// What the last BuildCamp left for the script to read back.
		struct Built
		{
			std::int32_t                    status{ Papyrus::Camp::kBadArguments };
			std::vector<RE::TESObjectREFR*> fireSeats;  // the player's first
			std::vector<std::int32_t>       firePlaces;
			std::vector<RE::TESObjectREFR*> beds;
			std::vector<RE::TESObjectREFR*> bedSeats;
			std::vector<std::int32_t>       bedGroups;
			std::vector<std::int32_t>       bedOwners;
			std::vector<std::int32_t>       bedOrders;
			std::vector<RE::TESObjectREFR*> tubSeats;
			std::vector<RE::TESObjectREFR*> rolledKits;  // by bedroll, None once unrolled (or never rolled)
			RE::TESObjectREFR*              cookBox{ nullptr };
			std::vector<std::int32_t>       owners;      // by returned reference: the layout slot it belongs to
		};
		Built built;

		// --- The quest script's properties -----------------------------------

		using ScriptObjects::FormValue;

		class Properties
		{
		public:
			explicit Properties(RE::BSScript::Object* a_object) :
				_object(a_object)
			{}

			template <class T>
			T* Form(std::string_view a_name) const
			{
				auto* form = FormValue(Value(a_name));
				return form ? form->As<T>() : nullptr;
			}

			template <class T>
			std::vector<T*> Forms(std::string_view a_name) const
			{
				std::vector<T*> forms;
				// The tent arrays have FormList copies in the plugin. The plugin
				// is read fresh on every load, but a property array keeps the
				// value it had when the save first ran the script, so a tent
				// added to the plugin later would never show up in old saves.
				if (auto* list = PluginList(a_name)) {
					for (auto* form : list->forms) {
						forms.push_back(form ? form->As<T>() : nullptr);
					}
					return forms;
				}
				const auto*     value = Value(a_name);
				const auto      array = value && value->IsArray() ? value->GetArray() : nullptr;
				if (array) {
					for (const auto& element : *array) {
						auto* form = FormValue(&element);
						forms.push_back(form ? form->As<T>() : nullptr);
					}
				}
				return forms;
			}

			// The FormList in the plugin that stands in for array property
			// a_name, if there is one and it isn't empty.
			static RE::BGSListForm* PluginList(std::string_view a_name)
			{
				static constexpr std::pair<std::string_view, RE::FormID> kLists[] = {
					{ "LZC_SmallTentKits", 0xDA3 },  // LZC_SmallTentKitList
					{ "LZC_LargeTentKits", 0xDA4 },  // LZC_LargeTentKitList
					{ "LZC_SmallTents", 0xDA5 },     // LZC_SmallTentList
					{ "LZC_LargeTents", 0xDA6 },     // LZC_LargeTentList
				};
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				if (!dataHandler) {
					return nullptr;
				}
				for (const auto& [name, id] : kLists) {
					if (name != a_name) {
						continue;
					}
					for (const auto* extension : { "esp", "esl", "esm" }) {
						const auto plugin = std::string(kPluginPrefix) + extension;
						if (auto* list = dataHandler->LookupForm<RE::BGSListForm>(id, plugin); list && !list->forms.empty()) {
							return list;
						}
					}
				}
				return nullptr;
			}

			bool Bool(std::string_view a_name, bool a_default) const
			{
				const auto* value = Value(a_name);
				return value && value->IsBool() ? value->GetBool() : a_default;
			}

			std::int32_t Int(std::string_view a_name, std::int32_t a_default) const
			{
				const auto* value = Value(a_name);
				return value && value->IsInt() ? value->GetSInt() : a_default;
			}

			float Float(std::string_view a_name, float a_default) const
			{
				const auto* value = Value(a_name);
				return value && value->IsFloat() ? value->GetFloat() : a_default;
			}

			// A plain script variable's float array (not a property).
			std::vector<float> Floats(std::string_view a_variable) const
			{
				std::vector<float> floats;
				const auto*        value = _object ? _object->GetVariable(RE::BSFixedString(a_variable)) : nullptr;
				const auto         array = value && value->IsArray() ? value->GetArray() : nullptr;
				if (array) {
					for (const auto& element : *array) {
						floats.push_back(element.IsFloat() ? element.GetFloat() : 0.0f);
					}
				}
				return floats;
			}

		private:
			const RE::BSScript::Variable* Value(std::string_view a_name) const
			{
				return ScriptObjects::AutoProperty(_object, a_name);
			}

			RE::BSScript::Object* _object;
		};

		// Moves a_ref (the tub's trigger box or water) onto the tub, as it stood
		// around the tub anchor in the holding cell: a_offset is its right,
		// forward and up from the anchor and its turn (degrees), worked out by
		// the script. The script disables it first, so it loads afresh at the
		// tub when enabled here: the game only treats placeable water as water
		// when its reference loads.
		// How far the tub's rim drops on its low side when the tub tilts with
		// the slope: the water, which stays level, is lowered by this much so it
		// doesn't show through the rim there.
		float TubRimDrop(const RE::TESObjectREFR* a_tub)
		{
			const auto* base = a_tub->GetBaseObject();
			if (!base) {
				return 0.0f;
			}
			const auto&         bounds = base->boundData;
			const float         halfX = (bounds.boundMax.x - bounds.boundMin.x) * 0.5f;
			const float         halfY = (bounds.boundMax.y - bounds.boundMin.y) * 0.5f;
			const RE::NiMatrix3 rotation = Placement::MatrixFromAngles(a_tub->data.angle);
			// The world height of the tub's right and forward axes: how much
			// each corner rises or falls.
			return halfX * std::abs(rotation.entry[2][0]) + halfY * std::abs(rotation.entry[2][1]);
		}

		void MoveOntoTub(RE::TESObjectREFR* a_tub, RE::TESObjectREFR* a_ref, const std::vector<float>& a_offset, float a_lower = 0.0f)
		{
			if (!a_ref || a_offset.size() < 4) {
				return;
			}
			const float        t = a_tub->GetAngleZ();
			const RE::NiPoint3 position = a_tub->GetPosition() + RE::NiPoint3{ a_offset[0] * std::cos(t) + a_offset[1] * std::sin(t),
																	 a_offset[1] * std::cos(t) - a_offset[0] * std::sin(t), a_offset[2] - a_lower };
			a_ref->MoveTo(a_tub);
			a_ref->data.angle = { 0.0f, 0.0f, t + a_offset[3] * std::numbers::pi_v<float> / 180.0f };
			a_ref->SetPosition(position);
			a_ref->Enable(false);
		}

		// The world map only shows markers in the player's map marker list,
		// which the game fills from the worldspace on loading. The camp's marker
		// starts out in the holding cell, an interior, so it's never in it:
		// add it.
		void ListMapMarker(RE::TESObjectREFR* a_marker)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !a_marker) {
				return;
			}
			RE::BSTArray<RE::ObjectRefHandle>* list = nullptr;
			if (REL::Module::IsVR()) {
				if (auto* vr = player->GetVRPlayerRuntimeData()) {
					list = &vr->currentMapMarkers;
				}
			} else {
				list = &player->GetPlayerRuntimeData().currentMapMarkers;
			}
			if (!list) {
				return;
			}
			auto&      markers = *list;
			const auto handle = a_marker->GetHandle();
			if (std::ranges::find(markers, handle) == markers.end()) {
				markers.push_back(handle);
			}
		}

		// --- Layout helpers ----------------------------------------------------

		// Place `index` in the fan: 0 in the middle, then alternately right and
		// left, further out each pair: 0, 1, -1, 2, -2...
		std::int32_t FanPlace(std::int32_t a_index)
		{
			const std::int32_t place = (a_index + 1) / 2;
			return a_index % 2 == 0 ? -place : place;
		}

		float FanBearing(std::int32_t a_index) { return static_cast<float>(FanPlace(a_index)) * kFanStep; }

		float TentHalfWidth(std::int32_t a_size) { return a_size == kLarge ? kLargeTentHalfWidth : kSmallTentHalfWidth; }
		float TentHalfDepth(std::int32_t a_size) { return a_size == kLarge ? kLargeTentHalfDepth : kSmallTentHalfDepth; }

		// Where bedroll `index` lies across a tent's row: in touching pairs,
		// with kBedrollSpacing between pairs.
		float RowPosition(std::int32_t a_index)
		{
			return static_cast<float>(a_index / 2) * (kPairedBedrollSpacing + kBedrollSpacing) +
			       static_cast<float>(a_index % 2) * kPairedBedrollSpacing;
		}

		struct Tent
		{
			RE::TESBoundObject* form{ nullptr };
			std::int32_t        size{ kSmall };
			std::int32_t        color{ 0 };  // its kind: index into _tentTypes
			std::int32_t        beds{ 0 };
			std::int32_t        owner{ -1 };  // the follower whose kit it is (index), or -1 for the player's
			float               fwd{ 0.0f };
			float               side{ 0.0f };
			float               turn{ 0.0f };
		};

		// An addon tent's clutter set given by its anchor's base object: the one
		// placed reference of that base (the anchor) and every reference whose
		// linked reference is the anchor (the items). Both have to be persistent
		// to be in memory. Looked up once per anchor base.
		std::pair<RE::TESObjectREFR*, std::vector<RE::TESObjectREFR*>> LinkedClutter(const RE::TESForm* a_anchorBase)
		{
			static std::map<RE::FormID, std::pair<RE::TESObjectREFR*, std::vector<RE::TESObjectREFR*>>> found;
			if (auto it = found.find(a_anchorBase->GetFormID()); it != found.end()) {
				return it->second;
			}
			RE::TESObjectREFR*              anchor = nullptr;
			std::vector<RE::TESObjectREFR*> items;
			const auto& [map, lock] = RE::TESForm::GetAllForms();
			if (map) {
				const RE::BSReadLockGuard guard{ lock };
				for (const auto& [id, form] : *map) {
					auto* ref = form ? form->As<RE::TESObjectREFR>() : nullptr;
					if (ref && ref->GetBaseObject() == a_anchorBase) {
						anchor = ref;
						break;
					}
				}
				for (const auto& [id, form] : *map) {
					auto* ref = anchor && form ? form->As<RE::TESObjectREFR>() : nullptr;
					if (ref && ref != anchor && ref->GetLinkedRef(nullptr) == anchor) {
						items.push_back(ref);
					}
				}
			}
			return found[a_anchorBase->GetFormID()] = { anchor, std::move(items) };
		}

		// Containers a follower framework keeps a follower's items in, besides
		// their own inventory. Nether's Follower Framework (looked up at run
		// time, not a master): a follower's extra storage is the container in
		// nwsFollowerXStorage's alias 10 + n, where alias n holds the follower
		// (nwsFollowerXStoreScript.OpenStorage); with a custom outfit, it's moved
		// to their satchel, nwsFFSatchelList's entry at their rank in
		// nwsFF_storedFac (nwsFollowerSetsScript).
		std::vector<RE::TESObjectREFR*> FollowerStores(RE::Actor* a_follower)
		{
			constexpr auto       kNFF = "nwsFollowerFramework.esp"sv;
			constexpr RE::FormID kStoredFaction = 0x0487D2;  // nwsFF_storedFac
			constexpr RE::FormID kSatchelList = 0x4AF847;    // nwsFFSatchelList
			constexpr RE::FormID kStorageQuest = 0x4220F4;   // nwsFollowerXStorage
			constexpr std::uint32_t kStorageSlots = 10;
			std::vector<RE::TESObjectREFR*> stores;
			auto*                           dataHandler = RE::TESDataHandler::GetSingleton();
			if (!a_follower || !dataHandler || (!dataHandler->LookupLoadedModByName(kNFF) && !dataHandler->LookupLoadedLightModByName(kNFF))) {
				return stores;
			}
			if (auto* storage = dataHandler->LookupForm<RE::TESQuest>(kStorageQuest, kNFF)) {
				for (std::uint32_t slot = 0; slot < kStorageSlots; ++slot) {
					if (storage->GetAliasedRef(slot).get().get() == a_follower) {
						if (auto container = storage->GetAliasedRef(slot + kStorageSlots).get()) {
							stores.push_back(container.get());
						}
						break;
					}
				}
			}
			auto* stored = dataHandler->LookupForm<RE::TESFaction>(kStoredFaction, kNFF);
			auto* satchels = dataHandler->LookupForm<RE::BGSListForm>(kSatchelList, kNFF);
			if (stored && satchels && a_follower->IsInFaction(stored)) {
				const auto slot = a_follower->GetFactionRank(stored, false);
				if (slot >= 0 && static_cast<std::size_t>(slot) < satchels->forms.size()) {
					if (auto* satchel = satchels->forms[slot] ? satchels->forms[slot]->As<RE::TESObjectREFR>() : nullptr) {
						stores.push_back(satchel);
					}
				}
			}
			return stores;
		}

		class Builder
		{
		public:
			Builder(const Properties& a_props, RE::Actor* a_player, const std::vector<RE::Actor*>& a_followers, std::int32_t a_partners, std::int32_t a_couples) :
				_props(a_props),
				_player(a_player),
				_partners(a_partners),
				_couples(a_couples)
			{
				_layout.origin = a_player;
				_layout.headingDegrees = a_player->GetAngleZ() * 180.0f / std::numbers::pi_v<float>;
				_layout.forms.assign(kSlotCount, nullptr);
				_layout.fwd.assign(kSlotCount, 0.0f);
				_layout.side.assign(kSlotCount, 0.0f);
				_layout.lift.assign(kSlotCount, 0.0f);
				_layout.turn.assign(kSlotCount, 0.0f);
				_layout.maxTilt.assign(kSlotCount, -1.0f);
				_layout.tiltClutter.assign(kSlotCount, false);
				_layout.clutterSet.assign(kSlotCount, 0);
				_layout.alignToSlope = a_props.Bool("AlignToSlope", true);
				_layout.maxTiltDegrees = kMaxTilt;
				_bedGroup.assign(kMaxBedrolls, 0);
				_bedOrder.assign(kMaxBedrolls, 0);
				_ringSeatTurn.assign(kMaxBedrolls, 0.0f);
				// The kits the followers carry count too: a follower with a tent kit
				// and a bedroll kit brings their own tent.
				_counts = a_player->GetInventoryCounts();
				_followers = a_followers;
				_followerCounts.resize(a_followers.size());
				for (std::size_t f = 0; f < a_followers.size(); ++f) {
					if (!a_followers[f]) {
						continue;
					}
					_followerCounts[f] = a_followers[f]->GetInventoryCounts();
					// Kits a follower framework keeps for them elsewhere count too.
					for (auto* store : FollowerStores(a_followers[f])) {
						for (const auto& [object, count] : store->GetInventoryCounts()) {
							_followerCounts[f][object] += count;
						}
					}
					for (const auto& [object, count] : _followerCounts[f]) {
						_counts[object] += count;
					}
				}
			}

			std::vector<RE::TESObjectREFR*> Run()
			{
				FindTentTypes();
				LogKits();
				ChooseTents();
				ShareBedrolls();
				LogTents();
				PlaceTents();

				std::vector<Papyrus::Camp::Footprint> footprints{ { kSeatRadius, 0.0f, 0.0f, kFireHalfSize, kFireHalfSize } };
				for (const auto& tent : _tents) {
					footprints.push_back({ tent.fwd, tent.side, tent.turn, TentHalfWidth(tent.size), TentHalfDepth(tent.size) });
				}
				built.status = Papyrus::Camp::CheckSite(_player, _layout.headingDegrees, footprints,
					kMaxStep, kMaxSlope);
				if (built.status != Papyrus::Camp::kClear) {
					return {};
				}

				LayOutFire();
				if (_props.Form<RE::TESBoundObject>("LZC_Tub") && _props.Bool("PlaceTub", true)) {
					LayOutTub();
				}
				const bool firewood = _props.Bool("PlaceCampfire", true) && _props.Bool("FirewoodMechanic", false) && _props.Form<RE::TESBoundObject>("LZC_Firepit");
				if (firewood) {
					Set(kFirepit, _props.Form<RE::TESForm>("LZC_Firepit"), kSeatRadius, 0.0f, 0.0f);
				}
				if (_props.Bool("PlaceFireSeats", true)) {
					LayOutSeats();
				}
				if (_tents.empty()) {
					LayOutRing();
				} else {
					LayOutTentBedrolls();
				}
				LayOutBedrollSeats();
				// After the bedrolls: without tents it goes beside the player's.
				if (firewood && _props.Form<RE::TESBoundObject>("LZC_ChoppingBlock") && _props.Bool("PlaceChoppingBlock", true)) {
					LayOutChoppingBlock();
				}
				CollectClutter();

				std::vector<std::int32_t> owners;
				auto                      refs = Papyrus::Camp::Place(_layout, &owners);
				if (refs.size() <= kSlotCount) {
					built.status = Papyrus::Camp::kBadArguments;
					return {};
				}
				Settle(refs);
				RollUp(refs, owners);
				built.owners = owners;
				Report(refs);
				PutAwayKits();
				return refs;
			}

		private:
			std::int32_t Count(RE::TESForm* a_form) const
			{
				auto* object = a_form ? a_form->As<RE::TESBoundObject>() : nullptr;
				const auto it = object ? _counts.find(object) : _counts.end();
				return it != _counts.end() ? it->second : 0;
			}

			// Whose tent kit a tent goes up from: a follower's first (so they
			// can sleep in their own tent), then the player's. Returns the
			// follower's index, or -1.
			std::int32_t TakeKit(RE::TESForm* a_kit)
			{
				auto* object = a_kit ? a_kit->As<RE::TESBoundObject>() : nullptr;
				for (std::size_t f = 0; object && f < _followerCounts.size(); ++f) {
					if (auto it = _followerCounts[f].find(object); it != _followerCounts[f].end() && it->second > 0) {
						--it->second;
						return static_cast<std::int32_t>(f);
					}
				}
				return -1;
			}

			// a_count of a_kit out of follower a_follower's own inventory, then the
			// stores a follower framework keeps for them. Returns how many.
			std::int32_t TakeFromFollower(std::size_t a_follower, RE::TESBoundObject* a_kit, std::int32_t a_count)
			{
				auto* follower = a_follower < _followers.size() ? _followers[a_follower] : nullptr;
				if (!follower) {
					return 0;
				}
				std::int32_t took = CampKits::Take(follower, a_kit, a_count);
				for (auto* store : FollowerStores(follower)) {
					if (took < a_count) {
						took += CampKits::Take(store, a_kit, a_count - took);
					}
				}
				return took;
			}

			// The kits the camp was set up from leave their owners' inventories
			// until it's packed up: each tent's kit from whoever's it was, and a
			// bedroll kit for each bedroll, followers' first.
			void PutAwayKits()
			{
				for (const auto& tent : _tents) {
					const auto type = static_cast<std::size_t>(tent.color);
					auto*      kitForm = type < _tentTypes.size() ? _tentTypes[type].kit : nullptr;
					auto*      kit = kitForm ? kitForm->As<RE::TESBoundObject>() : nullptr;
					if (!kit) {
						continue;
					}
					if (tent.owner < 0 || TakeFromFollower(static_cast<std::size_t>(tent.owner), kit, 1) < 1) {
						CampKits::Take(_player, kit, 1);
					}
				}
				auto* bedrollForm = _props.Form<RE::TESForm>("LZC_BedrollKit");
				auto* bedroll = bedrollForm ? bedrollForm->As<RE::TESBoundObject>() : nullptr;
				std::int32_t left = bedroll ? _bedrolls : 0;
				for (std::size_t f = 0; f < _followers.size() && left > 0; ++f) {
					left -= TakeFromFollower(f, bedroll, left);
				}
				if (left > 0) {
					CampKits::Take(_player, bedroll, left);
				}
			}

			// The kits each camper carries, to the log.
			void LogKits() const
			{
				std::vector<RE::TESForm*> kits{ _props.Form<RE::TESForm>("LZC_BedrollKit") };
				for (const auto& kind : _tentTypes) {
					kits.push_back(kind.kit);
				}
				const auto describe = [&](const RE::TESObjectREFR::InventoryCountMap& a_counts) {
					std::string text;
					for (auto* kit : kits) {
						auto* object = kit ? kit->As<RE::TESBoundObject>() : nullptr;
						if (auto it = object ? a_counts.find(object) : a_counts.end(); it != a_counts.end() && it->second > 0) {
							text += std::format("{}{} x{}", text.empty() ? "" : ", ", kit->GetName(), it->second);
						}
					}
					return text.empty() ? std::string("no kits") : text;
				};
				auto* player = RE::PlayerCharacter::GetSingleton();
				logger::info("BuildCamp: player carries {}", describe(player ? player->GetInventoryCounts() : RE::TESObjectREFR::InventoryCountMap{}));
				logger::info("BuildCamp: {} follower slot(s), {} partner(s), {} couple(s)", _followers.size(), _partners, _couples);
				for (std::size_t f = 0; f < _followers.size(); ++f) {
					if (_followers[f]) {
						logger::info("BuildCamp: follower {} {} ({:08X}) carries {}", f, _followers[f]->GetName(), _followers[f]->GetFormID(),
							describe(_followerCounts[f]));
					}
				}
			}

			// The tents chosen and their bedrolls, to the log.
			void LogTents() const
			{
				logger::info("BuildCamp: {} bedroll kit(s) in all, {} tent(s), {} bedroll(s) outside", _bedrollsCarried, _tents.size(), _outside);
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					const auto& tent = _tents[t];
					logger::info("BuildCamp: tent {}: {} colour {}, {} bedroll(s), kit from {}", t, tent.size == kLarge ? "large" : "small", tent.color, tent.beds,
						tent.owner < 0 ? std::string("the player") : std::format("follower {}", tent.owner));
				}
			}

			// The kinds of tent: the plugin's own (LZC_SmallTentKits/LZC_SmallTents
			// and the large ones, same order), then addon tents. An addon tent is
			// a form list in any plugin whose first entry is LZC_TentAddonSmall or
			// LZC_TentAddonLarge, then the kit (MiscObject), the tent (Activator),
			// and optionally its own clutter: the anchor reference and the item
			// references, arranged around it as for the plugin's own sets.
			void FindTentTypes()
			{
				for (const auto& [kits, tents, size] : { std::tuple{ "LZC_LargeTentKits", "LZC_LargeTents", kLarge },
						 std::tuple{ "LZC_SmallTentKits", "LZC_SmallTents", kSmall } }) {
					const auto kitForms = _props.Forms<RE::TESForm>(kits);
					const auto tentForms = _props.Forms<RE::TESBoundObject>(tents);
					for (std::size_t i = 0; i < kitForms.size(); ++i) {
						_tentTypes.push_back({ kitForms[i], i < tentForms.size() ? tentForms[i] : nullptr, size });
					}
				}
				auto* smallMarker = _props.Form<RE::BGSKeyword>("LZC_TentAddonSmall");  // not "small": a windows.h macro
				auto* largeMarker = _props.Form<RE::BGSKeyword>("LZC_TentAddonLarge");
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				if (!dataHandler || (!smallMarker && !largeMarker)) {
					return;
				}
				std::int32_t nextSet = 7;  // after the plugin's six clutter sets
				for (auto* list : dataHandler->GetFormArray<RE::BGSListForm>()) {
					if (!list) {
						continue;
					}
					const auto& forms = list->forms;
					if (forms.size() < 3 || !forms[0] || (forms[0] != smallMarker && forms[0] != largeMarker)) {
						continue;
					}
					// Kit/tent pairs (a kit, then the Activator after it), then the
					// clutter shared by every tent in the list: the anchor's base
					// object (its items are linked to the anchor), or the anchor and
					// item references themselves (lists edited in xEdit).
					const std::int32_t  size = forms[0] == largeMarker ? kLarge : kSmall;
					std::vector<TentType> kinds;
					RE::TESForm*        kit = nullptr;
					RE::TESObjectREFR*  anchor = nullptr;
					RE::TESForm*        anchorBase = nullptr;
					std::vector<RE::TESForm*> bedrolls;
					std::int32_t              bedrollStyle = 0;
					RE::TESObjectREFR*        fireAnchor = nullptr;
					std::vector<RE::TESObjectREFR*> fireItems;
					bool                      noFireClutter = false;
					std::vector<RE::TESObjectREFR*> items;
					for (std::size_t i = 1; i < forms.size(); ++i) {
						auto* form = forms[i];
						if (!form) {
							continue;
						}
						if (auto* ref = form->As<RE::TESObjectREFR>()) {
							if (anchor) {
								items.push_back(ref);
							} else {
								anchor = ref;
							}
						} else if (form->Is(RE::FormType::Activator)) {
							if (kit) {
								kinds.push_back({ kit, form->As<RE::TESBoundObject>(), size });
								kit = nullptr;
							} else {
								logger::warn("Tent addon list {:08X}: tent {:08X} has no kit before it", list->GetFormID(), form->GetFormID());
							}
						} else if (form->Is(RE::FormType::Misc)) {
							kit = form;
						} else if (form->Is(RE::FormType::Furniture)) {
							bedrolls.push_back(form);
						} else if (auto* global = form->As<RE::TESGlobal>()) {
							bedrollStyle = static_cast<std::int32_t>(global->value);
						} else if (auto* nested = form->As<RE::BGSListForm>()) {
							// The fire's clutter when one of these is the player's tent:
							// an anchor base (items linked to it), or references; an
							// empty list for none at all.
							noFireClutter = nested->forms.empty();
							for (auto* entry : nested->forms) {
								if (auto* ref = entry ? entry->As<RE::TESObjectREFR>() : nullptr) {
									if (fireAnchor) {
										fireItems.push_back(ref);
									} else {
										fireAnchor = ref;
									}
								} else if (entry && !fireAnchor) {
									std::tie(fireAnchor, fireItems) = LinkedClutter(entry);
								}
							}
							if (!fireAnchor && !noFireClutter) {
								logger::warn("Tent addon list {:08X}: no fire clutter anchor found in {:08X}", list->GetFormID(), nested->GetFormID());
							}
						} else {
							anchorBase = form;
						}
					}
					if (!anchor && anchorBase) {
						std::tie(anchor, items) = LinkedClutter(anchorBase);
						if (!anchor) {
							logger::warn("Tent addon list {:08X}: no persistent reference of clutter anchor {:08X}", list->GetFormID(), anchorBase->GetFormID());
						}
					}
					if (kinds.empty()) {
						logger::warn("Tent addon list {:08X} needs a kit and then a tent after its keyword", list->GetFormID());
						continue;
					}
					const std::int32_t clutterSet = anchor ? nextSet++ : 0;
					const std::int32_t fireSet = fireAnchor ? nextSet++ : noFireClutter ? -1 : 0;
					for (auto& kind : kinds) {
						// A tent already added (by another list) is updated, not
						// added again: a patch can give an addon's tents a bedroll,
						// or clutter, by repeating its kit/tent pairs.
						const auto same = std::ranges::find_if(_tentTypes, [&](const TentType& a_type) { return a_type.tent == kind.tent; });
						if (same != _tentTypes.end()) {
							if (!bedrolls.empty()) {
								same->bedrolls = bedrolls;
							}
							if (bedrollStyle != 0) {
								same->bedrollStyle = bedrollStyle;
							}
							if (fireAnchor || noFireClutter) {
								same->fireAnchor = fireAnchor;
								same->fireItems = fireItems;
								same->fireSet = fireSet;
							}
							if (anchor) {
								same->anchor = anchor;
								same->items = items;
								same->clutterSet = clutterSet;
							}
							logger::info("Tent addon {:08X}: updates tent {:08X} ({} bedroll(s))", list->GetFormID(), kind.tent->GetFormID(), bedrolls.size());
							continue;
						}
						kind.bedrolls = bedrolls;
						kind.bedrollStyle = bedrollStyle;
						kind.fireAnchor = fireAnchor;
						kind.fireItems = fireItems;
						kind.fireSet = fireSet;
						kind.anchor = anchor;
						kind.items = items;
						kind.clutterSet = clutterSet;
						logger::info("Tent addon {:08X}: {} tent {:08X}, kit {:08X}, {} clutter item(s)", list->GetFormID(),
							size == kLarge ? "large" : "small", kind.tent->GetFormID(), kind.kit->GetFormID(), kind.items.size());
						_tentTypes.push_back(std::move(kind));
					}
				}
			}

			void Set(std::size_t a_slot, RE::TESForm* a_form, float a_fwd, float a_side, float a_turn)
			{
				_layout.forms[a_slot] = a_form;
				_layout.fwd[a_slot] = a_fwd;
				_layout.side[a_slot] = a_side;
				_layout.turn[a_slot] = a_turn;
			}

			std::int32_t TentCapacity(std::int32_t a_size) const
			{
				std::int32_t capacity = _props.Int("SmallTentCapacity", 2);
				if (a_size == kLarge) {
					capacity = _props.Int("LargeTentCapacity", 4);
					// The tub takes the third and fourth bedrolls' place.
					if (_props.Bool("TubInLargeTent", false) && _props.Bool("PlaceTub", true) &&
						_props.Form<RE::TESBoundObject>("LZC_Tub") && capacity > 2) {
						capacity = 2;
					}
				}
				return std::clamp(capacity, 1, 4);
			}

			// The fewest bedrolls tent `index` starts with: the player's holds them
			// and their partners, the next `couples` a follower couple each, the
			// rest one.
			std::int32_t MinBeds(std::size_t a_index, std::int32_t a_size) const
			{
				std::int32_t want = 1;
				if (a_index == 0) {
					want = 1 + _partners;
				} else if (static_cast<std::int32_t>(a_index) <= _couples) {
					want = 2;
				}
				return std::min(want, TentCapacity(a_size));
			}

			// Up to kMaxTents and never more than bedrolls. At most one large tent:
			// the first large kit carried (in colour order) is the player's, in the
			// middle; every small kit carried fills the rest, in colour order.
			void ChooseTents()
			{
				_bedrollsCarried = Count(_props.Form<RE::TESForm>("LZC_BedrollKit"));
				for (std::size_t type = 0; type < _tentTypes.size() && _tents.empty() && _bedrollsCarried > 0; ++type) {
					const auto& kind = _tentTypes[type];
					if (kind.size == kLarge && kind.kit && Count(kind.kit) > 0) {
						_tents.push_back({ nullptr, kLarge, static_cast<std::int32_t>(type) });
						_tents.back().owner = TakeKit(kind.kit);
					}
				}
				const auto room = [&] {
					return static_cast<std::int32_t>(_tents.size()) < kMaxTents &&
					       static_cast<std::int32_t>(_tents.size()) < _bedrollsCarried;
				};
				for (std::size_t type = 0; type < _tentTypes.size() && room(); ++type) {
					const auto& kind = _tentTypes[type];
					if (kind.size != kSmall || !kind.kit) {
						continue;
					}
					for (std::int32_t have = Count(kind.kit); have > 0 && room(); --have) {
						_tents.push_back({ nullptr, kSmall, static_cast<std::int32_t>(type) });
						_tents.back().owner = TakeKit(kind.kit);
					}
				}
			}

			// Each tent starts with its minimum; if the bedrolls don't cover that,
			// fewer tents go up rather than splitting anyone up. The rest are
			// shared out in turn up to each tent's capacity. Without a large tent,
			// the tent with the most bedrolls becomes the player's, in the middle.
			void ShareBedrolls()
			{
				if (_tents.empty()) {
					_bedrolls = std::min(_bedrollsCarried, kRingBedrolls);
					return;
				}
				const auto needed = [&] {
					std::int32_t sum = 0;
					for (std::size_t t = 0; t < _tents.size(); ++t) {
						sum += MinBeds(t, _tents[t].size);
					}
					return sum;
				};
				while (_tents.size() > 1 && needed() > _bedrollsCarried) {
					_tents.pop_back();
				}
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					_tents[t].beds = MinBeds(t, _tents[t].size);
					_bedrolls += _tents[t].beds;
				}
				if (_bedrolls > _bedrollsCarried) {
					// A single tent and too few bedrolls for everyone's minimum.
					_tents[0].beds = _bedrollsCarried;
					_bedrolls = _bedrollsCarried;
				}
				for (bool added = true; added && _bedrolls < _bedrollsCarried;) {
					added = false;
					for (auto& tent : _tents) {
						if (_bedrolls >= _bedrollsCarried) {
							break;
						}
						if (tent.beds < TentCapacity(tent.size)) {
							++tent.beds;
							++_bedrolls;
							added = true;
						}
					}
				}
				// Bedrolls the tents can't hold lie outside, by the fire.
				_outside = std::clamp(std::min(_bedrollsCarried - _bedrolls, kOutsideBedrolls), 0, kMaxBedrolls - _bedrolls);
				_bedrolls += _outside;
				if (_tents[0].size != kLarge) {
					std::size_t main = 0;
					for (std::size_t t = 1; t < _tents.size(); ++t) {
						if (_tents[t].beds > _tents[main].beds) {
							main = t;
						}
					}
					std::swap(_tents[0], _tents[main]);
				}
			}

			// The player's tent straight across the fire, the others kFanStep to
			// either side, entrances facing the fire. Small tents beside the large
			// tent have a fixed spot on the player's side, angled in.
			void PlaceTents()
			{
				const auto smallTents = _props.Forms<RE::TESBoundObject>("LZC_SmallTents");
				const auto largeTents = _props.Forms<RE::TESBoundObject>("LZC_LargeTents");
				const bool fireOutside = _props.Bool("FireOutsideLargeTent", false);
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					auto&       tent = _tents[t];
					const auto& forms = tent.size == kLarge ? largeTents : smallTents;
					const auto  type = static_cast<std::size_t>(tent.color);
					tent.form = type < _tentTypes.size() ? _tentTypes[type].tent : nullptr;
					if (!tent.form && !forms.empty()) {
						tent.form = forms[0];
					}
					const float bearing = FanBearing(static_cast<std::int32_t>(t));
					const float distance = tent.size == kLarge ?
					                           kLargeTentDistance + (fireOutside ? kLargeTentOutside : 0.0f) :
					                           kSmallTentDistance;
					tent.fwd = kSeatRadius + distance * Cos(bearing);
					tent.side = distance * Sin(bearing);
					tent.turn = bearing + 180.0f;
					if (t > 0 && _tents[0].size == kLarge) {
						const float sideSign = bearing < 0.0f ? -1.0f : 1.0f;
						tent.fwd = kSeatRadius + kSideTentForward + (fireOutside ? kSideTentOutside : 0.0f);
						tent.side = sideSign * kSideTentSide;
						tent.turn = 180.0f + sideSign * kSideTentTurn;
						if (fireOutside) {
							// In an arc round the fire with the large tent, facing it.
							tent.turn = std::atan2(tent.side, tent.fwd - kSeatRadius) * 180.0f / std::numbers::pi_v<float> + 180.0f;
						}
					}
				}
			}

			void LayOutFire()
			{
				if (!_props.Bool("PlaceCampfire", true)) {
					// No fire: an invisible marker keeps its place as the camp's
					// centre (for the follower quest and the map marker).
					Set(kCampfire, RE::TESForm::LookupByID(kXMarker), kSeatRadius, 0.0f, 0.0f);
					return;
				}
				Set(kCampfire, _props.Form<RE::TESForm>("LZC_CampfirePlaced"), kSeatRadius, 0.0f, 0.0f);
				// One of the two fire clutter sets, at random, if there's a second.
				std::int32_t fireSet = 1;
				if (_props.Form<RE::TESObjectREFR>("LZC_FireClutterAnchor2") && !_props.Forms<RE::TESObjectREFR>("LZC_FireClutter2").empty()) {
					static std::mt19937 random{ std::random_device{}() };
					if (std::uniform_int_distribution<int>(0, 1)(random) == 1) {
						fireSet = 6;
					}
				}
				// An addon's own fire clutter, when the player's tent is one of its.
				if (!_tents.empty()) {
					const auto type = static_cast<std::size_t>(_tents[0].color);
					if (type < _tentTypes.size() && _tentTypes[type].fireSet != 0) {
						fireSet = std::max(_tentTypes[type].fireSet, 0);  // -1: no fire clutter
					}
				}
				_layout.clutterSet[kCampfire] = fireSet;
				if (_props.Bool("PlaceCampfireLight", true)) {
					// "Dim" (CampfireLightLevel 1) uses LZC_CampfireLightDim, if set.
					auto* dim = _props.Int("CampfireLightLevel", 2) == 1 ? _props.Form<RE::TESForm>("LZC_CampfireLightDim") : nullptr;
					Set(kLight, dim ? dim : _props.Form<RE::TESForm>("LZC_CampfireLight"), kSeatRadius, 0.0f, 0.0f);
				}
				_layout.fwd[kLight] = kSeatRadius;
				_layout.lift[kLight] = kLightHeight;
				Set(kCookingStation, _props.Form<RE::TESForm>("LZC_CookingStation"), kSeatRadius, 0.0f, 0.0f);
			}

			// Puts `slot` `distance` from the centre of a tent, straight out from its
			// side and facing the way it does: toward higher fan bearings if
			// `outward` is 1, lower if -1.
			// Degrees between neighbouring places without tents: a close arc of
			// kRingStep for a few bedrolls, spread evenly all round the fire for
			// kFullRing or more.
			float RingStep() const
			{
				return _bedrolls >= kFullRing ? 360.0f / static_cast<float>(_bedrolls) : kRingStep;
			}

			// Outside a full ring of bedrolls, at `bearing`, facing the fire.
			void SetOutsideRing(std::size_t a_slot, float a_bearing, float a_radius)
			{
				Set(a_slot, _layout.forms[a_slot], kSeatRadius + a_radius * Cos(a_bearing), a_radius * Sin(a_bearing), a_bearing + 180.0f);
			}

			void SetBesideTent(std::size_t a_slot, float a_fwd, float a_side, float a_turn, float a_outward, float a_distance)
			{
				_layout.fwd[a_slot] = a_fwd - a_outward * a_distance * Cos(a_turn + 90.0f);
				_layout.side[a_slot] = a_side - a_outward * a_distance * Sin(a_turn + 90.0f);
				_layout.turn[a_slot] = a_turn;
			}

			// Beside the outermost tent on the other side from the chopping block,
			// in the large tent with TubInLargeTent, or left of the player's bedroll
			// without tents. The tub clutter set goes around it.
			void LayOutTub()
			{
				_layout.forms[kTub] = _props.Form<RE::TESForm>("LZC_Tub");
				_layout.clutterSet[kTub] = 5;
				_layout.maxTilt[kTub] = kTubMaxTilt;
				_layout.tiltClutter[kTub] = true;
				if (_props.Bool("TubInLargeTent", false) && !_tents.empty() && _tents[0].size == kLarge) {
					const auto& tent = _tents[0];
					const float offset = -kTubInTentRight;
					_layout.fwd[kTub] = tent.fwd + offset * Cos(tent.turn + 90.0f) - kLargeTentBedrollBack * Cos(tent.turn);
					_layout.side[kTub] = tent.side + offset * Sin(tent.turn + 90.0f) - kLargeTentBedrollBack * Sin(tent.turn);
					_layout.turn[kTub] = tent.turn + kTubInTentTurn;
					return;
				}
				if (!_tents.empty()) {
					const std::size_t beside = _tents.size() >= 3 ? 1 : 0;
					const float       outward = _tents.size() >= 3 ? 1.0f : -1.0f;
					const auto&       tent = _tents[beside];
					if (tent.size == kSmall) {
						const float right = -outward * kTubBesideSmallRight;
						_layout.fwd[kTub] = tent.fwd + kTubBesideSmallForward * Cos(tent.turn) + right * Cos(tent.turn + 90.0f);
						_layout.side[kTub] = tent.side + kTubBesideSmallForward * Sin(tent.turn) + right * Sin(tent.turn + 90.0f);
						_layout.turn[kTub] = tent.turn - outward * kTubBesideSmallTurn;
					} else {
						SetBesideTent(kTub, tent.fwd, tent.side, tent.turn, outward, TentHalfWidth(tent.size) + kChoppingBlockGap + kTubHalfWidth);
					}
					return;
				}
				if (_bedrolls >= kFullRing) {
					// Outside the ring, between the player's bedroll and the next on
					// the left.
					const auto [bearing, radius] = TentlessTubSpot();
					SetOutsideRing(kTub, bearing, radius);
					return;
				}
				SetBesideTent(kTub, kSeatRadius + kBedrollRingRadius, 0.0f, 180.0f, -1.0f, kBedrollHalfLength + kChoppingBlockGap + kTubHalfWidth);
			}

			// Where the tub stands without tents, as a bearing and distance from
			// the fire's centre: outside a full ring, between the player's bedroll
			// and the next on the left, or left of the player's bedroll at the back
			// of the arc.
			std::pair<float, float> TentlessTubSpot() const
			{
				if (_bedrolls >= kFullRing) {
					return { -RingStep() / 2.0f, kBedrollRingRadius + kChoppingBlockGap + kTubHalfWidth };
				}
				const float fwd = kBedrollRingRadius;
				const float side = -(kBedrollHalfLength + kChoppingBlockGap + kTubHalfWidth);
				return { std::atan2(side, fwd) * 180.0f / std::numbers::pi_v<float>, std::hypot(fwd, side) };
			}

			// Beside the outermost tent, on its far side from the middle one;
			// without tents, right of the player's bedroll and its clutter. Always level.
			void LayOutChoppingBlock()
			{
				_layout.forms[kChoppingBlock] = _props.Form<RE::TESForm>("LZC_ChoppingBlock");
				_layout.maxTilt[kChoppingBlock] = 0.0f;
				if (!_tents.empty()) {
					const auto  outer = static_cast<std::int32_t>(_tents.size()) - 1;
					const auto& tent = _tents[outer];
					const float outward = FanBearing(outer) < 0.0f ? -1.0f : 1.0f;
					const float distance = TentHalfWidth(tent.size) + kChoppingBlockGap +
					                       (tent.size == kSmall ? kChoppingBlockBesideSmallTent : kChoppingBlockBesideLargeTent);
					SetBesideTent(kChoppingBlock, tent.fwd, tent.side, tent.turn, outward, distance);
				} else {
					// As far from the fire as ever (the tub's distance, plus a
					// little), round to the right of the player's bedroll and its
					// clutter as seen across the fire (the tub is on the left).
					const float radius = TentlessTubSpot().second + kChoppingBlockTentlessOut;
					SetOutsideRing(kChoppingBlock, RightOfPlayerBedroll() + (kChoppingBlockGap + kChoppingBlockHalfSize) / radius * 180.0f / std::numbers::pi_v<float>,
						radius);
					_layout.turn[kChoppingBlock] += kChoppingBlockTentlessTurn;
				}
				_layout.turn[kChoppingBlock] += kChoppingBlockTurn;
			}

			// Without tents: the bearing from the fire of the right-hand edge of
			// the player's bedroll and its clutter (LZC_RingBedrollClutter,
			// measured around its anchor in the holding cell), as seen across the
			// fire.
			float RightOfPlayerBedroll() const
			{
				const float fwd = _layout.fwd[kFirstBedroll];
				const float side = _layout.side[kFirstBedroll];
				const float turn = _layout.turn[kFirstBedroll];
				const auto  bearingOf = [](float a_fwd, float a_side) {
                    return std::atan2(a_side, a_fwd - kSeatRadius) * 180.0f / std::numbers::pi_v<float>;
				};
				// The bedroll lies sideways: its ends are kBedrollHalfLength along
				// its own forward axis.
				float rightmost = std::max(bearingOf(fwd + kBedrollHalfLength * Cos(turn), side + kBedrollHalfLength * Sin(turn)),
					bearingOf(fwd - kBedrollHalfLength * Cos(turn), side - kBedrollHalfLength * Sin(turn)));

				auto* anchor = _props.Form<RE::TESObjectREFR>("LZC_RingBedrollClutterAnchor");
				if (!anchor) {
					return rightmost;
				}
				// The bedroll's facing, turned as its anchor is.
				const float         facing = turn + anchor->GetAngle().z * 180.0f / std::numbers::pi_v<float>;
				const RE::NiMatrix3 toAnchor = Placement::MatrixFromAngles(anchor->GetAngle()).Transpose();
				for (auto* item : _props.Forms<RE::TESObjectREFR>("LZC_RingBedrollClutter")) {
					if (!item) {
						continue;
					}
					// x right, y forward of the bedroll.
					const RE::NiPoint3 offset = toAnchor * (item->GetPosition() - anchor->GetPosition());
					rightmost = std::max(rightmost, bearingOf(fwd + offset.y * Cos(facing) + offset.x * Cos(facing + 90.0f),
														side + offset.y * Sin(facing) + offset.x * Sin(facing + 90.0f)));
				}
				return rightmost;
			}

			void SetSeat(std::size_t a_slot, RE::TESForm* a_seat, float a_bearing)
			{
				if (a_seat) {
					Set(a_slot, a_seat, kSeatRadius + kSeatRadius * Cos(a_bearing), kSeatRadius * Sin(a_bearing), a_bearing + 180.0f);
				}
			}

			// The player's seat across the fire, a follower seat for each bedroll
			// beyond theirs, kSeatStep apart. The two beside the player's are the
			// player's kind too, and placed even without followers.
			void LayOutSeats()
			{
				auto*      follower = _props.Form<RE::TESForm>("LZC_SitMarkerFollower");
				auto*      playerSeat = _props.Form<RE::TESForm>("LZC_SitMarkerPlayer");
				const auto colors = _props.Forms<RE::TESForm>("LZC_SeatColors");
				const auto color = static_cast<std::size_t>(std::max(_props.Int("SeatColor", 0) - 1, -1));
				if (color < colors.size() && colors[color]) {
					playerSeat = colors[color];
				}
				const std::int32_t followerSeats = follower ? std::clamp(_bedrolls - 1, 0, kMaxFollowerSeats) : 0;
				SetSeat(kPlayerSeat, playerSeat, 0.0f);
				_layout.lift[kPlayerSeat] = kPlayerSeatLift;
				const std::int32_t sideSeats = playerSeat ? std::max(followerSeats, 2) : followerSeats;
				for (std::int32_t f = 0; f < sideSeats; ++f) {
					const auto slot = kFirstFollowerSeat + static_cast<std::size_t>(f);
					const bool cushioned = f < 2 && playerSeat;
					SetSeat(slot, cushioned ? playerSeat : follower, static_cast<float>(FanPlace(f + 1)) * kSeatStep);
					if (cushioned) {
						_layout.lift[slot] = kPlayerSeatLift;
					}
				}
			}

			// A bedroll in the style chosen on the SKSE menu: vanilla, or one of
			// The bedroll for a bedroll style: Bedroll Alternative's green, blue,
			// red or mixed (1-4), or Ash Nomad's blue, red, purple or mixed (5-8),
			// each looked up at run time; nullptr for vanilla (0) or if that mod
			// isn't loaded. `colorSeq` steps through the three colours for mixed.
			static RE::TESForm* StyleBedroll(std::int32_t a_style, std::int32_t a_colorSeq)
			{
				auto*      dataHandler = RE::TESDataHandler::GetSingleton();
				const bool ash = a_style >= 5 && a_style <= 8;
				const auto plugin = ash ? kAshNomadBedrollsPlugin : kBedrollAltPlugin;
				if (a_style < 1 || a_style > 8 || !dataHandler ||
					(!dataHandler->LookupLoadedModByName(plugin) && !dataHandler->LookupLoadedLightModByName(plugin))) {
					return nullptr;
				}
				std::int32_t colour = ash ? a_style - 5 : a_style - 1;  // 0-2, or 3 for mixed
				if (colour == 3) {
					colour = a_colorSeq % 3;
				}
				constexpr RE::FormID alternative[] = { 0xD62, 0xD70, 0xD69 };  // green, blue, red
				constexpr RE::FormID ashNomad[] = { 0xD62, 0xD63, 0xD64 };     // Bedroll01_dunmerblue, red, purple
				return dataHandler->LookupForm((ash ? ashNomad : alternative)[colour], plugin);
			}

			// The bedroll for BedrollStyle (SKSE menu), or for a_style if given and
			// its mod is loaded (an addon tent's own style); vanilla otherwise.
			RE::TESForm* BedrollForm(std::int32_t a_colorSeq, std::int32_t a_style = 0) const
			{
				if (auto* bedroll = StyleBedroll(a_style, a_colorSeq)) {
					return bedroll;
				}
				if (auto* bedroll = StyleBedroll(_props.Int("BedrollStyle", 0), a_colorSeq)) {
					return bedroll;
				}
				return _props.Form<RE::TESForm>("LZC_BedrollPlaced");
			}

			// Side by side across the tent, facing the way it does: a small tent's
			// row centred, a large tent's laid out for a full tent and filled from
			// the left (as seen from the fire), turned round, pillows at the back.
			void SetTentBedroll(std::size_t a_slot, const Tent& a_tent, std::int32_t a_index, std::int32_t a_colorSeq)
			{
				const std::int32_t rowLength = a_tent.size == kLarge ? _props.Int("LargeTentCapacity", 4) : a_tent.beds;
				const float        back = a_tent.size == kLarge ? kLargeTentBedrollBack : 0.0f;
				const float        offset = RowPosition(rowLength - 1) / 2.0f - RowPosition(a_index);
				// An addon tent can bring its own bedrolls, taken in turn by color
				// step (the two of a touching pair matching).
				const auto   type = static_cast<std::size_t>(a_tent.color);
				const auto*  kind = type < _tentTypes.size() ? &_tentTypes[type] : nullptr;
				const auto*  own = kind && !kind->bedrolls.empty() ? &kind->bedrolls : nullptr;
				RE::TESForm* bedroll = own ? (*own)[static_cast<std::size_t>(a_colorSeq) % own->size()] :
				                             BedrollForm(a_colorSeq, kind ? kind->bedrollStyle : 0);
				Set(a_slot, bedroll,
					a_tent.fwd + offset * Cos(a_tent.turn + 90.0f) - back * Cos(a_tent.turn),
					a_tent.side + offset * Sin(a_tent.turn + 90.0f) - back * Sin(a_tent.turn),
					a_tent.size == kLarge ? a_tent.turn + 180.0f : a_tent.turn);
			}

			void LayOutTentBedrolls()
			{
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					_layout.forms[kFirstTent + t] = _tents[t].form;
					_layout.fwd[kFirstTent + t] = _tents[t].fwd;
					_layout.side[kFirstTent + t] = _tents[t].side;
					_layout.turn[kFirstTent + t] = _tents[t].turn;
					// 2 small, 3 large, or an addon tent's own set.
					const auto& kind = _tentTypes[static_cast<std::size_t>(_tents[t].color)];
					_layout.clutterSet[kFirstTent + t] = kind.clutterSet > 0 ? kind.clutterSet : 2 + _tents[t].size;
				}
				// Each touching pair, counted tent by tent, takes the next colour step.
				std::vector<std::int32_t> pairBase(_tents.size(), 0);
				for (std::size_t t = 1; t < _tents.size(); ++t) {
					pairBase[t] = pairBase[t - 1] + (_tents[t - 1].beds + 1) / 2;
				}
				// First each tent's first bedroll (the player's in the first tent),
				// then the rest.
				std::size_t next = 0;
				const auto  add = [&](std::size_t a_tent, std::int32_t a_index) {
                    SetTentBedroll(kFirstBedroll + next, _tents[a_tent], a_index, pairBase[a_tent] + a_index / 2);
                    _bedGroup[next] = static_cast<std::int32_t>(a_tent);
                    _bedOrder[next] = a_index + a_index / 2;  // pairs 2 apart, a pair's two 1 apart
                    ++next;
				};
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					add(t, 0);
				}
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					for (std::int32_t b = 1; b < _tents[t].beds; ++b) {
						add(t, b);
					}
				}
				// The rest outside the tents, on the player's side of the fire where
				// no tent stands: straight behind where the player stood, then to
				// either side, lying sideways like the bedrolls of a camp without
				// tents.
				const std::int32_t colorBase = pairBase.back() + (_tents.back().beds + 1) / 2;
				for (std::int32_t k = 0; k < _outside; ++k) {
					const std::int32_t place = FanPlace(k);
					const float        bearing = 180.0f + static_cast<float>(place) * kRingStep;
					const std::size_t  slot = kFirstBedroll + next;
					Set(slot, BedrollForm(colorBase + k), kSeatRadius + kBedrollRingRadius * Cos(bearing), kBedrollRingRadius * Sin(bearing),
						bearing + kBedrollRingYaw);
					_bedGroup[next] = static_cast<std::int32_t>(_tents.size());
					_bedOrder[next] = place * 2;
					_ringSeatTurn[next] = bearing + 180.0f;
					++next;
				}
			}

			// Without tents: a bedroll lying sideways at fan `place` round the fire,
			// its head clockwise, or anticlockwise if `pairHigh` (then the clockwise
			// one of a head-to-head pair with the place before it).
			void SetRingBedroll(std::int32_t a_index, std::int32_t a_place, bool a_pairHigh)
			{
				const std::size_t slot = kFirstBedroll + static_cast<std::size_t>(a_index);
				const float       bearing = static_cast<float>(a_place) * RingStep();
				_bedOrder[a_index] = a_pairHigh ? a_place * 2 - 1 : a_place * 2;
				Set(slot, BedrollForm(_bedOrder[a_index] + 10), kSeatRadius + kBedrollRingRadius * Cos(bearing), kBedrollRingRadius * Sin(bearing),
					bearing + kBedrollRingYaw + (a_pairHigh ? 180.0f : 0.0f));
				if (a_index == 0) {
					_layout.clutterSet[slot] = 4;  // only the player's bedroll
				}
				_ringSeatTurn[a_index] = bearing + 180.0f;
				_used[static_cast<std::size_t>(a_place + 3)] = true;
			}

			// The player's behind their seat, the rest beside it; the player and a
			// partner, and a follower couple, get neighbouring places head to head.
			void LayOutRing()
			{
				const bool partnerPair = _partners > 0 && _bedrolls >= 2;
				const bool couplePair = _couples > 0 && ((partnerPair && _bedrolls >= 4) || (!partnerPair && _bedrolls >= 3));
				SetRingBedroll(0, 0, partnerPair && couplePair);
				std::int32_t r = 1;
				if (partnerPair && couplePair) {
					SetRingBedroll(r++, -1, false);
				} else if (partnerPair) {
					SetRingBedroll(r++, 1, true);
				}
				if (couplePair) {
					SetRingBedroll(r, 1, false);
					SetRingBedroll(r + 1, 2, true);
					r += 2;
				}
				for (std::int32_t k = 1; r < _bedrolls && k < 7; ++k) {
					const std::int32_t place = FanPlace(k);
					if (!_used[static_cast<std::size_t>(place + 3)]) {
						SetRingBedroll(r++, place, false);
					}
				}
			}

			// A seat on each bedroll: facing the way its tent faces, or the fire.
			void LayOutBedrollSeats()
			{
				auto* seat = _props.Form<RE::TESForm>("LZC_BedrollSitMarker");
				if (!seat) {
					return;
				}
				for (std::int32_t b = 0; b < kMaxBedrolls; ++b) {
					if (!_layout.forms[kFirstBedroll + b]) {
						continue;
					}
					// Turned with its own bedroll, so the head is on the pillow
					// however the bedroll lies (bedrolls in a large tent, round
					// the fire and head to head all lie different ways).
					Set(kFirstBedrollSeat + b, seat, _layout.fwd[kFirstBedroll + b], _layout.side[kFirstBedroll + b],
						_layout.turn[kFirstBedroll + b] + kBedrollSeatTurn);
					_layout.lift[kFirstBedrollSeat + b] = kBedrollSeatLift;
				}
			}

			void CollectClutter()
			{
				constexpr std::array<std::pair<std::string_view, std::string_view>, 6> sets{ {
					{ "LZC_FireClutterAnchor", "LZC_FireClutter" },
					{ "LZC_SmallTentClutterAnchor", "LZC_SmallTentClutter" },
					{ "LZC_LargeTentClutterAnchor", "LZC_LargeTentClutter" },
					{ "LZC_RingBedrollClutterAnchor", "LZC_RingBedrollClutter" },
					{ "LZC_TubClutterAnchor", "LZC_TubClutter" },
					{ "LZC_FireClutterAnchor2", "LZC_FireClutter2" },
				} };
				// The tub water isn't copied: the script moves it onto the tub.
				auto* water = _props.Form<RE::TESObjectREFR>("LZC_TubWater");
				for (std::size_t s = 0; s < sets.size(); ++s) {
					_layout.setAnchors.push_back(_props.Form<RE::TESObjectREFR>(sets[s].first));
					for (auto* item : _props.Forms<RE::TESObjectREFR>(sets[s].second)) {
						if (item && item != water) {
							_layout.setItems.push_back(item);
							_layout.itemSet.push_back(static_cast<std::int32_t>(s) + 1);
						}
					}
				}
				// Addon tents' own clutter sets, after the six above.
				std::set<RE::TESBoundObject*> storageBases;
				for (auto* marker : _props.Forms<RE::TESObjectREFR>("LZC_CampStorageMarkers")) {
					_layout.storageMarkers.push_back(marker);
					if (marker && marker->GetBaseObject()) {
						storageBases.insert(marker->GetBaseObject());
					}
				}
				// Each addon set once, however many tents share it, in set number
				// order (sets are found by position); a number no tent uses any
				// more (a patch gave its tents other clutter) stays empty.
				// (Tent sets and fire sets alike.)
				std::map<std::int32_t, std::pair<RE::TESObjectREFR*, const std::vector<RE::TESObjectREFR*>*>> addonSets;
				for (const auto& kind : _tentTypes) {
					if (kind.clutterSet > 0) {
						addonSets.try_emplace(kind.clutterSet, kind.anchor, &kind.items);
					}
					if (kind.fireSet > 0) {
						addonSets.try_emplace(kind.fireSet, kind.fireAnchor, &kind.fireItems);
					}
				}
				const std::int32_t                    lastSet = addonSets.empty() ? 6 : addonSets.rbegin()->first;
				const std::vector<RE::TESObjectREFR*> none;
				for (std::int32_t set = 7; set <= lastSet; ++set) {
					const auto it = addonSets.find(set);
					const bool used = it != addonSets.end();
					_layout.setAnchors.push_back(used ? it->second.first : nullptr);
					for (auto* item : used ? *it->second.second : none) {
						_layout.setItems.push_back(item);
						_layout.itemSet.push_back(set);
						// A copy of the storage backpack in it marks the storage spot.
						if (storageBases.contains(item->GetBaseObject())) {
							_layout.storageMarkers.push_back(item);
						}
					}
				}
			}

			// Moves a persistent piece to a_target, disabled first and enabled
			// after, so it loads afresh there.
			static void MoveFresh(RE::TESObjectREFR* a_ref, RE::TESObjectREFR* a_target, std::string_view a_what)
			{
				a_ref->Disable();
				a_ref->MoveTo(a_target);
				a_ref->Enable(false);
				const auto  pos = a_ref->GetPosition();
				const auto* cell = a_ref->GetParentCell();
				logger::info("Moved the {} ({:08X}) to ({:.0f}, {:.0f}, {:.0f}) in cell {:08X}{}", a_what, a_ref->GetFormID(), pos.x, pos.y, pos.z,
					cell ? cell->GetFormID() : 0, a_ref->IsDisabled() ? ", still disabled" : "");
			}

			// A new reference of a_base where a_target stands, turned like it.
			static RE::TESObjectREFR* PlaceFresh(RE::TESBoundObject* a_base, RE::TESObjectREFR* a_target, std::string_view a_what)
			{
				if (!a_base || !a_target) {
					return nullptr;
				}
				const auto placed = a_target->PlaceObjectAtMe(a_base, false);
				auto*      ref = placed.get();
				if (ref) {
					ref->SetAngle(a_target->GetAngle());
					const auto pos = ref->GetPosition();
					logger::info("Placed a {} ({:08X}) at ({:.0f}, {:.0f}, {:.0f})", a_what, ref->GetFormID(), pos.x, pos.y, pos.z);
				}
				return ref;
			}

			// The pieces that move to the camp rather than being placed.
			void Settle(std::vector<RE::TESObjectREFR*>& a_refs) const
			{
				auto* campfire = a_refs[kCampfire];

				// The cook box over the fire, enabled only while it's lit. If its
				// base (LZC_CookActivator) has a model (an invisible collision
				// box), a new one is placed with each camp; otherwise the one
				// persistent box, whose clickable box is set on the reference
				// itself, moves here. (Moving it has left it impossible to
				// activate on some setups.)
				auto*              cookBoxRef = _props.Bool("PlaceCampfire", true) ? _props.Form<RE::TESObjectREFR>("LZC_CookBox") : nullptr;
				RE::TESObjectREFR* cookBox = nullptr;
				if (cookBoxRef && campfire) {
					auto*       base = cookBoxRef->GetBaseObject();
					const auto* model = base ? base->As<RE::TESModel>() : nullptr;
					if (model && model->GetModel() && *model->GetModel()) {
						cookBox = PlaceFresh(base, campfire, "cook box");
						if (cookBox) {
							a_refs.push_back(cookBox);  // deleted with the camp
						}
					} else {
						MoveFresh(cookBoxRef, campfire, "cook box");
						cookBox = cookBoxRef;
					}
				}
				built.cookBox = cookBox;
				// With the firewood mechanic the fire starts unlit: the firepit shows.
				if (a_refs[kFirepit]) {
					a_refs[kFirepit]->Enable(false);
					for (auto* ref : { campfire, a_refs[kLight], cookBox }) {
						if (ref) {
							ref->Disable();
						}
					}
				}

				std::vector<RE::TESObjectREFR*> beds(a_refs.begin() + kFirstBedroll, a_refs.begin() + kFirstBedroll + kMaxBedrolls);
				std::vector<RE::TESObjectREFR*> seats(a_refs.begin() + kFirstBedrollSeat, a_refs.begin() + kFirstBedrollSeat + kMaxBedrolls);
				BedrollSeats::Set(beds, seats);

				// A new backpack on the spot marked for it, which goes. It's a copy
				// of LZC_CampStorage that opens LZC_CampStorage itself, which stays
				// in the holding cell with the things in it (see the camp script).
				auto*              storageRef = _props.Form<RE::TESObjectREFR>("LZC_CampStorage");
				RE::TESObjectREFR* storage = nullptr;
				if (auto* spot = a_refs[kSlotCount]) {
					if (storageRef) {
						storage = PlaceFresh(storageRef->GetBaseObject(), spot, "storage backpack");
						if (storage) {
							a_refs.push_back(storage);  // opens the real storage: see Events.cpp
						}
					}
					spot->Disable();
					spot->SetDelete(true);
					a_refs[kSlotCount] = nullptr;
				}

				if (_props.Bool("HideGrassUnderCamp", true)) {
					// The storage borrows the spot's slot so its grass goes too.
					auto refs = a_refs;
					refs[kSlotCount] = storage;
					GrassHider::Hide(refs);
				} else {
					logger::info("Grass: not cleared, \"Clear grass around the camp\" (HideGrassUnderCamp) is off");
				}

				if (auto* tub = a_refs[kTub]) {
					MoveOntoTub(tub, _props.Form<RE::TESObjectREFR>("LZC_TubTrigger"), _props.Floats("tubTriggerOffset"));
					MoveOntoTub(tub, _props.Form<RE::TESObjectREFR>("LZC_TubWater"), _props.Floats("tubWaterOffset"), TubRimDrop(tub));
				}

				auto* marker = _props.Form<RE::TESObjectREFR>("LZC_CampMapMarker");
				if (marker && campfire && _props.Bool("PlaceMapMarker", true)) {
					marker->MoveTo(campfire);
					marker->Enable(false);
					if (auto* extra = marker->extraList.GetByType<RE::ExtraMapMarker>(); extra && extra->mapData) {
						extra->mapData->SetVisible(true);
						extra->mapData->flags.set(RE::MapMarkerData::Flag::kCanTravelTo);
					}
					ListMapMarker(marker);
				}
			}

			// With BedrollsStartRolled, each bedroll is placed as usual, with its
			// seat, but disabled, and a rolled-up bedroll dropped where it lies.
			// Its clutter (the player's, without tents) shows from the start. Activating that (or a follower going to bed in it) unrolls
			// the bedroll where it was laid out, however far the rolled one has
			// been knocked: see Unroll. The rolled ones are added to a_refs, so
			// packing up takes them.
			void RollUp(std::vector<RE::TESObjectREFR*>& a_refs, std::vector<std::int32_t>& a_owners) const
			{
				built.rolledKits.assign(kMaxBedrolls, nullptr);
				auto* plain = _props.Form<RE::TESBoundObject>("LZC_BedrollRolled");
				if (!plain || !_props.Bool("BedrollsStartRolled", true)) {
					return;
				}
				// Each Bedroll Alternative colour has its own rolled-up bedroll.
				const auto colors = _props.Forms<RE::TESBoundObject>("LZC_BedrollRolledColors");  // green, blue, red
				auto*      dataHandler = RE::TESDataHandler::GetSingleton();
				const auto rolledFor = [&](const RE::TESObjectREFR* a_bed) -> RE::TESBoundObject* {
					const auto*          base = a_bed->GetBaseObject();
					constexpr RE::FormID ids[] = { 0xD62, 0xD70, 0xD69 };  // green, blue, red
					for (std::size_t c = 0; dataHandler && base && c < std::size(ids) && c < colors.size(); ++c) {
						if (colors[c] && base == dataHandler->LookupForm(ids[c], kBedrollAltPlugin)) {
							return colors[c];
						}
					}
					return plain;
				};
				for (std::int32_t b = 0; b < kMaxBedrolls; ++b) {
					auto* bed = a_refs[kFirstBedroll + b];
					if (!bed) {
						continue;
					}
					bed->Disable();
					if (auto* seat = a_refs[kFirstBedrollSeat + b]) {
						seat->Disable();
					}
					const auto kit = bed->PlaceObjectAtMe(rolledFor(bed), false);
					if (!kit) {
						continue;
					}
					// Tilted like its bedroll (to the slope, with AlignToSlope). Those
					// outside the tents are turned round to face the fire.
					kit->data.angle = bed->data.angle;
					if (!_tents.empty() && _bedGroup[b] >= static_cast<std::int32_t>(_tents.size())) {
						kit->data.angle.z += std::numbers::pi_v<float>;
					}
					kit->SetPosition(bed->GetPosition() + RE::NiPoint3{ 0.0f, 0.0f, kRolledDrop });
					a_refs.push_back(kit.get());
					a_owners.push_back(-1);
					built.rolledKits[b] = kit.get();
				}
			}

			// What the script reads back for the follower quest and its state.
			void Report(const std::vector<RE::TESObjectREFR*>& a_refs) const
			{
				built.fireSeats.assign(1, a_refs[kPlayerSeat]);
				built.firePlaces.assign(1, 0);
				for (std::int32_t i = 0; i < kMaxFollowerSeats; ++i) {
					built.fireSeats.push_back(a_refs[kFirstFollowerSeat + i]);
					built.firePlaces.push_back(FanPlace(i + 1));
				}
				built.beds.assign(a_refs.begin() + kFirstBedroll, a_refs.begin() + kFirstBedroll + kMaxBedrolls);
				built.bedSeats.assign(a_refs.begin() + kFirstBedrollSeat, a_refs.begin() + kFirstBedrollSeat + kMaxBedrolls);
				built.bedGroups = _bedGroup;
				// Whose kit each bedroll's tent came from (a follower's index), or -1.
				built.bedOwners.assign(kMaxBedrolls, -1);
				for (std::int32_t b = 0; b < kMaxBedrolls; ++b) {
					const auto group = static_cast<std::size_t>(_bedGroup[b]);
					if (a_refs[kFirstBedroll + b] && group < _tents.size()) {
						built.bedOwners[b] = _tents[group].owner;
					}
				}
				built.bedOrders = _bedOrder;

				// The seats among the tub clutter: placed furniture whose base is in
				// LZC_TubClutter.
				std::set<RE::TESBoundObject*> tubBases;
				for (auto* item : _props.Forms<RE::TESObjectREFR>("LZC_TubClutter")) {
					if (auto* base = item ? item->GetBaseObject() : nullptr; base && base->Is(RE::FormType::Furniture)) {
						tubBases.insert(base);
					}
				}
				built.tubSeats.clear();
				for (std::size_t i = kSlotCount + 1; i < a_refs.size() && built.tubSeats.size() < 4; ++i) {
					if (a_refs[i] && tubBases.contains(a_refs[i]->GetBaseObject())) {
						built.tubSeats.push_back(a_refs[i]);
					}
				}
				built.tubSeats.resize(4, nullptr);
			}

			const Properties&                                 _props;
			RE::Actor*                                        _player;
			std::int32_t                                      _partners;
			std::int32_t                                      _couples;
			struct TentType
			{
				RE::TESForm*                    kit{ nullptr };
				RE::TESBoundObject*             tent{ nullptr };
				std::int32_t                    size{ kSmall };
				RE::TESObjectREFR*              anchor{ nullptr };  // an addon tent's own clutter set, if any
				std::vector<RE::TESObjectREFR*> items;
				std::int32_t                    clutterSet{ 0 };  // its set number, or 0 for the plugin's
				std::vector<RE::TESForm*>       bedrolls;  // an addon tent's own bedrolls, taken in turn, if any
				std::int32_t                    bedrollStyle{ 0 };  // or its own bedroll style (as BedrollStyle), if not 0
				RE::TESObjectREFR*              fireAnchor{ nullptr };  // the fire's clutter when it's the player's tent, if any
				std::vector<RE::TESObjectREFR*> fireItems;
				std::int32_t                    fireSet{ 0 };  // its set number, 0 for the plugin's, -1 for none
			};
			std::vector<TentType>                             _tentTypes;
			RE::TESObjectREFR::InventoryCountMap              _counts;  // the player's and the followers' together
			std::vector<RE::Actor*>                           _followers;
			std::vector<RE::TESObjectREFR::InventoryCountMap> _followerCounts;
			Papyrus::Camp::Layout                             _layout;
			std::vector<Tent>                                 _tents;
			std::int32_t                                      _bedrollsCarried{ 0 };
			std::int32_t                                      _bedrolls{ 0 };
			std::int32_t                                      _outside{ 0 };  // with tents: bedrolls outside them
			std::vector<std::int32_t>                         _bedGroup;
			std::vector<std::int32_t>                         _bedOrder;
			std::vector<float>                                _ringSeatTurn;
			std::array<bool, 7>                               _used{};  // ring places -3 to 3, at place + 3
		};

		// --- Native functions ----------------------------------------------------

		// Sets up the camp for the quest's script; returns the placed references
		// (empty if it couldn't: see GetBuildStatus).
		// The followers in the follower quest's Followers aliases, in the
		// script's order, read here rather than trusting what the script passed
		// (an older compiled follower script passes none).
		std::vector<RE::Actor*> FollowerActors()
		{
			static RE::FormID quest = 0;
			auto              followers = ScriptObjects::FindQuestScript(kPluginPrefix, "LZC_CampFollowersScript", quest);
			const auto*       value = followers ? ScriptObjects::AutoProperty(followers.get(), "Followers") : nullptr;
			const auto        aliases = value && value->IsArray() ? value->GetArray() : nullptr;
			std::vector<RE::Actor*> actors;
			for (std::size_t i = 0; aliases && i < aliases->size(); ++i) {
				auto* alias = (*aliases)[i].Unpack<RE::BGSRefAlias*>();
				actors.push_back(alias ? alias->GetActorReference() : nullptr);
			}
			return actors;
		}

		std::vector<RE::ObjectRefHandle> newTents;  // placed by the last BuildCamp

		// With TentCollision off: the tent's loaded 3D stops colliding. It
		// collides again when its 3D next loads without this.
		void RemoveCollision(RE::TESObjectREFR* a_tent)
		{
			if (auto* root = a_tent ? a_tent->Get3D() : nullptr) {
				root->SetCollisionLayer(RE::COL_LAYER::kNonCollidable);
			}
		}

		std::vector<RE::TESObjectREFR*> BuildCamp(RE::StaticFunctionTag*, RE::TESQuest* a_quest, std::int32_t a_partners, std::int32_t a_couples,
			std::vector<RE::Actor*> a_followers)
		{
			if (std::ranges::none_of(a_followers, [](RE::Actor* a_actor) { return a_actor != nullptr; })) {
				a_followers = FollowerActors();
			}
			built = {};
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!a_quest || !policy || !player) {
				return {};
			}
			RE::BSTSmartPointer<RE::BSScript::Object> object;
			vm->FindBoundObject(policy->GetHandleForObject(a_quest->GetFormType(), a_quest), kScript, object);
			if (!object) {
				return {};
			}
			const Properties props(object.get());
			auto refs = Builder(props, player, a_followers, a_partners, a_couples).Run();
			// Until the script stores them in placedRefs, the new tents are
			// known only here.
			newTents.clear();
			for (std::size_t t = 0; t < static_cast<std::size_t>(kMaxTents) && kFirstTent + t < refs.size(); ++t) {
				if (auto* tent = refs[kFirstTent + t]) {
					newTents.push_back(tent->GetHandle());
					if (!props.Bool("TentCollision", true)) {
						RemoveCollision(tent);
					}
				}
			}
			return refs;
		}

		// The tents the camp in a_camp has up, or that the last BuildCamp
		// placed.
		std::vector<RE::TESObjectREFR*> CampTents(RE::BSScript::Object* a_camp)
		{
			std::vector<RE::TESObjectREFR*> tents;
			for (const auto& handle : newTents) {
				if (auto ref = handle.get()) {
					tents.push_back(ref.get());
				}
			}
			const auto* placedValue = a_camp ? a_camp->GetVariable("placedRefs") : nullptr;
			const auto  placed = placedValue && placedValue->IsArray() ? placedValue->GetArray() : nullptr;
			for (std::size_t t = 0; placed && t < static_cast<std::size_t>(kMaxTents) && kFirstTent + t < placed->size(); ++t) {
				auto* form = FormValue(&(*placed)[kFirstTent + t]);
				auto* tent = form ? form->As<RE::TESObjectREFR>() : nullptr;
				if (tent && std::ranges::find(tents, tent) == tents.end()) {
					tents.push_back(tent);
				}
			}
			return tents;
		}

		// Unrolls camp bedroll a_bed (0 to kMaxBedrolls - 1) if it's still
		// rolled up: enables the bedroll, its seat and its clutter, where they
		// were laid out, and deletes the rolled-up one. Reads and updates the
		// camp script's placedRefs, PlacedOwners and RolledKits.
		void Unroll(RE::BSScript::Object* a_camp, std::size_t a_bed)
		{
			const auto* rolledValue = ScriptObjects::AutoProperty(a_camp, "RolledKits");
			const auto  rolled = rolledValue && rolledValue->IsArray() ? rolledValue->GetArray() : nullptr;
			auto*       kitForm = rolled && a_bed < rolled->size() ? FormValue(&(*rolled)[a_bed]) : nullptr;
			auto*       kit = kitForm ? kitForm->As<RE::TESObjectREFR>() : nullptr;
			if (!kit) {
				return;
			}
			const auto* placedValue = a_camp->GetVariable("placedRefs");
			const auto  placed = placedValue && placedValue->IsArray() ? placedValue->GetArray() : nullptr;
			const auto* ownersValue = ScriptObjects::AutoProperty(a_camp, "PlacedOwners");
			const auto  owners = ownersValue && ownersValue->IsArray() ? ownersValue->GetArray() : nullptr;
			const auto  slot = static_cast<std::int32_t>(kFirstBedroll + a_bed);
			for (std::size_t k = 0; placed && k < placed->size(); ++k) {
				auto* form = FormValue(&(*placed)[k]);
				auto* ref = form ? form->As<RE::TESObjectREFR>() : nullptr;
				if (!ref) {
					continue;
				}
				if (ref == kit) {
					(*placed)[k].SetNone();
					continue;
				}
				const bool owned = k > kSlotCount && owners && k < owners->size() && (*owners)[k].IsInt() && (*owners)[k].GetSInt() == slot;
				if (static_cast<std::int32_t>(k) == slot || k == kFirstBedrollSeat + a_bed || owned) {
					ref->Enable(false);
				}
			}
			kit->Disable();
			kit->SetDelete(true);
			(*rolled)[a_bed].SetNone();
		}

		RE::BSTSmartPointer<RE::BSScript::Object> CampScript()
		{
			static RE::FormID quest = 0;
			return ScriptObjects::FindQuestScript(kPluginPrefix, kScript, quest);
		}

		// Unrolls the camp bedrolls among a_beds that are still rolled up (for
		// followers going to bed).
		void UnrollBeds(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> a_beds)
		{
			auto        camp = CampScript();
			const auto* placedValue = camp ? camp->GetVariable("placedRefs") : nullptr;
			const auto  placed = placedValue && placedValue->IsArray() ? placedValue->GetArray() : nullptr;
			if (!placed) {
				return;
			}
			for (std::int32_t b = 0; b < kMaxBedrolls; ++b) {
				const auto index = kFirstBedroll + b;
				auto*      form = index < placed->size() ? FormValue(&(*placed)[index]) : nullptr;
				auto*      bed = form ? form->As<RE::TESObjectREFR>() : nullptr;
				if (bed && std::ranges::find(a_beds, bed) != a_beds.end()) {
					Unroll(camp.get(), static_cast<std::size_t>(b));
				}
			}
		}

		std::int32_t GetBuildStatus(RE::StaticFunctionTag*) { return built.status; }
		std::vector<RE::TESObjectREFR*> GetRolledKits(RE::StaticFunctionTag*) { return built.rolledKits; }
		std::vector<std::int32_t>       GetPlacedOwners(RE::StaticFunctionTag*) { return built.owners; }
		RE::TESObjectREFR*              GetCookBox(RE::StaticFunctionTag*) { return built.cookBox; }
		std::vector<RE::TESObjectREFR*> GetFireSeats(RE::StaticFunctionTag*) { return built.fireSeats; }
		std::vector<std::int32_t>       GetFirePlaces(RE::StaticFunctionTag*) { return built.firePlaces; }
		std::vector<RE::TESObjectREFR*> GetCampBeds(RE::StaticFunctionTag*) { return built.beds; }
		std::vector<RE::TESObjectREFR*> GetBedSeats(RE::StaticFunctionTag*) { return built.bedSeats; }
		std::vector<std::int32_t>       GetBedGroups(RE::StaticFunctionTag*) { return built.bedGroups; }
		std::vector<std::int32_t>       GetBedOwners(RE::StaticFunctionTag*) { return built.bedOwners; }
		std::vector<std::int32_t>       GetBedOrders(RE::StaticFunctionTag*) { return built.bedOrders; }
		std::vector<RE::TESObjectREFR*> GetTubSeats(RE::StaticFunctionTag*) { return built.tubSeats; }
	}

	namespace
	{
		// Campfire (and through it Frostfall), looked up at run time: not a
		// master. FormIDs within Campfire.esm.
		constexpr auto       kCampfirePlugin = "Campfire.esm"sv;
		constexpr RE::FormID kTentWaterproof = 0x03AB3A;  // isCampfireTentWaterproof
		constexpr RE::FormID kTentWarm = 0x03AB3B;        // isCampfireTentWarm

		RE::ObjectRefHandle campfireTent;  // the camp tent last reported to Campfire as the player's

		// The camp tent the player stands in, if any.
		RE::TESObjectREFR* TentAroundPlayer(RE::BSScript::Object* a_camp)
		{
			auto*       player = RE::PlayerCharacter::GetSingleton();
			const auto* deployed = ScriptObjects::AutoProperty(a_camp, "CampDeployed");
			const auto* placedValue = a_camp->GetVariable("placedRefs");
			const auto  placed = placedValue && placedValue->IsArray() ? placedValue->GetArray() : nullptr;
			if (!player || !deployed || !deployed->GetBool() || !placed) {
				return nullptr;
			}
			const auto large = Properties(a_camp).Forms<RE::TESBoundObject>("LZC_LargeTents");
			const auto pos = player->GetPosition();
			for (std::size_t t = 0; t < static_cast<std::size_t>(kMaxTents); ++t) {
				const auto index = kFirstTent + t;
				auto*      form = index < placed->size() ? FormValue(&(*placed)[index]) : nullptr;
				auto*      tent = form ? form->As<RE::TESObjectREFR>() : nullptr;
				if (!tent || tent->IsDisabled() || tent->GetWorldspace() != player->GetWorldspace()) {
					continue;
				}
				const bool  isLarge = std::ranges::find(large, tent->GetBaseObject()) != large.end();
				const float halfWidth = isLarge ? kLargeTentHalfWidth : kSmallTentHalfWidth;
				const float halfDepth = isLarge ? kLargeTentHalfDepth : kSmallTentHalfDepth;
				const auto  centre = tent->GetPosition();
				const float a = tent->GetAngleZ();
				const float dx = pos.x - centre.x;
				const float dy = pos.y - centre.y;
				const float right = dx * std::cos(a) - dy * std::sin(a);
				const float forward = dx * std::sin(a) + dy * std::cos(a);
				if (std::abs(right) < halfWidth && std::abs(forward) < halfDepth && std::abs(pos.z - centre.z) < 300.0f) {
					return tent;
				}
			}
			return nullptr;
		}

		// Tells Campfire which camp tent the player is in (or that they've
		// left it), as its own tents do, so Frostfall counts it as shelter.
		void UpdateCampfireTent()
		{
			const auto camp = CampScript();
			auto*      tent = camp ? TentAroundPlayer(camp.get()) : nullptr;
			if (tent == campfireTent.get().get()) {
				return;
			}
			campfireTent = tent ? tent->GetHandle() : RE::ObjectRefHandle();
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (!vm) {
				return;
			}
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			vm->DispatchStaticCall("_CampInternal", "SetCurrentTent", RE::MakeFunctionArguments(std::move(tent)), callback);
		}
	}

	void SetUpCampfireTents()
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || (!dataHandler->LookupLoadedModByName(kCampfirePlugin) && !dataHandler->LookupLoadedLightModByName(kCampfirePlugin))) {
			return;
		}
		// Campfire's warm and waterproof tent keywords on the camp's tents.
		auto  camp = CampScript();
		auto* warm = dataHandler->LookupForm<RE::BGSKeyword>(kTentWarm, kCampfirePlugin);
		auto* waterproof = dataHandler->LookupForm<RE::BGSKeyword>(kTentWaterproof, kCampfirePlugin);
		if (camp) {
			const Properties props(camp.get());
			for (const auto* list : { "LZC_SmallTents", "LZC_LargeTents" }) {
				for (auto* tent : props.Forms<RE::TESObjectACTI>(list)) {
					for (auto* keyword : { warm, waterproof }) {
						if (tent && keyword && !tent->HasKeyword(keyword)) {
							tent->AddKeyword(keyword);
						}
					}
				}
			}
		}
		campfireTent = {};

		static std::once_flag polling;
		std::call_once(polling, [] {
			logger::info("Campfire found: the camp's tents count as Campfire tents (Frostfall shelter)");
			std::thread([] {
				for (;;) {
					std::this_thread::sleep_for(1s);
					if (auto* tasks = SKSE::GetTaskInterface()) {
						tasks->AddTask(UpdateCampfireTent);
					}
				}
			}).detach();
		});
	}

	void ListCampMapMarker()
	{
		auto        camp = CampScript();
		const auto* value = camp ? ScriptObjects::AutoProperty(camp.get(), "LZC_CampMapMarker") : nullptr;
		auto*       form = FormValue(value);
		auto*       marker = form ? form->As<RE::TESObjectREFR>() : nullptr;
		if (marker && !marker->IsDisabled()) {
			ListMapMarker(marker);
		}
	}

	bool UnrollKit(RE::TESObjectREFR* a_kit)
	{
		auto        camp = a_kit ? CampScript() : nullptr;
		const auto* rolledValue = camp ? ScriptObjects::AutoProperty(camp.get(), "RolledKits") : nullptr;
		const auto  rolled = rolledValue && rolledValue->IsArray() ? rolledValue->GetArray() : nullptr;
		for (std::size_t b = 0; rolled && b < rolled->size(); ++b) {
			if (FormValue(&(*rolled)[b]) == a_kit) {
				Unroll(camp.get(), b);
				return true;
			}
		}
		return false;
	}

	void OnObjectLoaded(RE::TESObjectREFR* a_ref)
	{
		auto camp = a_ref ? CampScript() : nullptr;
		const auto* collision = camp ? ScriptObjects::AutoProperty(camp.get(), "TentCollision") : nullptr;
		if (!collision || !collision->IsBool() || collision->GetBool()) {
			return;
		}
		const auto tents = CampTents(camp.get());
		if (std::ranges::find(tents, a_ref) != tents.end()) {
			RemoveCollision(a_ref);
		}
	}

	void ApplyTentCollision(bool a_collide)
	{
		for (auto* tent : CampTents(CampScript().get())) {
			if (!tent || tent->IsDisabled()) {
				continue;
			}
			if (a_collide) {
				// Loading its 3D afresh brings its collision back.
				tent->Disable();
				tent->Enable(false);
			} else {
				RemoveCollision(tent);
			}
		}
	}

	void Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("BuildCamp"sv, kClass, BuildCamp);
		a_vm->RegisterFunction("GetBuildStatus"sv, kClass, GetBuildStatus);
		a_vm->RegisterFunction("GetFireSeats"sv, kClass, GetFireSeats);
		a_vm->RegisterFunction("GetFirePlaces"sv, kClass, GetFirePlaces);
		a_vm->RegisterFunction("GetCampBeds"sv, kClass, GetCampBeds);
		a_vm->RegisterFunction("GetBedSeats"sv, kClass, GetBedSeats);
		a_vm->RegisterFunction("GetBedGroups"sv, kClass, GetBedGroups);
		a_vm->RegisterFunction("GetBedOwners"sv, kClass, GetBedOwners);
		a_vm->RegisterFunction("GetBedOrders"sv, kClass, GetBedOrders);
		a_vm->RegisterFunction("GetTubSeats"sv, kClass, GetTubSeats);
		a_vm->RegisterFunction("GetRolledKits"sv, kClass, GetRolledKits);
		a_vm->RegisterFunction("GetPlacedOwners"sv, kClass, GetPlacedOwners);
		a_vm->RegisterFunction("GetCookBox"sv, kClass, GetCookBox);
		a_vm->RegisterFunction("UnrollBeds"sv, kClass, UnrollBeds);
	}
}
