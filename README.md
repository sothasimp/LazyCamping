# Campsite Kit: a mod for Skyrim Special Edition

The mod adds a **Set Up Camp** Lesser Power. When the player has at least one **Bedroll** and a **Campfire Kit** in their inventory, casting the power uses up those kits and builds a camp in front of the player. Tent kits are optional. The camp includes:

- a burning campfire with flickering light and an optional cooking pot
- cross-legged seats in a ring around the fire, facing it:
  - one for the player, which the player can sit in. It's across the fire from where they stood to cast the power, in front of their tent's entrance.
  - one for each follower who's with the player, spaced evenly around the ring from the player's seat
- **tents in a ring around the fire**, outside the seats, with their entrances facing the fire:
  - every tent kit the player carries is pitched, duplicates included, up to 4 tents
  - each tent's size and colour come from its kit. There are small and large tents, each in brown, blue, green, red and white: 10 tent kits in all.
  - every tent gets at least one bedroll, so there are never more tents than bedrolls. With 3 tent kits and 2 bedrolls, for example, only 2 tents go up.
  - tents are taken in kit order: small brown, blue, green, red, white, then large in the same colour order
- **bedrolls in the tents**:
  - each tent gets one bedroll first, and each follower's first bed is in a different tent from the player's
  - spare bedrolls then fill the tents in turn, up to each tent's capacity: 2 for a small tent, 4 for a large one
- **the main tent**, the one with the most bedrolls (or the only tent), stands straight across the fire from the player, entrance facing them. The player's bedroll is in it.
- **the other tents** are spaced evenly around the ring: 2 tents face each other across the fire, 3 form a triangle, and 4 form a square. The player stands on the ring of seats, inside the ring of tents, so a tent on the near side stands behind them.
- **with no tent kits**, there are no tents. The bedrolls (up to 4) lie sideways, spaced evenly in a circle just outside the seats, starting behind the player's seat. With one bedroll per person, each lies behind its owner's seat.

Followers sit in their seats while the player sits in theirs, and lie down in the other bedrolls while the player is in their own bedroll. When the player gets up, the followers get up and follow them again.

**Sneaking and activating any of the tents from outside** packs up the camp straight away, with no prompt: everything is removed and every kit is returned, including each tent kit used. Activating a tent without sneaking, or from inside, does nothing. So the player can't pack up by accident while getting into bed.

In a camp without tents, **sneaking and activating any of the camp's bedrolls** packs the camp up. Activating a bedroll without sneaking opens the usual Sleep/Wait menu.

All EditorIDs use the prefix `CSK_`.

---

## 1. Files

```
Scripts/Source/
  CSK_CampsiteQuestScript.psc   main logic: checks, placement, tent menu, pack-up, state
  CSK_SetUpCampEffect.psc       on the magic effect; calls the quest
  CSK_TentActivatorScript.psc   on every tent; sneak-activating it from outside packs up the camp
  CSK_BedrollAliasScript.psc    on the RingBedroll aliases; sneak-activating a bedroll packs up a camp without tents
  CSK_CampFollowersScript.psc   followers' seats and bedrolls; sit/sleep with the player
  CSK_CampPlayerAliasScript.psc on the follower quest's Player alias; notices the player sitting
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

The script works out every position before placing anything, so a bedroll's ground check can't hit the tent standing over it.

Compiled `.pex` files go in `Data/Scripts/`.

---

## 2. Creation Kit setup

Open the Creation Kit with `Skyrim.esm` loaded. Before you compile anything, extract `Data/Scripts.zip` once. Copy the seven `.psc` files into `Data/Source/Scripts/` (the SE CK uses this folder).

### 2a. Inventory kits (Items > MiscItem)

| EditorID | Name | Suggested model | Weight / Value |
|---|---|---|---|
| `CSK_TentKitSmallBrown`, `…Blue`, `…Green`, `…Red`, `…White` | Small Brown Tent, Small Blue Tent, … | a rolled hide or a leather bundle clutter mesh | 8 / 40 |
| `CSK_TentKitLargeBrown`, `…Blue`, `…Green`, `…Red`, `…White` | Large Brown Tent, Large Blue Tent, … | the same, or a bigger bundle | 15 / 80 |
| `CSK_BedrollKit` | Bedroll | the rolled-up bedroll clutter mesh | 4 / 15 |
| `CSK_CampfireKit` | Campfire Kit | a firewood or kindling mesh | 3 / 10 |

That's 10 tent kits: two sizes in five colours. They can all share one inventory model, or use the coloured TextureSets from step 2b (on the model's *Alternate Textures*) so each kit looks its colour in the inventory.

To use custom meshes, put them in `Data/Meshes/CampsiteKit/` and point the Model field at them.

### 2b. Objects placed in the world

**Use vanilla objects directly where you can.** The campfire, light, cooking pot, bedroll and follower sit marker need nothing added to them, so fill their properties (step 2d) with the vanilla objects themselves rather than duplicates. That way, mods that change those vanilla objects also change the camp's. In particular, **Base Object Swapper (BOS) variance mods**, which swap vanilla objects for varied versions, also vary the camp's campfire and bedrolls. Find the objects by filtering the Object Window with the search term shown.

| Property | Type | Vanilla object (search term) | Notes |
|---|---|---|---|
| `CSK_CampfirePlaced` | Static | `campfire` (a *burning* one) | |
| `CSK_CampfireLight` | Light | `fire` (any flickering fire light) | |
| `CSK_CookingStation` | Furniture | `CookingPot` | Has the `CraftingCookingPot` keyword, so cooking works. Optional. |
| `CSK_BedrollPlaced` | Furniture | `bedroll` (the ground bedroll furniture) | Its sleep marker makes Sleep and Wait work. |
| `CSK_SitMarkerFollower` | Furniture | `CrossLegged` (a cross-legged sit marker) | Followers use these. Invisible is fine. |
| `CSK_SitMarkerPlayer` | Furniture | **duplicate** the same marker | The only one that has to be a new record: it needs a **visible model** so the player can activate it (see below). |

**The player's seat.** The player can only activate furniture they can point the crosshair at, and plain sit markers have no visible model or collision. So after duplicating the cross-legged marker:
1. Set its model to something to sit on, such as a fur pelt, a folded bedroll or a log.
2. Check that the furniture's sit marker still lines up with the model. Adjust it in the furniture's marker settings if needed.
3. Set the Activate Text Override to `Sit`.

The followers' seats can use the same visible model if you want all the seats to match.

**Tents: 2 sizes × 5 colours = 10 Activators.** Make each tent a new **Activator**:
- Model: a tent mesh. Search `tent` in Statics to find the vanilla `.nif` paths; pick a smaller and a larger one, or use your own meshes.
- Script: attach `CSK_TentActivatorScript` and set its `CampQuest` property to `CSK_CampsiteQuest`.
- Activate Text Override: `Camp`.

| | Brown | Blue | Green | Red | White |
|---|---|---|---|---|---|
| Small | `CSK_TentSmallBrown` | `CSK_TentSmallBlue` | `CSK_TentSmallGreen` | `CSK_TentSmallRed` | `CSK_TentSmallWhite` |
| Large | `CSK_TentLargeBrown` | `CSK_TentLargeBlue` | `CSK_TentLargeGreen` | `CSK_TentLargeRed` | `CSK_TentLargeWhite` |

You don't need a separate mesh for each colour:
1. Make recoloured copies of the tent's canvas or hide texture (`.dds`) in `Data/Textures/CampsiteKit/`.
2. Make a **TextureSet** record for each colour that points at its recoloured texture.
3. On each coloured Activator, open the model's *Alternate Textures* and swap the canvas texture for that colour's TextureSet.

Brown can stay as the mesh's original texture.

### 2c. The power

1. **Magic Effect** `CSK_SetUpCampEffect`
   - Archetype: *Script*, Casting Type: *Fire and Forget*, Delivery: *Self*
   - Check *No Duration*, *No Magnitude*, *No Area*, *No Hit Event*, *Painless*
   - Papyrus Scripts: add `CSK_SetUpCampEffect`, then set the `CampQuest` property to `CSK_CampsiteQuest`
2. **Spell** `CSK_SetUpCampPower`, named "Set Up Camp"
   - Type: *Lesser Power*, Casting: *Fire and Forget*, Delivery: *Self*
   - Effects: `CSK_SetUpCampEffect`, magnitude 0, duration 0
   - Set Equip Type to *Voice*, the slot powers use

### 2d. Controller quest

**Quest** `CSK_CampsiteQuest`
- Check *Start Game Enabled* and *Run Once*
- Scripts tab: add `CSK_CampsiteQuestScript`, then fill these properties:

| Property | Value |
|---|---|
| `PlayerRef` | `PlayerRef` (use auto-fill) |
| `CSK_SetUpCampPower` | the spell |
| `CSK_SmallTentKits`, `CSK_LargeTentKits` | the five tent kits of each size, **in the order brown, blue, green, red, white** |
| `CSK_SmallTents`, `CSK_LargeTents` | the five tents of each size, **in the same order** as the kits. Each kit pitches the tent at the same position in its list. |
| `CSK_BedrollKit`, `CSK_CampfireKit` | the other kits |
| `CSK_BedrollPlaced`, `CSK_CampfirePlaced` | the vanilla bedroll and burning campfire (step 2b) |
| `RingBedrollAliases` | the aliases `RingBedroll1`–`4`, in order (below) |
| `CSK_CampfireLight`, `CSK_CookingStation` | optional; leave empty to skip |
| `CSK_SitMarkerPlayer`, `CSK_SitMarkerFollower` | the seats; leave either empty to skip it |
| `LocTypeCity`, `LocTypeTown` | vanilla keywords (use auto-fill); blocks camping in settlements |
| `Firewood01`, `FirewoodRequired` | optional; for example `2` to burn 2 firewood per fire |
| `SmallTentCapacity`, `LargeTentCapacity` | bedrolls per tent (default 2 and 4, maximum 4) |
| `SmallTentHalfWidth`, `SmallTentHalfDepth`, `LargeTentHalfWidth`, `LargeTentHalfDepth` | half of each size's footprint; match these to your meshes. They place the tents in the ring, drive the flat-ground check, and decide what counts as inside a tent. |
| `TentRingGap` | all tents share one ring, `SeatRadius + TentRingGap + the largest HalfDepth among them` from the fire (gap default 60) |
| `BedrollSpacing` | side-to-side gap between bedrolls; check that 4 bedrolls fit in the large tent |
| `SeatRadius`, `LightHeight` | how far the seats are from the fire (default 110), and the light's height. The fire goes `SeatRadius` in front of the player, so they stand at the near edge of the seat ring. |
| `BedrollRingRadius`, `BedrollRingYaw` | without tents: how far the bedrolls are from the fire (default 230, just outside the seats), and the turn that lays them sideways (default 90; use 270 to swap head and foot) |
| `UseNativePlacement`, `AlignToSlope` | turn the SKSE features on or off (both default to on) |
| `MaxSlope`, `MaxTilt`, `MaxStep`, `CampfireHalfSize` | steepest ground allowed, most an object will tilt, how uneven the tent footprint may be, and the fire's footprint |
| `CSK_CampFollowersQuest` | the follower quest from step 2e; leave empty to skip follower seats and behaviour |
| `AllowPackUpFromPower`, `MaxPackUpDistance` | off by default, so only a sneak-activated tent packs up the camp, or a sneak-activated bedroll in a camp without tents. Turn it on to let casting the power pack up the camp too, for example to retrieve a camp left far away. |

**Quest Aliases** tab: create `RingBedroll1` to `RingBedroll4`, each with:
- Fill Type: *Specific Reference*, left empty
- Check *Optional*
- Attach `CSK_BedrollAliasScript`

In a camp without tents, the script puts the bedrolls in these aliases, so sneaking and activating one packs up the camp. Using aliases instead of a script on the bedroll means the bedroll can stay the vanilla object.

When the quest starts, it gives the player the power.

### 2e. Followers at camp (optional)

This quest runs while a camp is set up. When the camp goes up, its aliases find the followers who are with the player, and each one gets a seat by the fire and, if there's a spare bedroll, a bed. A global variable tells the followers what to do:

| `CSK_CampActivity` | When | Followers |
|---|---|---|
| 0 | otherwise | follow the player as normal |
| 1 | the player is sitting in their own seat by the fire | sit in their own seats |
| 2 | the player is in their own bedroll | sleep in their bedrolls |

**Global** `CSK_CampActivity`: Short, value 0.

**Quest** `CSK_CampFollowersQuest`
- Leave *Start Game Enabled* **unchecked**; the camp script starts and stops it. Leave *Run Once* unchecked too.
- **Quest Aliases** tab, create:
  - `Player`:
    - Fill Type: *Specific Reference* `PlayerRef`
    - Attach `CSK_CampPlayerAliasScript`
  - `Follower1`, `Follower2`, `Follower3`, each with:
    - Fill Type: *Find Matching Reference*, *In Loaded Area*, *Closest*
    - Check *Optional* and *Allow Reserved*
    - Conditions: `IsPlayerTeammate == 1` AND `GetDistance` (Reference: `PlayerRef`) `< 2048`
    - On `Follower2` and `Follower3`, add conditions that keep them from picking the same actor twice. One way is `GetIsAliasRef Follower1 == 0` (and `Follower2 == 0` on `Follower3`).
  - `Bedroll1`–`3` and `Seat1`–`3`, each with:
    - Fill Type: *Specific Reference*, left empty
    - Check *Optional*
    - The script fills these by calling `ForceRefTo`
- **Packages:** on each `FollowerN` alias, add two packages. Put the sleep package first.
  1. `CSK_FollowerSleepN`
     - Template: *Sleep*
     - Location: *Near Reference Alias* `BedrollN`, radius about 64. Keep the radius small so the follower picks that bedroll. If the template also has a bed or target field, point it at `BedrollN` too.
     - Condition: `GetGlobalValue CSK_CampActivity == 2`
  2. `CSK_FollowerSitN`
     - Template: *Sit*
     - Furniture or target: Reference Alias `SeatN`
     - Condition: `GetGlobalValue CSK_CampActivity == 1`

  For both packages, set the schedule to any time, with no duration limit.
- **Scripts** tab: add `CSK_CampFollowersScript`, and fill:
  - `PlayerRef`
  - `CSK_CampActivity`
  - `Followers`, `Bedrolls` and `Seats`: the three aliases of each kind, in order 1–3

Then set `CSK_CampFollowersQuest` on the main quest's script (step 2d).

A follower without a seat or bedroll keeps their normal behaviour, because that package's alias is empty. This also covers having more followers than spare bedrolls. Followers are counted when the camp goes up, so someone who joins afterwards gets a seat the next time you make camp.

### 2f. How players get the kits

Pick one or more of these:

- **Crafting**: add a Constructible Object (COBJ) for each kit with Workbench Keyword `CraftingTanningRack`. For example:
  - Small tent kit = 3× `Leather01` + 3× `LeatherStrips` + 2× `Firewood01`
  - Large tent kit = 6× `Leather01` + 5× `LeatherStrips` + 3× `Firewood01`
  - For colours, start from the brown recipe and add a dye ingredient, for example Blue Mountain Flower for blue, Red Mountain Flower for red, Hanging Moss for green and Bone Meal for white. Alternatively, make a colour recipe that converts a brown kit plus the dye into the coloured kit.
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

Every push to `main`, and every pull request, runs `.github/workflows/build.yml` on GitHub's Windows machines. To get the DLL:
1. Open the **Actions** tab on GitHub and select a run.
2. Download the **CampsiteKit** artifact from the run's page.

You can also start a build by hand with *Run workflow*.

xmake downloads the dependencies (such as spdlog) on its first run. The DLL ends up in `SKSE/dist/SKSE/Plugins/CampsiteKit.dll`.

The plugin uses Address Library, so the same DLL works on SE (1.5.97), AE (1.6.x) and VR. Players need **SKSE** and **Address Library for SKSE Plugins** installed. The plugin writes a log to `Documents/My Games/Skyrim Special Edition/SKSE/CampsiteKit.log`.

> CommonLibSSE-NG has maintained forks, such as alandtse's `CommonLibVR` (`ng` branch). If a future game update breaks the build, point the `SKSE/lib/commonlibsse-ng` submodule at one of those. The plugin only uses a handful of stable APIs.

## 4. Compile scripts and package

1. In the CK, compile all seven scripts (*Gameplay > Papyrus Script Manager*). `CSK_Native` compiles without the DLL; its functions are only bound at runtime.
2. Save the plugin as `CampsiteKit.esp`. It adds only new records, so you can flag it as ESL with xEdit.
3. Package these files:
   ```
   CampsiteKit.esp
   Scripts/CSK_CampsiteQuestScript.pex
   Scripts/CSK_SetUpCampEffect.pex
   Scripts/CSK_TentActivatorScript.pex
   Scripts/CSK_BedrollAliasScript.pex
   Scripts/CSK_Native.pex
   Scripts/CSK_CampFollowersScript.pex
   Scripts/CSK_CampPlayerAliasScript.pex
   Textures/CampsiteKit/...        (the tent recolours)
   SKSE/Plugins/CampsiteKit.dll
   Source/Scripts/*.psc            (optional but polite)
   Meshes/CampsiteKit/...          (if you made custom meshes)
   ```
   Mark SKSE as optional on your mod page. Without it, the mod still works, but the camp may float on slopes. If SKSE isn't installed, the Papyrus log records an "unbound native function" error on each cast. This error is harmless.

## 5. Known limitations and ideas

- **Uneven ground without SKSE**: in fallback mode, objects are placed at the player's height. Players should camp on flat ground or install SKSE.
- **Testing the tilt**: the slope math matches the game's `EulerAnglesToAxesZXY` rotation, and I checked it numerically. It still needs an in-game check on a hillside. If objects lean the wrong way, set `AlignToSlope` to false while you report or fix it; ground snapping and the site checks don't depend on it.
- **Footprint check**: the check samples 5 points (the centre and four corners). Something small that sits between those points can still poke through the tent. You can add more sample points in `IsAreaClear` if needed.
- **One camp at a time**: this is by design. Casting the power while a camp is up says so, unless `AllowPackUpFromPower` is on.
- **Bedroll ring**: neighbouring bedrolls in a tentless camp are at least 90° apart (4 bedrolls). If your bedroll model is long enough that they overlap end to end, raise `BedrollRingRadius`.
- **Sneak to pack up**: in a camp without tents, the bedrolls' normal activation is blocked and `CSK_BedrollAliasScript` passes it on itself. Followers should be unaffected, because anyone else's activation is passed straight through. If followers ever stop using the ring bedrolls, check this first.
- **Base Object Swapper**: BOS swaps a reference's base object when the reference loads. That should include references the camp places, but test it: with a campfire or bedroll variance mod installed, set up camp a few times and check that the campfire and bedrolls vary. The camp's scripts don't depend on the base object, so a swapped bedroll still sleeps, gets followers, and packs up.
- **Tent inside check**: "inside" means within the tent's `HalfWidth` × `HalfDepth` footprint. If activating the tent from just outside its entrance does nothing, shrink those values a little.
- **Follower count**: followers are counted when the camp goes up, up to 3. Someone who joins or leaves afterwards keeps their old status until the next camp.
- **Expansion ideas**:
  - a "Well Rested" style bonus for sleeping in your own tent (check `OnSleepStop` with a player alias)
  - fire damage when standing in the fire (a hazard)
  - chopping firewood as a requirement (already supported through `FirewoodRequired`)
  - a storage chest (place a container ref and empty it into the player on pack-up)
  - weather-dependent warmth for survival mode
