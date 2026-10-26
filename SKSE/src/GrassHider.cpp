#include "GrassHider.h"

// Skyrim builds a cell's grass when the cell loads: for each grass type, a
// BSMultiStreamInstanceTriShape (shared by a 12x12 block of cells) holds
// groups of instances, and the cell's ExtraCellGrassData lists the groups it
// added. Each instance is 16 half floats; the first three are its position,
// x and y measured from the block's south-west corner. Grass loaded from No
// Grass In Objects' cache ends up in the same groups, so this works with or
// without it.
//
// Hiding: every group in a camp cell with instances inside a footprint is
// removed and added again without them. Grass that loads while the camp is up
// (walking back to it, loading a save) is caught by a check once a second.
// Showing: the camp's cells rebuild their grass from scratch.
//
// The layouts below aren't in CommonLibSSE-NG. They follow No Grass In
// Objects NG (GPL-3.0, like this plugin) and Community Shaders' grass shader.
namespace GrassHider
{
	namespace
	{
		constexpr float         kCellSize = 4096.0f;
		constexpr std::int32_t  kCellsPerBlock = 12;
		constexpr std::uint32_t kHalvesPerInstance = 16;
		// Grass blades lean and sway; this much height above and below an
		// object's bounds still counts as under it.
		constexpr float kHeightSlack = 64.0f;
		// How far outside its cell an instance may sit before the layout is
		// taken to be different from what this code expects.
		constexpr float kCellSlack = 1024.0f;

		constexpr std::uint32_t kRecordId = 'LZCG';
		constexpr std::uint32_t kZonesRecord = 'ZONE';
		constexpr std::uint32_t kZonesVersion = 1;

		struct VertexBuffer
		{
			void*          buffer;     // 00 - ID3D11Buffer
			std::uint16_t* data;       // 08 - CPU copy of the instances
			std::size_t    byteWidth;  // 10
		};
		static_assert(sizeof(VertexBuffer) == 0x18);

		struct InstanceGroup
		{
			std::byte     multiBound[0x40];  // 00 - BSMultiBoundAABB
			VertexBuffer* vertexBuffer;      // 40
			std::uint32_t triCount;          // 48
			std::uint32_t instanceCount;     // 4C
			bool          visible;           // 50
		};
		static_assert(sizeof(InstanceGroup) == 0x58);

		// BSMultiStreamInstanceTriShape from 0x160 (SE and AE).
		struct ShapeData
		{
			RE::BSTArray<InstanceGroup*> groups;                // 00
			std::uint32_t                meshTriCount;          // 18
			std::uint32_t                maxInstancesPerGroup;  // 1C
			float                        renderDistance;        // 20
			std::uint32_t                unk24;                 // 24
			void*                        groupAlloc;            // 28
			std::uint32_t                instanceCount;         // 30
			std::uint32_t                instanceSize;          // 34 - half floats per instance
			std::uint32_t                activeGroupCount;      // 38
		};
		static_assert(sizeof(ShapeData) == 0x40);

		// CellGrassData: one grass type's groups in one cell.
		struct GrassHandle
		{
			std::uint64_t                                    typeKey;      // 00
			RE::BSFixedString                                modelKey;     // 08
			RE::NiPointer<RE::BSMultiStreamInstanceTriShape> shape;        // 10
			RE::BSTArray<std::uint32_t>                      groupIndices; // 18
			RE::BSTArray<float>                              groupFades;   // 30
		};
		static_assert(sizeof(GrassHandle) == 0x48);

		ShapeData& DataOf(RE::BSMultiStreamInstanceTriShape* a_shape)
		{
			return REL::RelocateMember<ShapeData>(a_shape, 0x160);
		}

		void RemoveCellGrass(RE::BGSGrassManager* a_manager, RE::TESObjectCELL* a_cell)
		{
			using func_t = void (*)(RE::BGSGrassManager*, RE::TESObjectCELL*);
			static REL::Relocation<func_t> func{ RELOCATION_ID(15207, 15375) };
			func(a_manager, a_cell);
		}

		// Queues an AddCellGrassTask, as the game does when a cell comes into
		// grass range. Call with the cell's grassTaskLock held.
		void QueueCellGrass(RE::TESObjectCELL* a_cell)
		{
			using func_t = void (*)(RE::TESObjectCELL*);
			static REL::Relocation<func_t> func{ RELOCATION_ID(13137, 13277) };
			func(a_cell);
		}

		float HalfToFloat(std::uint16_t a_half)
		{
			const int   exponent = (a_half >> 10) & 0x1F;
			const int   mantissa = a_half & 0x3FF;
			const float sign = (a_half & 0x8000) ? -1.0f : 1.0f;
			if (exponent == 0) {
				return sign * std::ldexp(static_cast<float>(mantissa), -24);
			}
			if (exponent == 31) {
				return sign * std::numeric_limits<float>::infinity();
			}
			return sign * std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, exponent - 15);
		}

		// An object's bounds in its own frame, turned to its heading. Saved
		// as is, so keep it plain data.
		struct Footprint
		{
			float x, y;
			float sinYaw, cosYaw;
			float minX, maxX, minY, maxY;
			float minZ, maxZ;

			[[nodiscard]] bool Contains(float a_x, float a_y, float a_z) const
			{
				if (a_z < minZ || a_z > maxZ) {
					return false;
				}
				const float dx = a_x - x;
				const float dy = a_y - y;
				// Inverse of the heading turn: forward is (sin, cos), right is (cos, -sin).
				const float right = dx * cosYaw - dy * sinYaw;
				const float forward = dx * sinYaw + dy * cosYaw;
				return right >= minX && right <= maxX && forward >= minY && forward <= maxY;
			}

			[[nodiscard]] float Reach() const
			{
				return std::sqrt(std::max(minX * minX, maxX * maxX) + std::max(minY * minY, maxY * maxY));
			}
		};
		static_assert(std::is_trivially_copyable_v<Footprint>);

		struct State
		{
			RE::FormID             world{ 0 };
			std::vector<Footprint> zones;
			// A circle around all the zones, to skip most grass with one test.
			float centreX{ 0.0f };
			float centreY{ 0.0f };
			float radiusSquared{ 0.0f };
			// Cells whose grass is still to be rebuilt after Show().
			std::vector<std::pair<std::int32_t, std::int32_t>> rebuild;
			RE::FormID                                         rebuildWorld{ 0 };
			// Groups already checked against the current zones.
			std::set<std::tuple<const InstanceGroup*, const std::uint16_t*, std::uint32_t>> clean;
			bool layoutBad{ false };
		};

		State& GetState()
		{
			static State state;
			return state;
		}

		// Read by the polling thread; everything else is main-thread only.
		std::atomic<bool> busy{ false };

		void UpdateCircle()
		{
			auto& state = GetState();
			if (state.zones.empty()) {
				state.radiusSquared = 0.0f;
				return;
			}
			float minX = std::numeric_limits<float>::max();
			float minY = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::lowest();
			float maxY = std::numeric_limits<float>::lowest();
			for (const auto& zone : state.zones) {
				minX = std::min(minX, zone.x);
				minY = std::min(minY, zone.y);
				maxX = std::max(maxX, zone.x);
				maxY = std::max(maxY, zone.y);
			}
			state.centreX = (minX + maxX) * 0.5f;
			state.centreY = (minY + maxY) * 0.5f;
			float radius = 0.0f;
			for (const auto& zone : state.zones) {
				radius = std::max(radius, std::hypot(zone.x - state.centreX, zone.y - state.centreY) + zone.Reach());
			}
			state.radiusSquared = radius * radius;
		}

		void UpdateBusy()
		{
			const auto& state = GetState();
			busy = !state.zones.empty() || !state.rebuild.empty();
		}

		std::set<std::pair<std::int32_t, std::int32_t>> CellsUnder(const std::vector<Footprint>& a_zones)
		{
			std::set<std::pair<std::int32_t, std::int32_t>> cells;
			for (const auto& zone : a_zones) {
				const float reach = zone.Reach();
				const auto  x0 = static_cast<std::int32_t>(std::floor((zone.x - reach) / kCellSize));
				const auto  x1 = static_cast<std::int32_t>(std::floor((zone.x + reach) / kCellSize));
				const auto  y0 = static_cast<std::int32_t>(std::floor((zone.y - reach) / kCellSize));
				const auto  y1 = static_cast<std::int32_t>(std::floor((zone.y + reach) / kCellSize));
				for (auto cx = x0; cx <= x1; ++cx) {
					for (auto cy = y0; cy <= y1; ++cy) {
						cells.emplace(cx, cy);
					}
				}
			}
			return cells;
		}

		// The loaded exterior cell at (a_x, a_y) in a_world, if any.
		RE::TESObjectCELL* LoadedCell(RE::FormID a_world, std::int32_t a_x, std::int32_t a_y)
		{
			auto* tes = RE::TES::GetSingleton();
			auto* grid = tes ? tes->gridCells : nullptr;
			if (!grid || !grid->cells) {
				return nullptr;
			}
			for (std::uint32_t i = 0; i < grid->length * grid->length; ++i) {
				auto* cell = grid->cells[i];
				if (!cell || !cell->IsAttached() || !cell->IsExteriorCell()) {
					continue;
				}
				auto* world = cell->GetRuntimeData().worldSpace;
				const auto* coords = cell->GetCoordinates();
				if (world && world->GetFormID() == a_world && coords && coords->cellX == a_x && coords->cellY == a_y) {
					return cell;
				}
			}
			return nullptr;
		}

		// Removes the cell's grass instances inside any of the zones.
		void HideInCell(RE::TESObjectCELL* a_cell, std::int32_t a_cellX, std::int32_t a_cellY)
		{
			auto& state = GetState();
			auto* extra = a_cell->extraList.GetByType<RE::ExtraCellGrassData>();
			if (!extra) {
				return;
			}

			const float originX = kCellSize * static_cast<float>(kCellsPerBlock * (a_cellX / kCellsPerBlock));
			const float originY = kCellSize * static_cast<float>(kCellsPerBlock * (a_cellY / kCellsPerBlock));
			const float cellMinX = kCellSize * static_cast<float>(a_cellX) - kCellSlack;
			const float cellMinY = kCellSize * static_cast<float>(a_cellY) - kCellSlack;
			const float cellMaxX = cellMinX + kCellSize + 2.0f * kCellSlack;
			const float cellMaxY = cellMinY + kCellSize + 2.0f * kCellSlack;

			std::vector<std::uint16_t> kept;
			for (auto* entry : extra->grassHandles) {
				auto* handle = reinterpret_cast<GrassHandle*>(entry);
				auto* shape = handle ? handle->shape.get() : nullptr;
				// Leave a shape alone while the game is adding grass to it.
				if (!shape || shape->GetIsAddingInstances()) {
					continue;
				}
				auto& data = DataOf(shape);
				if (data.instanceSize != kHalvesPerInstance) {
					continue;
				}

				// Removing a group slides the next one into place, and groups
				// added back go on the end (and come up clean).
				for (std::uint32_t i = 0; i < handle->groupIndices.size();) {
					const auto index = handle->groupIndices[i];
					const auto* group = index < data.groups.size() ? data.groups[index] : nullptr;
					const auto* instances = group && group->vertexBuffer ? group->vertexBuffer->data : nullptr;
					const auto  key = std::make_tuple(group, instances, group ? group->instanceCount : 0);
					if (!instances || group->instanceCount == 0 || state.clean.contains(key)) {
						++i;
						continue;
					}

					kept.clear();
					std::uint32_t removed = 0;
					std::uint32_t strays = 0;
					for (std::uint32_t n = 0; n < group->instanceCount; ++n) {
						const auto* instance = instances + static_cast<std::size_t>(n) * kHalvesPerInstance;
						const float x = originX + HalfToFloat(instance[0]);
						const float y = originY + HalfToFloat(instance[1]);
						const float z = HalfToFloat(instance[2]);
						if (x < cellMinX || x > cellMaxX || y < cellMinY || y > cellMaxY) {
							++strays;
						}
						const float dx = x - state.centreX;
						const float dy = y - state.centreY;
						if (dx * dx + dy * dy <= state.radiusSquared &&
							std::ranges::any_of(state.zones, [&](const Footprint& a_zone) { return a_zone.Contains(x, y, z); })) {
							++removed;
						} else {
							kept.insert(kept.end(), instance, instance + kHalvesPerInstance);
						}
					}

					// Grass that isn't where it should be means the layout isn't
					// what this code expects (another game version, or a mod
					// that changes it). Stop rather than remove the wrong grass.
					if (strays * 4 > group->instanceCount) {
						logger::warn("Grass in cell ({}, {}) isn't laid out as expected; grass hiding is off", a_cellX, a_cellY);
						state.layoutBad = true;
						return;
					}
					if (removed == 0) {
						state.clean.insert(key);
						++i;
						continue;
					}

					shape->RemoveGroup(index);
					handle->groupIndices.erase(handle->groupIndices.begin() + i);
					if (i < handle->groupFades.size()) {
						handle->groupFades.erase(handle->groupFades.begin() + i);
					}
					if (!kept.empty()) {
						shape->BeginAddingInstances(kHalvesPerInstance);
						shape->AddInstances(static_cast<std::uint32_t>(kept.size() / kHalvesPerInstance), kept[0]);
						shape->DoneAddingInstances(handle->groupIndices);
					}
				}
			}
		}

		void Rebuild(RE::TESObjectCELL* a_cell)
		{
			auto* manager = RE::BGSGrassManager::GetSingleton();
			if (!manager) {
				return;
			}
			RemoveCellGrass(manager, a_cell);
			RE::BSSpinLockGuard lock(a_cell->grassTaskLock);
			QueueCellGrass(a_cell);
		}

		// Runs on the main thread, once a second while there's anything to do.
		void Update()
		{
			auto& state = GetState();
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->Is3DLoaded()) {
				return;
			}

			// Cells that aren't loaded will build their grass whole when they load.
			for (const auto& [x, y] : state.rebuild) {
				if (auto* cell = LoadedCell(state.rebuildWorld, x, y)) {
					Rebuild(cell);
				}
			}
			state.rebuild.clear();

			if (!state.zones.empty() && !state.layoutBad) {
				for (const auto& [x, y] : CellsUnder(state.zones)) {
					if (auto* cell = LoadedCell(state.world, x, y)) {
						HideInCell(cell, x, y);
					}
				}
			}
			UpdateBusy();
		}

		std::optional<Footprint> FootprintOf(RE::TESObjectREFR* a_ref, float a_margin)
		{
			auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
			if (!base || base->Is(RE::FormType::Light)) {
				return std::nullopt;
			}
			const auto& bounds = base->boundData;
			if (bounds.boundMin.x == bounds.boundMax.x || bounds.boundMin.y == bounds.boundMax.y) {
				return std::nullopt;
			}

			const float scale = a_ref->GetScale();
			const auto  position = a_ref->GetPosition();
			const float yaw = a_ref->GetAngleZ();
			return Footprint{
				position.x, position.y,
				std::sin(yaw), std::cos(yaw),
				bounds.boundMin.x * scale - a_margin, bounds.boundMax.x * scale + a_margin,
				bounds.boundMin.y * scale - a_margin, bounds.boundMax.y * scale + a_margin,
				position.z + bounds.boundMin.z * scale - kHeightSlack, position.z + bounds.boundMax.z * scale + kHeightSlack
			};
		}

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			const auto& state = GetState();
			if (!a_intfc->OpenRecord(kZonesRecord, kZonesVersion)) {
				return;
			}
			const auto count = static_cast<std::uint32_t>(state.zones.size());
			a_intfc->WriteRecordData(state.world);
			a_intfc->WriteRecordData(count);
			if (count > 0) {
				a_intfc->WriteRecordData(state.zones.data(), static_cast<std::uint32_t>(count * sizeof(Footprint)));
			}
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			auto& state = GetState();
			state.world = 0;
			state.zones.clear();
			state.rebuild.clear();
			state.clean.clear();
			UpdateBusy();
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			auto&         state = GetState();
			std::uint32_t type = 0;
			std::uint32_t version = 0;
			std::uint32_t length = 0;
			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (type != kZonesRecord || version != kZonesVersion) {
					continue;
				}
				RE::FormID    world = 0;
				std::uint32_t count = 0;
				if (!a_intfc->ReadRecordData(world) || !a_intfc->ReadRecordData(count) || !a_intfc->ResolveFormID(world, world)) {
					continue;
				}
				std::vector<Footprint> zones(count);
				const auto             bytes = static_cast<std::uint32_t>(count * sizeof(Footprint));
				if (count > 0 && a_intfc->ReadRecordData(zones.data(), bytes) != bytes) {
					continue;
				}
				state.world = world;
				state.zones = std::move(zones);
				state.clean.clear();
				UpdateCircle();
			}
			UpdateBusy();
		}
	}

	void Install()
	{
		if (REL::Module::IsVR()) {
			logger::info("Grass hiding isn't supported in Skyrim VR");
			return;
		}

		auto* serialization = SKSE::GetSerializationInterface();
		serialization->SetUniqueID(kRecordId);
		serialization->SetSaveCallback(OnSave);
		serialization->SetRevertCallback(OnRevert);
		serialization->SetLoadCallback(OnLoad);

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

	void Hide(const std::vector<RE::TESObjectREFR*>& a_refs, float a_margin)
	{
		if (REL::Module::IsVR()) {
			return;
		}
		auto& state = GetState();
		state.zones.clear();
		state.clean.clear();
		state.world = 0;
		for (auto* ref : a_refs) {
			auto* world = ref ? ref->GetWorldspace() : nullptr;
			if (!world) {
				continue;  // Interiors have no grass.
			}
			if (auto footprint = FootprintOf(ref, a_margin)) {
				state.world = world->GetFormID();
				state.zones.push_back(*footprint);
			}
		}
		UpdateCircle();
		Update();
	}

	void Show()
	{
		if (REL::Module::IsVR()) {
			return;
		}
		auto& state = GetState();
		if (!state.zones.empty()) {
			const auto cells = CellsUnder(state.zones);
			state.rebuild.assign(cells.begin(), cells.end());
			state.rebuildWorld = state.world;
		}
		state.zones.clear();
		state.clean.clear();
		state.world = 0;
		Update();
	}
}
