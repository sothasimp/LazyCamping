#include "ShrubHider.h"

namespace ShrubHider
{
	namespace
	{
		constexpr std::uint32_t kRecord = 'SHRB';
		constexpr std::uint32_t kVersion = 1;
		constexpr float         kMaxShrubHeight = 300.0f;  // taller trees are trees, and stay

		std::vector<RE::FormID> hidden;  // main thread only

		bool IsShrub(RE::TESObjectREFR* a_ref)
		{
			auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
			if (!base || a_ref->IsDisabled() || a_ref->IsPersistent() || a_ref->IsDynamicForm()) {
				return false;
			}
			if (base->Is(RE::FormType::Flora)) {
				return true;
			}
			if (base->Is(RE::FormType::Tree)) {
				const auto& bounds = base->boundData;
				return static_cast<float>(bounds.boundMax.z - bounds.boundMin.z) * a_ref->GetScale() < kMaxShrubHeight;
			}
			return false;
		}
	}

	void Hide(const std::vector<RE::TESObjectREFR*>& a_refs)
	{
		Show();
		// The circle the camp's objects cover, as for the grass: centred on
		// their spread, out to the furthest object's reach.
		std::vector<std::pair<RE::NiPoint3, float>> extents;
		RE::TESObjectREFR*                          origin = nullptr;
		for (auto* ref : a_refs) {
			auto* base = ref ? ref->GetBaseObject() : nullptr;
			if (!base || !ref->GetWorldspace() || base->Is(RE::FormType::Light)) {
				continue;
			}
			const auto& bounds = base->boundData;
			const float scale = ref->GetScale();
			const float reachX = std::max(std::abs(bounds.boundMin.x), std::abs(bounds.boundMax.x)) * scale;
			const float reachY = std::max(std::abs(bounds.boundMin.y), std::abs(bounds.boundMax.y)) * scale;
			extents.emplace_back(ref->GetPosition(), std::hypot(reachX, reachY));
			if (!origin) {
				origin = ref;
			}
		}
		if (extents.empty() || !origin) {
			return;
		}
		float minX = std::numeric_limits<float>::max(), minY = minX;
		float maxX = std::numeric_limits<float>::lowest(), maxY = maxX;
		for (const auto& [pos, reach] : extents) {
			minX = std::min(minX, pos.x - reach);
			minY = std::min(minY, pos.y - reach);
			maxX = std::max(maxX, pos.x + reach);
			maxY = std::max(maxY, pos.y + reach);
		}
		const RE::NiPoint3 centre{ (minX + maxX) * 0.5f, (minY + maxY) * 0.5f, origin->GetPositionZ() };
		float              radius = 0.0f;
		for (const auto& [pos, reach] : extents) {
			radius = std::max(radius, std::hypot(pos.x - centre.x, pos.y - centre.y) + reach);
		}

		auto* tes = RE::TES::GetSingleton();
		if (!tes) {
			return;
		}
		// Search from the origin far enough to cover the circle, then keep
		// what's inside it.
		const float searchRadius = radius + std::hypot(origin->GetPositionX() - centre.x, origin->GetPositionY() - centre.y);
		std::vector<RE::TESObjectREFR*> found;
		tes->ForEachReferenceInRange(origin, searchRadius, [&](RE::TESObjectREFR* a_ref) {
			if (a_ref && IsShrub(a_ref)) {
				const auto pos = a_ref->GetPosition();
				if (std::hypot(pos.x - centre.x, pos.y - centre.y) <= radius) {
					found.push_back(a_ref);
				}
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
		for (auto* ref : found) {
			ref->Disable();
			hidden.push_back(ref->GetFormID());
		}
		logger::info("Shrubs: {} hidden within {:.0f} of ({:.0f}, {:.0f})", hidden.size(), radius, centre.x, centre.y);
	}

	void Show()
	{
		for (const auto id : hidden) {
			if (auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id); ref && ref->IsDisabled()) {
				ref->Enable(false);
			}
		}
		if (!hidden.empty()) {
			logger::info("Shrubs: {} put back", hidden.size());
		}
		hidden.clear();
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (hidden.empty() || !a_intfc->OpenRecord(kRecord, kVersion)) {
			return;
		}
		a_intfc->WriteRecordData(static_cast<std::uint32_t>(hidden.size()));
		for (const auto id : hidden) {
			a_intfc->WriteRecordData(id);
		}
	}

	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version)
	{
		if (a_type != kRecord) {
			return false;
		}
		std::uint32_t count = 0;
		if (a_version != kVersion || a_intfc->ReadRecordData(count) != sizeof(count)) {
			return true;
		}
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID id = 0;
			if (a_intfc->ReadRecordData(id) != sizeof(id)) {
				break;
			}
			if (a_intfc->ResolveFormID(id, id)) {
				hidden.push_back(id);
			}
		}
		return true;
	}

	void Revert()
	{
		hidden.clear();
	}
}
