#include "CampLayout.h"

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
		constexpr auto kPluginPrefix = "Wayfarer - A Convenient Camping Mod."sv;
		constexpr RE::FormID kXMarker = 0x3B;  // Skyrim.esm XMarker: the fire's place when there's no fire
		constexpr auto kBedrollAltPlugin = "Bedroll Alternative - BOS Color Variance.esp"sv;

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
		constexpr float kBedrollSeatTurn = 90.0f;  // the marker on each bedroll, clockwise from facing the fire/tent
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
		constexpr float kRolledDrop = 10.0f;  // a rolled-up bedroll starts this far above its bedroll

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
			std::int32_t        color{ 0 };
			std::int32_t        beds{ 0 };
			std::int32_t        owner{ -1 };  // the follower whose kit it is (index), or -1 for the player's
			float               fwd{ 0.0f };
			float               side{ 0.0f };
			float               turn{ 0.0f };
		};

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
					for (const auto& [object, count] : _followerCounts[f]) {
						_counts[object] += count;
					}
				}
			}

			std::vector<RE::TESObjectREFR*> Run()
			{
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

			// The kits each camper carries, to the log.
			void LogKits() const
			{
				std::vector<RE::TESForm*> kits{ _props.Form<RE::TESForm>("LZC_BedrollKit") };
				for (auto* kit : _props.Forms<RE::TESForm>("LZC_SmallTentKits")) {
					kits.push_back(kit);
				}
				for (auto* kit : _props.Forms<RE::TESForm>("LZC_LargeTentKits")) {
					kits.push_back(kit);
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
				const auto largeKits = _props.Forms<RE::TESForm>("LZC_LargeTentKits");
				const auto smallKits = _props.Forms<RE::TESForm>("LZC_SmallTentKits");
				for (std::size_t color = 0; color < largeKits.size() && _tents.empty() && _bedrollsCarried > 0; ++color) {
					if (largeKits[color] && Count(largeKits[color]) > 0) {
						_tents.push_back({ nullptr, kLarge, static_cast<std::int32_t>(color) });
						_tents.back().owner = TakeKit(largeKits[color]);
					}
				}
				const auto room = [&] {
					return static_cast<std::int32_t>(_tents.size()) < kMaxTents &&
					       static_cast<std::int32_t>(_tents.size()) < _bedrollsCarried;
				};
				for (std::size_t color = 0; color < smallKits.size() && room(); ++color) {
					for (std::int32_t have = smallKits[color] ? Count(smallKits[color]) : 0; have > 0 && room(); --have) {
						_tents.push_back({ nullptr, kSmall, static_cast<std::int32_t>(color) });
						_tents.back().owner = TakeKit(smallKits[color]);
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
				for (std::size_t t = 0; t < _tents.size(); ++t) {
					auto&       tent = _tents[t];
					const auto& forms = tent.size == kLarge ? largeTents : smallTents;
					if (!forms.empty()) {
						const auto color = static_cast<std::size_t>(tent.color);
						tent.form = color < forms.size() && forms[color] ? forms[color] : forms[0];
					}
					const float bearing = FanBearing(static_cast<std::int32_t>(t));
					const float distance = tent.size == kLarge ? kLargeTentDistance : kSmallTentDistance;
					tent.fwd = kSeatRadius + distance * Cos(bearing);
					tent.side = distance * Sin(bearing);
					tent.turn = bearing + 180.0f;
					if (t > 0 && _tents[0].size == kLarge) {
						const float sideSign = bearing < 0.0f ? -1.0f : 1.0f;
						tent.fwd = kSeatRadius + kSideTentForward;
						tent.side = sideSign * kSideTentSide;
						tent.turn = 180.0f + sideSign * kSideTentTurn;
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
				_layout.clutterSet[kCampfire] = fireSet;
				if (_props.Bool("PlaceCampfireLight", true)) {
					Set(kLight, _props.Form<RE::TESForm>("LZC_CampfireLight"), kSeatRadius, 0.0f, 0.0f);
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
				const std::int32_t sideSeats = playerSeat ? std::max(followerSeats, 2) : followerSeats;
				for (std::int32_t f = 0; f < sideSeats; ++f) {
					SetSeat(kFirstFollowerSeat + static_cast<std::size_t>(f), f < 2 && playerSeat ? playerSeat : follower, static_cast<float>(FanPlace(f + 1)) * kSeatStep);
				}
			}

			// A bedroll in the style chosen on the SKSE menu: vanilla, or one of
			// Bedroll Alternative's colours. `colorSeq` steps through green, blue
			// and red for mixed colours.
			RE::TESForm* BedrollForm(std::int32_t a_colorSeq) const
			{
				auto*        vanilla = _props.Form<RE::TESForm>("LZC_BedrollPlaced");
				std::int32_t style = _props.Int("BedrollStyle", 0);
				auto*        dataHandler = RE::TESDataHandler::GetSingleton();
				if (style < 1 || style > 4 || !dataHandler ||
					(!dataHandler->LookupLoadedModByName(kBedrollAltPlugin) && !dataHandler->LookupLoadedLightModByName(kBedrollAltPlugin))) {
					return vanilla;
				}
				if (style == 4) {
					style = a_colorSeq % 3 + 1;
				}
				const RE::FormID id = style == 2 ? 0xD70 : style == 3 ? 0xD69 : 0xD62;  // blue, red, green
				auto*            bedroll = dataHandler->LookupForm(id, kBedrollAltPlugin);
				return bedroll ? bedroll : vanilla;
			}

			// Side by side across the tent, facing the way it does: a small tent's
			// row centred, a large tent's laid out for a full tent and filled from
			// the left (as seen from the fire), turned round, pillows at the back.
			void SetTentBedroll(std::size_t a_slot, const Tent& a_tent, std::int32_t a_index, std::int32_t a_colorSeq)
			{
				const std::int32_t rowLength = a_tent.size == kLarge ? _props.Int("LargeTentCapacity", 4) : a_tent.beds;
				const float        back = a_tent.size == kLarge ? kLargeTentBedrollBack : 0.0f;
				const float        offset = RowPosition(rowLength - 1) / 2.0f - RowPosition(a_index);
				Set(a_slot, BedrollForm(a_colorSeq),
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
					_layout.clutterSet[kFirstTent + t] = 2 + _tents[t].size;  // 2 small, 3 large
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
					const bool  outside = _tents.empty() || _bedGroup[b] >= static_cast<std::int32_t>(_tents.size());
					const float turn = outside ? _ringSeatTurn[b] : _layout.turn[kFirstTent + _bedGroup[b]];
					Set(kFirstBedrollSeat + b, seat, _layout.fwd[kFirstBedroll + b], _layout.side[kFirstBedroll + b], turn + kBedrollSeatTurn);
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
				for (auto* marker : _props.Forms<RE::TESObjectREFR>("LZC_CampStorageMarkers")) {
					_layout.storageMarkers.push_back(marker);
				}
			}

			// The pieces that move to the camp rather than being placed.
			void Settle(std::vector<RE::TESObjectREFR*>& a_refs) const
			{
				auto* campfire = a_refs[kCampfire];

				// The cook box goes over the fire, enabled only while it's lit.
				auto* cookBox = _props.Bool("PlaceCampfire", true) ? _props.Form<RE::TESObjectREFR>("LZC_CookBox") : nullptr;
				if (cookBox && campfire) {
					cookBox->MoveTo(campfire);
					cookBox->Enable(false);
				}
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

				// The storage backpack moves onto the spot marked for it, which goes.
				auto* storage = _props.Form<RE::TESObjectREFR>("LZC_CampStorage");
				if (auto* spot = a_refs[kSlotCount]) {
					if (storage) {
						storage->MoveTo(spot);
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
		std::vector<RE::TESObjectREFR*> BuildCamp(RE::StaticFunctionTag*, RE::TESQuest* a_quest, std::int32_t a_partners, std::int32_t a_couples,
			std::vector<RE::Actor*> a_followers)
		{
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
			return Builder(props, player, a_followers, a_partners, a_couples).Run();
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
		std::vector<RE::TESObjectREFR*> GetFireSeats(RE::StaticFunctionTag*) { return built.fireSeats; }
		std::vector<std::int32_t>       GetFirePlaces(RE::StaticFunctionTag*) { return built.firePlaces; }
		std::vector<RE::TESObjectREFR*> GetCampBeds(RE::StaticFunctionTag*) { return built.beds; }
		std::vector<RE::TESObjectREFR*> GetBedSeats(RE::StaticFunctionTag*) { return built.bedSeats; }
		std::vector<std::int32_t>       GetBedGroups(RE::StaticFunctionTag*) { return built.bedGroups; }
		std::vector<std::int32_t>       GetBedOwners(RE::StaticFunctionTag*) { return built.bedOwners; }
		std::vector<std::int32_t>       GetBedOrders(RE::StaticFunctionTag*) { return built.bedOrders; }
		std::vector<RE::TESObjectREFR*> GetTubSeats(RE::StaticFunctionTag*) { return built.tubSeats; }
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
		a_vm->RegisterFunction("UnrollBeds"sv, kClass, UnrollBeds);
	}
}
