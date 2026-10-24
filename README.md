# Campsite Kit: a mod for Skyrim Special Edition

The mod adds a **Set Up Camp** Lesser Power. When the player has a **Tent Kit**, a **Bedroll** and a **Campfire Kit** in their inventory, casting the power uses up those kits and builds a camp in front of the player:

- a burning campfire with flickering light and an optional cooking pot
- a tent facing the fire
- a bedroll inside the tent that the player can sleep in

To pack up, the player either casts the power again or activates the tent. The kits go back into the inventory.

All EditorIDs use the prefix `CSK_`.

---

## 1. Files

```
SkyrimCampsite/
  Scripts/Source/
    CSK_CampsiteQuestScript.psc   main logic: checks, placement, pack-up, state
    CSK_SetUpCampEffect.psc       on the magic effect; calls the quest
    CSK_TentActivatorScript.psc   on the tent; activating it offers to pack up
    CSK_Native.psc                declarations of the SKSE plugin's functions
  SKSE/                           optional C++ plugin (CampsiteKit.dll)
    xmake.lua
    src/main.cpp                  plugin entry point, logging
    src/Papyrus.cpp               native functions for CSK_Native
    src/Placement.cpp             ground raycast, slope tilt, water check
```

### How the hybrid works

The mod works with or without SKSE. Each time the power is cast, the quest script calls `CSK_Native.GetVersion()`:

- **When `CampsiteKit.dll` is loaded**, the plugin:
  - checks the ground under the campfire and the tent footprint, and refuses to camp if the ground is too steep, uneven, blocked by rocks or logs, underwater, or missing
  - finds the real ground height under each object
  - tilts each object to match the slope, up to `MaxTilt` degrees
- **When the DLL is missing**, the call returns 0 and the script places everything at the player's height, as in the Papyrus-only version.

The script works out every position before placing anything, so the bedroll's ground check can't hit the tent standing over it.

Compiled `.pex` files go in `Data/Scripts/`.

---

## 2. Creation Kit setup

Open the Creation Kit with `Skyrim.esm` loaded. Before you compile anything, extract `Data/Scripts.zip` once. Copy the four `.psc` files into `Data/Source/Scripts/` (the SE CK uses this folder).

### 2a. Inventory kits (Items > MiscItem)

| EditorID | Name | Suggested model | Weight / Value |
|---|---|---|---|
| `CSK_TentKit` | Tent Kit | a rolled hide or a leather bundle clutter mesh | 10 / 50 |
| `CSK_BedrollKit` | Bedroll | the rolled-up bedroll clutter mesh | 4 / 15 |
| `CSK_CampfireKit` | Campfire Kit | a firewood or kindling mesh | 3 / 10 |

To use custom meshes, put them in `Data/Meshes/CampsiteKit/` and point the Model field at them.

### 2b. Objects placed in the world

Duplicate vanilla objects for these. Right-click a vanilla object, choose *Duplicate*, and give the copy a new ID. You can find candidates by filtering the Object Window with the search term shown.

| EditorID | Type | Duplicate from (search term) | Notes |
|---|---|---|---|
| `CSK_CampfirePlaced` | Static | `campfire` (a *burning* one) | |
| `CSK_CampfireLight` | Light | `fire` (any flickering fire light) | |
| `CSK_CookingStation` | Furniture | `CookingPot` | Keeps the `CraftingCookingPot` keyword, so cooking works. Optional. |
| `CSK_TentPlaced` | **Activator** | Make a new Activator and set its model to a vanilla tent mesh (search `tent` in Statics to find the `.nif` path) | Attach `CSK_TentActivatorScript`. Set the Activate Text Override to `Pack Up`. |
| `CSK_BedrollPlaced` | Furniture | `bedroll` (the ground bedroll furniture) | Duplicating it keeps the sleep marker, so Sleep and Wait work. |

### 2c. The power

1. **Magic Effect** `CSK_SetUpCampEffect`
   - Archetype: *Script*, Casting Type: *Fire and Forget*, Delivery: *Self*
   - Check *No Duration*, *No Magnitude*, *No Area*, *No Hit Event*, *Painless*
   - Papyrus Scripts: add `CSK_SetUpCampEffect`, then set the `CampQuest` property to `CSK_CampsiteQuest`
2. **Spell** `CSK_SetUpCampPower`, named "Set Up Camp"
   - Type: *Lesser Power*, Casting: *Fire and Forget*, Delivery: *Self*
   - Effects: `CSK_SetUpCampEffect`, magnitude 0, duration 0
   - Set Equip Type to *Voice*, the slot powers use

### 2d. Message box

**Message** `CSK_PackUpMsg`
- Check *Message Box*
- Text: "Pack up your camp?"
- Menu Buttons: `Pack up camp` (button 0), `Leave it` (button 1)

### 2e. Controller quest

**Quest** `CSK_CampsiteQuest`
- Check *Start Game Enabled* and *Run Once*
- Scripts tab: add `CSK_CampsiteQuestScript`, then fill these properties:

| Property | Value |
|---|---|
| `PlayerRef` | `PlayerRef` (use auto-fill) |
| `CSK_SetUpCampPower` | the spell |
| `CSK_TentKit`, `CSK_BedrollKit`, `CSK_CampfireKit` | the three MiscItems |
| `CSK_TentPlaced`, `CSK_BedrollPlaced`, `CSK_CampfirePlaced` | the placed objects |
| `CSK_CampfireLight`, `CSK_CookingStation` | optional; leave empty to skip |
| `CSK_PackUpMsg` | the message |
| `LocTypeCity`, `LocTypeTown` | vanilla keywords (use auto-fill); blocks camping in settlements |
| `Firewood01`, `FirewoodRequired` | optional; for example `2` to burn 2 firewood per fire |
| `MaxPackUpDistance` | `0` lets the player pack up from anywhere; for example `2048` requires being near the camp |
| `*Forward`, `*Right`, `LightHeight` | layout offsets; tune these to your meshes |
| `UseNativePlacement`, `AlignToSlope` | turn the SKSE features on or off (both default to on) |
| `MaxSlope`, `MaxTilt`, `MaxStep` | steepest ground allowed, most an object will tilt, and how uneven the tent footprint may be |
| `TentHalfWidth`, `TentHalfDepth`, `CampfireHalfSize` | footprints used by the flat-ground check; match these to your tent mesh |

When the quest starts, it gives the player the power.

The tent Activator from step 2b also needs its `CampQuest` property set to `CSK_CampsiteQuest`.

### 2f. How players get the kits

Pick one or more of these:

- **Crafting**: add a Constructible Object (COBJ) for each kit with Workbench Keyword `CraftingTanningRack`. For example:
  - Tent Kit = 4× `Leather01` + 4× `LeatherStrips` + 2× `Firewood01`
  - Bedroll = 2× `Leather01` + 2× `LeatherStrips`
  - Campfire Kit = 3× `Firewood01`
- **Merchants**: add the kits to a general goods leveled list, such as `LItemMiscVendorMiscItems75`.
- **Testing**: in the console, run `help CSK_ 4` to find the form IDs, then `player.additem <id> 1`.

---

## 3. Build the SKSE plugin

You need a Windows machine with:
- Visual Studio 2022, with the *Desktop development with C++* workload
- [xmake](https://xmake.io)
- git

Then run:

```powershell
git clone --recursive https://github.com/sothasimp/LazyCamping.git
cd LazyCamping\SKSE
xmake config -m release
xmake
```

(If you already cloned without `--recursive`, run `git submodule update --init` first. CommonLibSSE-NG is pinned as a submodule in `SKSE/lib/commonlibsse-ng`.)

The full Visual Studio IDE isn't required. The free **Build Tools for Visual Studio**, with only the *Desktop development with C++* workload selected, are enough.

### Building without a Windows PC

Every push runs `.github/workflows/build.yml` on GitHub's Windows machines. To get the DLL:
1. Open the **Actions** tab on GitHub and select a run.
2. Download the **CampsiteKit** artifact from the run's page.

You can also start a build by hand with *Run workflow*.

xmake downloads the dependencies (such as spdlog) on its first run. The DLL ends up in `SKSE/dist/SKSE/Plugins/CampsiteKit.dll`.

The plugin uses Address Library, so the same DLL works on SE (1.5.97), AE (1.6.x) and VR. Players need **SKSE** and **Address Library for SKSE Plugins** installed. The plugin writes a log to `Documents/My Games/Skyrim Special Edition/SKSE/CampsiteKit.log`.

> CommonLibSSE-NG has maintained forks, such as alandtse's `CommonLibVR` (`ng` branch). If a future game update breaks the build, point the `SKSE/lib/commonlibsse-ng` submodule at one of those. The plugin only uses a handful of stable APIs.

## 4. Compile scripts and package

1. In the CK, compile all four scripts (*Gameplay > Papyrus Script Manager*). `CSK_Native` compiles without the DLL; its functions are only bound at runtime.
2. Save the plugin as `CampsiteKit.esp`. It adds only new records, so you can flag it as ESL with xEdit.
3. Package these files:
   ```
   CampsiteKit.esp
   Scripts/CSK_CampsiteQuestScript.pex
   Scripts/CSK_SetUpCampEffect.pex
   Scripts/CSK_TentActivatorScript.pex
   Scripts/CSK_Native.pex
   SKSE/Plugins/CampsiteKit.dll
   Source/Scripts/*.psc            (optional but polite)
   Meshes/CampsiteKit/...          (if you made custom meshes)
   ```
   Mark SKSE as optional on your mod page. Without it, the mod still works, but the camp may float on slopes. If SKSE isn't installed, the Papyrus log records an "unbound native function" error on each cast. This error is harmless.

## 5. Known limitations and ideas

- **Uneven ground without SKSE**: in fallback mode, objects are placed at the player's height. Players should camp on flat ground or install SKSE.
- **Testing the tilt**: the slope math matches the game's `EulerAnglesToAxesZXY` rotation, and I checked it numerically. It still needs an in-game check on a hillside. If objects lean the wrong way, set `AlignToSlope` to false while you report or fix it; ground snapping and the site checks don't depend on it.
- **Footprint check**: the check samples 5 points (the centre and four corners). Something small that sits between those points can still poke through the tent. You can add more sample points in `IsAreaClear` if needed.
- **One camp at a time**: this is by design. Recasting the power offers to pack up the existing camp.
- **Expansion ideas**:
  - a "Well Rested" style bonus for sleeping in your own tent (check `OnSleepStop` with a player alias)
  - fire damage when standing in the fire (a hazard)
  - chopping firewood as a requirement (already supported through `FirewoodRequired`)
  - a storage chest (place a container ref and empty it into the player on pack-up)
  - weather-dependent warmth for survival mode
