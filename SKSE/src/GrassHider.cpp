#include "GrassHider.h"

// Skyrim builds a cell's grass when the cell loads: for each grass type, a
// BSMultiStreamInstanceTriShape (shared by a 12x12 block of cells) holds
// groups of instances, and the cell's ExtraCellGrassData lists the groups it
// added. Each instance is 16 half floats; the first three are its position,
// x and y measured from the block's south-west corner. Grass loaded from No
// Grass In Objects' cache ends up in the same groups, so this works with or
// without it.
//
// Hiding: the camp gets a circle just big enough to hold every object, and
// every group in its cells with instances inside the circle is removed and
// added again without them. Grass that loads while the camp is up (walking
// back to it, loading a save) is caught by a check once a second.
// Showing: the removed instances are kept, and on pack-up they're added
// back to the same grass shapes they came from. Nothing is rebuilt, so it
// works however the grass was made (vanilla, a cache, or No Grass In Objects
// generating it). Grass in cells that unloaded meanwhile comes back whole
// when they load again.
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
		// How far outside its cell an instance may sit before the layout is
		// taken to be different from what this code expects.
		constexpr float kCellSlack = 1024.0f;

		constexpr std::uint32_t kCircleRecord = 'CIRC';
		constexpr std::uint32_t kCircleVersion = 1;

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

		// The cleared area. Saved as is, so keep it plain data.
		struct Circle
		{
			float x, y, radius;
			// Grass above or below this (a cliff top over the camp, say) stays.
			float minZ, maxZ;

			[[nodiscard]] bool Contains(float a_x, float a_y, float a_z) const
			{
				const float dx = a_x - x;
				const float dy = a_y - y;
				return a_z >= minZ && a_z <= maxZ && dx * dx + dy * dy <= radius * radius;
			}
		};
		static_assert(std::is_trivially_copyable_v<Circle>);

		struct State
		{
			RE::FormID            world{ 0 };
			std::optional<Circle> circle;
			// Instances removed so far, by cell and grass shape, to add back
			// on Show(). Holding the shape keeps its address from being reused.
			struct Removed
			{
				std::int32_t                                     cellX, cellY;
				RE::NiPointer<RE::BSMultiStreamInstanceTriShape> shape;
				std::vector<std::uint16_t>                       instances;
			};
			std::vector<Removed> removed;
			// Set while some of them wait for a busy shape after Show().
			RE::FormID restoreWorld{ 0 };
			// Groups already checked against the current circle.
			std::set<std::tuple<const InstanceGroup*, const std::uint16_t*, std::uint32_t>> clean;
			bool layoutBad{ false };
			bool logPending{ false };  // log what the next pass finds
		};

		// What a pass over the camp's cells found, for the log.
		struct Stats
		{
			std::uint32_t cells{ 0 };          // loaded cells under the circle
			std::uint32_t noGrassData{ 0 };    // of those, without ExtraCellGrassData
			std::uint32_t handles{ 0 };        // grass types in them
			std::uint32_t busyShapes{ 0 };     // skipped: the game was adding grass
			std::uint32_t otherSize{ 0 };      // skipped: not 16 half floats an instance
			std::uint32_t lastOtherSize{ 0 };
			std::uint32_t groups{ 0 };         // instance groups checked
			std::uint32_t instances{ 0 };
			std::uint32_t inCircle{ 0 };       // inside the circle across the ground
			std::uint32_t wrongHeight{ 0 };    // of those, outside its height band
			std::uint32_t removed{ 0 };
			std::uint32_t noData{ 0 };          // groups skipped: no CPU copy of their instances
			std::uint32_t noDataInstances{ 0 };
			std::uint32_t missingGroups{ 0 };   // indices with no group behind them
		};

		State& GetState()
		{
			static State state;
			return state;
		}

		// No Grass In Objects with both Extend-grass-distance and
		// Only-load-from-cache on loads every cell's grass onto stand-in cells
		// of its own, which other plugins can't reach: the real cells have no
		// grass handles, so there's nothing here to clear.
		bool ngioStandIns{ false };

		bool NgioUsesStandIns()
		{
			std::ifstream file("Data/SKSE/Plugins/GrassControl.ini");
			if (!file) {
				return false;
			}
			bool extend = false;
			bool cacheOnly = false;
			for (std::string line; std::getline(file, line);) {
				if (line.starts_with("\xEF\xBB\xBF")) {
					line.erase(0, 3);
				}
				if (const auto comment = line.find_first_of(";#"); comment != std::string::npos) {
					line.erase(comment);
				}
				const auto equals = line.find('=');
				if (equals == std::string::npos) {
					continue;
				}
				const auto trim = [](std::string a_text) {
					const auto first = a_text.find_first_not_of(" \t\r");
					const auto last = a_text.find_last_not_of(" \t\r");
					a_text = first == std::string::npos ? std::string() : a_text.substr(first, last - first + 1);
					std::ranges::transform(a_text, a_text.begin(), [](unsigned char a_c) { return static_cast<char>(std::tolower(a_c)); });
					return a_text;
				};
				const auto key = trim(line.substr(0, equals));
				const auto value = trim(line.substr(equals + 1));
				const bool on = value == "true" || value == "1";
				if (key == "extend-grass-distance") {
					extend = on;
				} else if (key == "only-load-from-cache") {
					cacheOnly = on;
				}
			}
			return extend && cacheOnly;
		}

		// Read by the polling thread; everything else is main-thread only.
		std::atomic<bool> busy{ false };

		void UpdateBusy()
		{
			const auto& state = GetState();
			busy = state.circle.has_value() || state.restoreWorld != 0;
		}

		std::vector<std::pair<std::int32_t, std::int32_t>> CellsUnder(const Circle& a_circle)
		{
			const auto x0 = static_cast<std::int32_t>(std::floor((a_circle.x - a_circle.radius) / kCellSize));
			const auto x1 = static_cast<std::int32_t>(std::floor((a_circle.x + a_circle.radius) / kCellSize));
			const auto y0 = static_cast<std::int32_t>(std::floor((a_circle.y - a_circle.radius) / kCellSize));
			const auto y1 = static_cast<std::int32_t>(std::floor((a_circle.y + a_circle.radius) / kCellSize));
			std::vector<std::pair<std::int32_t, std::int32_t>> cells;
			for (auto cx = x0; cx <= x1; ++cx) {
				for (auto cy = y0; cy <= y1; ++cy) {
					cells.emplace_back(cx, cy);
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

		// Removes the cell's grass instances inside the circle.
		// The south-west corner of the 12x12 block of cells a cell's grass is
		// measured from. Whether the blocks are aligned by rounding toward zero
		// or down only matters for negative coordinates, so both are tried and
		// the one the grass fits is used.
		float BlockOrigin(std::int32_t a_cell, bool a_floor)
		{
			std::int32_t block = a_cell / kCellsPerBlock;
			if (a_floor && a_cell < 0 && a_cell % kCellsPerBlock != 0) {
				--block;
			}
			return kCellSize * static_cast<float>(kCellsPerBlock * block);
		}

		void HideInCell(RE::TESObjectCELL* a_cell, std::int32_t a_cellX, std::int32_t a_cellY, const Circle& a_circle, Stats& a_stats)
		{
			auto& state = GetState();
			++a_stats.cells;
			auto* extra = a_cell->extraList.GetByType<RE::ExtraCellGrassData>();
			if (!extra) {
				++a_stats.noGrassData;
				return;
			}

			const float cellMinX = kCellSize * static_cast<float>(a_cellX) - kCellSlack;
			const float cellMinY = kCellSize * static_cast<float>(a_cellY) - kCellSlack;
			const float cellMaxX = cellMinX + kCellSize + 2.0f * kCellSlack;
			const float cellMaxY = cellMinY + kCellSize + 2.0f * kCellSlack;

			std::vector<std::uint16_t> kept;
			std::vector<std::uint16_t> gone;
			for (auto* entry : extra->grassHandles) {
				auto* handle = reinterpret_cast<GrassHandle*>(entry);
				auto* shape = handle ? handle->shape.get() : nullptr;
				if (!shape) {
					continue;
				}
				++a_stats.handles;
				// Leave a shape alone while the game is adding grass to it.
				if (shape->GetIsAddingInstances()) {
					++a_stats.busyShapes;
					continue;
				}
				auto& data = DataOf(shape);
				// Grass added a group at a time (from No Grass In Objects' cache,
				// say) leaves the shape's instance size at 0; each group's own
				// buffer size is checked below instead.
				if (data.instanceSize != kHalvesPerInstance && data.instanceSize != 0) {
					++a_stats.otherSize;
					a_stats.lastOtherSize = data.instanceSize;
					continue;
				}

				// Removing a group slides the next one into place, and groups
				// added back go on the end (and come up clean).
				for (std::uint32_t i = 0; i < handle->groupIndices.size();) {
					const auto index = handle->groupIndices[i];
					const auto* group = index < data.groups.size() ? data.groups[index] : nullptr;
					const auto* instances = group && group->vertexBuffer ? group->vertexBuffer->data : nullptr;
					const auto  key = std::make_tuple(group, instances, group ? group->instanceCount : 0);
					if (!group) {
						++a_stats.missingGroups;
					} else if (!instances && group->instanceCount > 0 && !state.clean.contains(key)) {
						++a_stats.noData;
						a_stats.noDataInstances += group->instanceCount;
					}
					if (!instances || group->instanceCount == 0 || state.clean.contains(key)) {
						++i;
						continue;
					}
					// The buffer must hold at least 16 half floats an instance,
					// whatever the shape says.
					const std::size_t needed = static_cast<std::size_t>(group->instanceCount) * kHalvesPerInstance * sizeof(std::uint16_t);
					if (data.instanceSize == 0 && group->vertexBuffer->byteWidth < needed) {
						++a_stats.otherSize;
						a_stats.lastOtherSize = static_cast<std::uint32_t>(group->vertexBuffer->byteWidth / group->instanceCount / sizeof(std::uint16_t));
						state.clean.insert(key);
						++i;
						continue;
					}

					++a_stats.groups;
					a_stats.instances += group->instanceCount;

					// Which block alignment the group's grass fits.
					const auto strayCount = [&](bool a_floor) {
						const float ox = BlockOrigin(a_cellX, a_floor);
						const float oy = BlockOrigin(a_cellY, a_floor);
						std::uint32_t count = 0;
						for (std::uint32_t n = 0; n < group->instanceCount; ++n) {
							const auto* instance = instances + static_cast<std::size_t>(n) * kHalvesPerInstance;
							const float x = ox + HalfToFloat(instance[0]);
							const float y = oy + HalfToFloat(instance[1]);
							if (x < cellMinX || x > cellMaxX || y < cellMinY || y > cellMaxY) {
								++count;
							}
						}
						return count;
					};
					const std::uint32_t truncStrays = strayCount(false);
					const bool          floorAligned = truncStrays > 0 && (a_cellX < 0 || a_cellY < 0) && strayCount(true) < truncStrays;
					const float         originX = BlockOrigin(a_cellX, floorAligned);
					const float         originY = BlockOrigin(a_cellY, floorAligned);

					kept.clear();
					gone.clear();
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
						const float dx = x - a_circle.x;
						const float dy = y - a_circle.y;
						if (dx * dx + dy * dy <= a_circle.radius * a_circle.radius) {
							++a_stats.inCircle;
							if (z < a_circle.minZ || z > a_circle.maxZ) {
								++a_stats.wrongHeight;
							}
						}
						if (a_circle.Contains(x, y, z)) {
							++removed;
							gone.insert(gone.end(), instance, instance + kHalvesPerInstance);
						} else {
							kept.insert(kept.end(), instance, instance + kHalvesPerInstance);
						}
					}
					a_stats.removed += removed;

					// Grass that isn't where it should be means the layout isn't
					// what this code expects (another game version, or a mod
					// that changes it). Stop rather than remove the wrong grass.
					if (strays * 4 > group->instanceCount) {
						logger::warn("Grass in cell ({}, {}) isn't laid out as expected ({} of {} instances outside the cell); grass hiding is off",
							a_cellX, a_cellY, strays, group->instanceCount);
						state.layoutBad = true;
						return;
					}
					if (removed == 0) {
						state.clean.insert(key);
						++i;
						continue;
					}

					auto stored = std::find_if(state.removed.begin(), state.removed.end(), [&](const auto& a_entry) {
						return a_entry.cellX == a_cellX && a_entry.cellY == a_cellY && a_entry.shape.get() == shape;
					});
					if (stored == state.removed.end()) {
						state.removed.push_back(State::Removed{ a_cellX, a_cellY, RE::NiPointer<RE::BSMultiStreamInstanceTriShape>(shape), {} });
						stored = std::prev(state.removed.end());
					}
					stored->instances.insert(stored->instances.end(), gone.begin(), gone.end());

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

		// The cell's grass handle for a_shape, if the cell still has it.
		GrassHandle* HandleFor(RE::TESObjectCELL* a_cell, const RE::BSMultiStreamInstanceTriShape* a_shape)
		{
			auto* extra = a_cell ? a_cell->extraList.GetByType<RE::ExtraCellGrassData>() : nullptr;
			if (!extra) {
				return nullptr;
			}
			for (auto* entry : extra->grassHandles) {
				auto* handle = reinterpret_cast<GrassHandle*>(entry);
				if (handle && handle->shape.get() == a_shape) {
					return handle;
				}
			}
			return nullptr;
		}

		// Drops stored instances whose grass is gone (the cell unloaded, or
		// the game rebuilt its grass): it comes back whole on its own.
		void PruneRemoved(RE::FormID a_world)
		{
			auto& removed = GetState().removed;
			std::erase_if(removed, [&](const auto& a_entry) {
				return !HandleFor(LoadedCell(a_world, a_entry.cellX, a_entry.cellY), a_entry.shape.get());
			});
		}

		// Adds the stored instances back where they came from.
		void Restore(RE::FormID a_world)
		{
			auto&         state = GetState();
			std::uint32_t restored = 0;
			std::uint32_t dropped = 0;
			std::uint32_t waiting = 0;
			std::vector<State::Removed> later;
			for (auto& entry : state.removed) {
				const auto count = static_cast<std::uint32_t>(entry.instances.size() / kHalvesPerInstance);
				auto*      shape = entry.shape.get();
				auto*      handle = HandleFor(LoadedCell(a_world, entry.cellX, entry.cellY), shape);
				if (!handle || count == 0) {
					dropped += count;
					continue;
				}
				// The game is adding grass to it: try again next second.
				if (shape->GetIsAddingInstances()) {
					waiting += count;
					later.push_back(std::move(entry));
					continue;
				}
				const auto&   data = DataOf(shape);
				std::uint32_t chunk = data.maxInstancesPerGroup > 0 ? data.maxInstancesPerGroup : count;
				for (std::uint32_t first = 0; first < count; first += chunk) {
					const auto n = std::min(chunk, count - first);
					shape->BeginAddingInstances(kHalvesPerInstance);
					shape->AddInstances(n, entry.instances[static_cast<std::size_t>(first) * kHalvesPerInstance]);
					shape->DoneAddingInstances(handle->groupIndices);
				}
				restored += count;
			}
			state.removed = std::move(later);
			state.restoreWorld = state.removed.empty() ? 0 : a_world;
			logger::info("Grass: {} instance(s) put back, {} already regrown or unloaded, {} waiting", restored, dropped, waiting);
		}

		// Runs on the main thread, once a second while there's anything to do.
		void Update()
		{
			auto& state = GetState();
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->Is3DLoaded()) {
				return;
			}

			if (state.restoreWorld != 0) {
				Restore(state.restoreWorld);
			}

			if (state.circle && !state.layoutBad) {
				PruneRemoved(state.world);
				Stats stats;
				for (const auto& [x, y] : CellsUnder(*state.circle)) {
					if (auto* cell = LoadedCell(state.world, x, y)) {
						HideInCell(cell, x, y, *state.circle, stats);
					}
				}
				if (state.logPending) {
					state.logPending = false;
					const auto& c = *state.circle;
					logger::info("Grass: circle at ({:.0f}, {:.0f}) radius {:.0f}, heights {:.0f} to {:.0f}", c.x, c.y, c.radius, c.minZ, c.maxZ);
					if (ngioStandIns && stats.handles == 0) {
						logger::warn("Grass: No Grass In Objects has Extend-grass-distance and Only-load-from-cache on, so its grass isn't in the game's cells and can't be cleared. "
									 "Set Extend-grass-distance = false in GrassControl.ini for grass clearing.");
					}
					logger::info("Grass: {} loaded cell(s), {} without grass data; {} grass type(s), {} busy, {} other format (size {}); "
								 "{} group(s), {} instance(s), {} in the circle, {} of those at the wrong height; {} removed{}",
						stats.cells, stats.noGrassData, stats.handles, stats.busyShapes, stats.otherSize, stats.lastOtherSize, stats.groups,
						stats.instances, stats.inCircle, stats.wrongHeight, stats.removed, state.layoutBad ? "; stopped: unexpected layout" : "");
					logger::info("Grass: skipped {} group(s) ({} instance(s)) with no CPU copy of their instances, {} missing group(s)",
						stats.noData, stats.noDataInstances, stats.missingGroups);
				}
			}
			UpdateBusy();
		}

	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		const auto& state = GetState();
		if (!state.circle || !a_intfc->OpenRecord(kCircleRecord, kCircleVersion)) {
			return;
		}
		a_intfc->WriteRecordData(state.world);
		a_intfc->WriteRecordData(*state.circle);
	}

	bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version)
	{
		if (a_type != kCircleRecord) {
			return false;
		}
		auto&      state = GetState();
		RE::FormID world = 0;
		Circle     circle{};
		if (a_version == kCircleVersion && !REL::Module::IsVR() &&
			a_intfc->ReadRecordData(world) == sizeof(world) && a_intfc->ReadRecordData(circle) == sizeof(circle) &&
			a_intfc->ResolveFormID(world, world)) {
			state.world = world;
			state.circle = circle;
			state.clean.clear();
			state.logPending = true;
			UpdateBusy();
		}
		return true;
	}

	void Revert()
	{
		auto& state = GetState();
		state.world = 0;
		state.circle.reset();
		state.removed.clear();
		state.restoreWorld = 0;
		state.clean.clear();
		UpdateBusy();
	}

	void Install()
	{
		if (REL::Module::IsVR()) {
			logger::info("Grass hiding isn't supported in Skyrim VR");
			return;
		}
		ngioStandIns = NgioUsesStandIns();
		if (ngioStandIns) {
			logger::warn("No Grass In Objects has Extend-grass-distance and Only-load-from-cache on: it keeps grass on cells of its own that "
						 "other plugins can't reach, so the camp can't clear grass. Set Extend-grass-distance = false in GrassControl.ini for grass clearing.");
		}

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

	void Hide(const std::vector<RE::TESObjectREFR*>& a_refs)
	{
		if (REL::Module::IsVR()) {
			return;
		}

		// Each object as a centre and the distance to its furthest corner.
		struct Extent
		{
			RE::NiPoint3 centre;
			float        reach, bottom, top;
		};
		std::vector<Extent> extents;
		RE::FormID          worldId = 0;
		for (auto* ref : a_refs) {
			auto* world = ref ? ref->GetWorldspace() : nullptr;
			auto* base = ref ? ref->GetBaseObject() : nullptr;
			// Interiors have no grass, and lights have no size.
			if (!world || !base || base->Is(RE::FormType::Light)) {
				continue;
			}
			const auto& bounds = base->boundData;
			const float scale = ref->GetScale();
			const float reachX = std::max(std::abs(bounds.boundMin.x), std::abs(bounds.boundMax.x)) * scale;
			const float reachY = std::max(std::abs(bounds.boundMin.y), std::abs(bounds.boundMax.y)) * scale;
			const auto  position = ref->GetPosition();
			extents.push_back({ position, std::hypot(reachX, reachY), position.z + bounds.boundMin.z * scale, position.z + bounds.boundMax.z * scale });
			worldId = world->GetFormID();
		}

		auto& state = GetState();
		state.circle.reset();
		state.clean.clear();
		state.removed.clear();
		state.restoreWorld = 0;
		state.world = worldId;
		if (!extents.empty()) {
			// Centred on the middle of the objects' spread; near enough the
			// smallest circle for a camp's fan.
			float minX = std::numeric_limits<float>::max();
			float minY = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::lowest();
			float maxY = std::numeric_limits<float>::lowest();
			float bottom = std::numeric_limits<float>::max();
			float top = std::numeric_limits<float>::lowest();
			for (const auto& extent : extents) {
				minX = std::min(minX, extent.centre.x - extent.reach);
				minY = std::min(minY, extent.centre.y - extent.reach);
				maxX = std::max(maxX, extent.centre.x + extent.reach);
				maxY = std::max(maxY, extent.centre.y + extent.reach);
				bottom = std::min(bottom, extent.bottom);
				top = std::max(top, extent.top);
			}
			Circle circle{ (minX + maxX) * 0.5f, (minY + maxY) * 0.5f, 0.0f, 0.0f, 0.0f };
			for (const auto& extent : extents) {
				circle.radius = std::max(circle.radius, std::hypot(extent.centre.x - circle.x, extent.centre.y - circle.y) + extent.reach);
			}
			// The ground may fall away towards the edge: allow for a slope
			// of about 27 degrees beyond the objects' own heights.
			circle.minZ = bottom - circle.radius * 0.5f;
			circle.maxZ = top + circle.radius * 0.5f;
			state.circle = circle;
			state.layoutBad = false;
			state.logPending = true;
		}
		Update();
	}

	void Show()
	{
		if (REL::Module::IsVR()) {
			return;
		}
		auto& state = GetState();
		const auto world = state.world;
		state.circle.reset();
		state.clean.clear();
		state.world = 0;
		if (!state.removed.empty()) {
			Restore(world);
		}
		UpdateBusy();
	}
}
