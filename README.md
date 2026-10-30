# Lazy Camping: a mod for Skyrim Special Edition

The mod adds a **Set Up Camp** Lesser Power, named " Set Up Camp" with a leading space so it sorts to the top of the magic menu. When the player has at least one **Bedroll** in their inventory, casting the power builds a camp in front of the player, always with a campfire. Tent kits are optional. The kits are **reusable**: setting up camp doesn't take them from the inventory. The mod requires **SKSE**, **Address Library for SKSE Plugins**, and its own `LazyCamping.dll`. The camp includes:

- a burning campfire with flickering light. **Activating it opens the Cook menu** (no animation, as there's no pot to stir).
  - with the optional **firewood mechanic** (`FirewoodMechanic`), the fire starts out as an unlit firepit. Activating it with two `Firewood01` in the inventory lights it, burning both; it burns for 6 game hours (`FireBurnHours`), then goes out and can be lit again. A woodcutting block stands beside the outermost tent (or along the bedroll fan without tents) for chopping more.
- cross-legged seats around the fire, facing it:
  - one for the player, which the player can sit in. It's across the fire from where they stood to cast the power, in front of their tent's entrance.
  - one for followers per bedroll beyond the player's, up to 5, fanning out from the player's seat every 60°: 60° right, 60° left, 120° right, 120° left, and the fifth on the near side, where the player stood. The two at 60° use the player's seat, so the player can sit in them too, and they're there even when camping alone.
- **tents in an amphitheater** on the far side of the fire, outside the seats, with their entrances facing the fire and the open side where the player stands:
  - up to 3 tents, with **at most one large tent**: the first large tent kit the player carries, in colour order. Every small tent kit carried, duplicates included, fills the rest.
  - each tent's size and colour come from its kit. There are small and large tents, each in brown, blue, green, red and white: 10 tent kits in all.
  - every tent gets at least one bedroll, so there are never more tents than bedrolls. With 3 tent kits and 2 bedrolls, for example, only 2 tents go up.
  - small tents are taken in colour order: brown, blue, green, red, white
- **bedrolls in the tents**:
  - the player's tent gets a bedroll for the player and one for each partner (spouse or lover) travelling with them
  - each follower couple gets a tent with two bedrolls
  - every other tent gets one bedroll
  - if there aren't enough bedrolls for that, fewer tents go up rather than splitting anyone up. With 2 tent kits, 2 bedrolls and a spouse, for example, one tent goes up with both bedrolls in it.
  - spare bedrolls then fill the tents in turn, up to each tent's capacity: 2 for a small tent, 4 for a large one
  - in a tent, the bedrolls lie in pairs that touch, with a small gap between pairs (`BedrollSpacing` minus `PairedBedrollSpacing`), so a couple (or the player and a partner) shares a touching pair. They run left to right as seen from the fire, the player's first. A large tent's row is laid out for a full tent, 1+2, gap, 3+4, and fills from the far left; it sits 60 further from the fire than the tent's centre (`LargeTentBedrollBack`), and can be shifted sideways (`LargeTentBedrollLeft`). A small tent's row is centred.
- **the player's tent** stands in the middle, straight across the fire from the player, entrance facing them, with the player's bedroll in it. It's the large tent if there is one; otherwise it's the small tent with the most bedrolls.
- **the other tents** go to either side of it: the second on the right, the third on the left. With only small tents, they stand 70° round the fire to either side, all 250 from its centre, facing it. Beside the large tent (107 from the fire), the small tents come round toward the player's side and angle in, so the three entrances face each other around the fire. All of these spots are constants in `SKSE/src/CampLayout.cpp`.
- **with no tent kits**, there are no tents. The bedrolls (up to 4) lie sideways in a close amphitheater arc just outside the seats, 50° apart round the fire: the player's straight across the fire, behind their seat, the rest to either side. The player and a partner, and a follower couple, get neighbouring places with their bedrolls head to head.

A **bathtub** (`LZC_Tub`, a static with your tub mesh) stands beside the outermost tent, on the other side from the chopping block, or, without tents, just left of the player's bedroll as the player sees it across the fire. It can be turned off (`PlaceTub`). With `TubInLargeTent` on, it goes inside the large tent instead, in place of the third and fourth bedrolls, 137 right of the tent's centre (`TubInTentRight`) and turned like the bedrolls; the large tent then holds 2 bedrolls at most, and the chopping block stays outside. What's in and around it is the **tub clutter set**, arranged in the Creation Kit: by default the vanilla rain barrel water (`LargeRainBarrelWater01`) in it and a **clothes basket** (`LZC_ClothesBasket`): activating it takes off everything the player is wearing, and activating it again puts it all back on; add steps, invisible seats inside the tub and so on. The **water** should be the clutter set's own reference, filled into `LZC_TubWater`: instead of being copied it moves onto the tub (keeping its offset, turn and scale), because the game only treats placeable water as real water, which bathing mods such as *Bathing in Skyrim* look for, when the reference was placed in the Creation Kit. An optional **tub trigger** (`LZC_TubTrigger`) moves onto the tub: followers who step into it take off their clothes, and put them back on when they step out or the camp is packed up. Its water is real water to the game, so *Bathing in Skyrim* lets you bathe in it.

**Clutter sets** add clutter around the camp, such as pelts, packs, firewood and lanterns. You arrange each set by eye in the Creation Kit around a stand-in marker, and every fire, tent or tentless bedroll gets a copy, turned to match. One **backpack** can be the player's storage: it's a single real container that moves from camp to camp, so whatever's in it stays there.

While a camp is up, **the grass is cleared** in a circle around it, just big enough to hold the tents, bedrolls, fire, seats, clutter and storage, so nothing pokes through them. It comes back when the camp is packed up. This works with or without *No Grass In Objects* and its grass cache.

While a camp is up, a **map marker** shows where it is, and the player can fast travel to it, unless fast travel is disabled globally (for example in Survival Mode). Packing up removes the marker.

Every bedroll also has a **seat on it**, facing the fire. Followers sit at the fire when the player sits in any of the three fire seats they can use, and on the other bedrolls when the player sits on a bedroll, matched the same way as at bedtime. Once seated, they stay seated after the player gets up, until the player goes to bed, packs up the camp, or leaves it. When the player sits in one of the tub's seats (furniture in the tub clutter set), one follower, a spouse or lover first, sits in another. When the player sleeps at camp, in any of the camp's bedrolls, followers sleep in the bedrolls the player isn't using. A spouse or lover gets the bedroll next to the player's: the one beside them in the same tent, or the neighbouring one in a camp without tents. Two followers who are a couple get two free bedrolls side by side, normally in their own tent.

**Sneaking and activating any of the tents from outside** asks *Pack up your camp?*; on *Yes*, everything the camp placed is removed. Activating a tent without sneaking, or from inside, does nothing. So the player can't pack up by accident while getting into bed.

**Sneaking and activating any of the camp's bedrolls** sits the player cross-legged on it (on its seat), with or without tents. Activating a bedroll without sneaking opens the usual Sleep/Wait menu.

While a camp is up, the power is called **Pack Up Camp** (" Pack Up Camp"). It keeps its place in favourites and the voice slot. **Casting it** asks *Pack up your camp?* and packs it up if confirmed, from anywhere, so a camp left behind never stops the player making a new one.

With **SKSE Menu Framework** installed (optional), its Mod Control Panel gets a *Lazy Camping* page for the firewood mechanic, how long a fire burns, the chopping block, the bathtub and whether it goes in the large tent, aligning the camp to the slope, the fire seats' colour, the bedrolls' style (with *Bedroll Alternative - BOS Color Variance*: its green, blue or red bedroll, or all three in turn, each touching pair in a tent matching; greyed out without it; the bedroll kit item takes the chosen colour too, green for mixed), selling the kits at general goods merchants, clearing grass around the camp, and the campfire light marker (turn it off if a *Light Placer* config already lights the campfire). The settings are kept in the save. Without it, change them on the quest in the Creation Kit.

All EditorIDs use the prefix `LZC_`.

---

## 1. Files

```
Scripts/Source/
  LZC_CampsiteQuestScript.psc   main logic: checks, camp objects, pack-up, state
  LZC_SetUpCampEffect.psc       on the magic effect; calls the quest
  LZC_CampObjectScript.psc      on the tents, unlit firepit, cook box, clothes basket and tub trigger; passes activation and trigger events to the quest
  LZC_CampFollowersScript.psc   followers' seats and bedrolls; sit/sleep with the player
  LZC_CampPlayerAliasScript.psc on the follower quest's Player alias; notices the player sitting
  LZC_Native.psc                declarations of the SKSE plugin's functions
SKSE/                           C++ plugin (LazyCamping.dll), required
  xmake.lua
  src/main.cpp                  plugin entry point, logging
  src/Papyrus.cpp               native functions for LZC_Native; ground checks and placing
  src/CampLayout.cpp            setting up a camp: the layout and everything placed with it
  src/BedrollKit.cpp            the bedroll kit's colour
  src/VendorKits.cpp            the kits at general goods merchants
  src/Placement.cpp             ground raycast, slope tilt, water check
  src/GrassHider.cpp            hides the grass under the camp
  src/BedrollSeats.cpp          sneak-activating a camp bedroll sits the player on it
  src/FormNames.cpp             renames the power while a camp is up, kept in the save
  src/Serialization.cpp         the SKSE co-save
  src/Menu.cpp                  the optional SKSE Menu Framework page
  src/ScriptObjects.cpp         reading and writing other scripts' properties
  extern/SKSEMenuFramework/     SKSE Menu Framework 3's API header (LGPL-2.1)
LazyCamping.esp                 the plugin, edited in the Creation Kit
```

### How the scripts and the plugin split the work

Setting up a camp is one call into `LazyCamping.dll`, **`BuildCamp`**, so it happens in a single frame instead of being spread over many by Papyrus's small time budget. From `LZC_CampsiteQuestScript`'s properties it:

1. works out which tents and bedrolls go up and where everything stands relative to the player (`SKSE/src/CampLayout.cpp`, where the layout distances and angles live);
2. checks the ground under the fire and every tent's footprint, and refuses the camp if the ground is too steep, uneven, blocked by rocks or logs, underwater, or missing;
3. finds the ground under every object, tilts each one to the slope (up to `MaxSlope` degrees), and creates them all with their clutter at their final positions. It finds the ground for everything before placing anything, so a bedroll's ground check can't hit the tent standing over it;
4. moves the cook box, storage backpack and map marker to the camp, sets up the bedroll seats, and clears the grass in a circle around the camp (see *Hiding grass* below).

The script keeps the rest: the events (casting the power, activating camp objects, sitting, sleeping), the follower quest, which it hands the camp's seats and bedrolls, and the tub's trigger and water. Packing up calls **`ShowGrass`**.

If the DLL isn't loaded, or is older than the scripts expect, casting the power tells the player that Lazy Camping needs SKSE, and nothing is placed.

Compiled `.pex` files go in `Data/Scripts/`.

---

## 2. The plugin (`LazyCamping.esp`)

`LazyCamping.esp` is edited in the Creation Kit and committed to the repository like any other file.

**Before editing it in the Creation Kit,** take the latest `LazyCamping.esp` from the repository: some records (the recipes, the merchant list) are sometimes added to it directly, and saving over an older copy loses them. After saving, check that the quest's object properties are still filled.

### What's in it

| Records | EditorIDs | Notes |
|---|---|---|
| Kits | `LZC_BedrollKit`, `LZC_TentKitSmallBrown` … `LZC_TentKitLargePlaidBlue` | MiscItems: the bedroll and a small and large tent in each colour (brown, blue, green, red, white, plaid green, plaid yellow, plaid blue) |
| Recipes | `LZC_RecipeBedrollKit`, `LZC_RecipeTentKit…` | one per kit, at a tanning rack (see 2d) |
| Merchants | `LZC_LItemCampingKits`, `LZC_VendorLists` | a leveled list of the kits, and the merchant lists it's added to (see 2d) |
| Tents | `LZC_TentSmallBrown` … `LZC_TentLargePlaidBlue` | Activators with `LZC_CampObjectScript`; colours are texture sets swapped onto the canvas |
| Bath | `LZC_Tub`, `LZC_ClothesBasket` | the tub (static, your mesh); a clothes basket activator (*Use*: undress, then dress again; `LZC_CampObjectScript`; model from `models.clothesBasket`) |
| Cooking | `LZC_CookActivator`, `LZC_CookingStation` | the base for the cook box (named *Campfire*, shows *Cook*, with `LZC_CampObjectScript`); a copy of the vanilla cooking pot furniture with the XMarker's mesh and *Is Marker*, placed hidden at the fire, which activating the box activates |
| Firepit | `LZC_Firepit` | an Activator with the vanilla unlit firepit's model and `LZC_CampObjectScript`, for the firewood mechanic |
| Seats | `LZC_SitMarkerPlayer`, `LZC_BedrollSitMarker`, `LZC_SitMarkerPlayer{White,Plaid…}` | copies of the vanilla cross-legged marker; the coloured fire seats are listed in `LZC_SeatColors` |
| Storage | `LZC_CampStorageContainer` | a container that never respawns |
| Power | `LZC_SetUpCampPower`, `LZC_SetUpCampEffect` | lesser power in the Voice slot; script magic effect |
| Message | `LZC_PackUpConfirmMsg` | *Pack up your camp?* with *Yes* / *No* |
| Global | `LZC_CampActivity` | what followers are doing (see section 2b) |
| Quests | `LZC_CampsiteQuest`, `LZC_CampFollowersQuest` | scripts, aliases, alias scripts and every property filled in |
| Holding cell | `LZC_HoldingCell` | the camp map marker (`LZC_CampMapMarkerRef`), the storage backpack (`LZC_CampStorageRef`), and the clutter sets |

The campfire, light, bedroll and follower sit marker are the vanilla objects themselves, not copies, so mods that change those objects also change the camp's.

For testing, run `help LZC_ 4` in the console to find the kits' form IDs, then `player.additem <id> 1`.

### Clutter sets

The clutter sets are laid out in the holding cell, `LZC_HoldingCell`, each around an anchor that is the object itself (the campfire, a brown small or large tent, the bedroll, the tub), so you can see what you're arranging around. The camp only reads the anchor's position and rotation, so any reference works as an anchor, an `XMarkerHeading` included.

| Set | Anchor | Copied around |
|---|---|---|
| Fire | `LZC_FireClutterAnchor` | the campfire |
| Small tent | `LZC_SmallTentClutterAnchor` | each small tent |
| Large tent | `LZC_LargeTentClutterAnchor` | each large tent |
| Ring bedroll | `LZC_RingBedrollClutterAnchor` | the player's bedroll in a camp without tents |
| Tub | `LZC_TubClutterAnchor` | the bathtub: its water, steps, clothes basket and seats |
| Fire, second set (optional) | `LZC_FireClutterAnchor2` | the fire, instead of the first fire set, in about half of all camps |

Each starts out with just a storage marker (see below). To fill a set, **arrange clutter around its anchor by eye in the Creation Kit**:
1. Open `LZC_HoldingCell`. Each anchor points the way its object faces: a tent's entrance, or along a bedroll. It helps to place a real tent or bedroll on the anchor while you work, then delete it.
2. Place the clutter. Use **static** clutter, not items that can be picked up, or players could take a fresh pelt from every camp. Make each item a **persistent** reference.
3. Add each item to the set's list on `LZC_CampsiteQuest`'s script: `LZC_FireClutter`, `LZC_SmallTentClutter`, `LZC_LargeTentClutter`, `LZC_RingBedrollClutter` or `LZC_TubClutter`.

The camp copies the arrangement around each fire, tent or tentless bedroll, keeping each item's offset, height, turn, lean and scale. The holding cell floor is z = 0: an anchor's height above it is how far its camp object is raised in game (lift a tent anchor 10 units and that tent stands 10 units higher in every camp), and each item is as high above the ground under its camp object as it is above the floor, so moving an anchor up or down doesn't carry the clutter with it. The clutter is fixed rigidly to its object, always level: the ground and slope under each item play no part, so it sits around the object exactly as arranged. Likewise an anchor's turn turns its camp object: rotate the tub anchor and its clutter together by 40° in the holding cell, and the tub and everything around it stand turned 40° in every camp. With `AlignToSlope` on, the camp objects tilt with the slope, but their clutter stays level. All the sets together can hold up to 128 items.

**Storage spots.** Each set's storage marker (`LZC_SmallTentStorageMarker` and so on) is a copy of the storage backpack itself, standing exactly where and how the backpack will: move, turn, tilt or raise it (to lean it against something, say). The markers are listed both in their set and in `LZC_CampStorageMarkers`. When a camp goes up, the backpack moves to the first storage marker in camp order (the player's tent, or the player's bedroll without tents), matching its position and rotation, and returns to the holding cell on pack-up, with its contents.

**Switching an older plugin's markers to real objects.** In the Creation Kit, select an `XMarkerHeading` anchor or storage marker in `LZC_HoldingCell` and use *Edit > Search and Replace* with *Selected only* to swap its base object for the tent, campfire, bedroll, tub or `LZC_CampStorageContainer`. The reference stays the same one, so the quest's properties still point at it (check they do). Keep the position and rotation as they are.


### 2a. Art

These go under `Data/Meshes/` and `Data/Textures/`, at the paths the plugin's records use:

- **Tent meshes**, small (170 × 170) and large (300 × 200).
- **The player's fire seat:** something visible to sit on, such as a fur pelt or a log, with collision. The player can only activate furniture they can point the crosshair at.
- **The bedroll seat** needs no art: give `LZC_BedrollSitMarker` the vanilla cross-legged sit marker's model, which is invisible in game and has the furniture marker the game needs to seat the player (furniture markers live in the `.nif`). The player reaches it by sneak-activating the bedroll, so it doesn't need to be selectable.
- **The cooking station** needs no art: `LZC_CookingStation` has the editor XMarker's mesh and the *Is Marker* flag, so it isn't drawn in game, and has no furniture marker (no stirring animation) or collision.
- **The storage backpack** mesh.
- **Tent colours.** Brown is each tent mesh's own texture. Each other colour is a texture set (`LZC_TentTexture…`) given to the tent as an alternate texture on its canvas shape.

After changing a mesh, recalculate the record's object bounds in the Creation Kit.

### 2b. How followers behave

This quest runs while a camp is set up. When the camp goes up, its aliases find the followers who are with the player. When the player sits in a fire seat, followers take the other fire seats; when the player sits on a bedroll, they take the other bedrolls' seats. When the player goes to sleep near camp, the script works out which camp bedroll the player is lying in, if any, and gives the followers the others. Partners choose first, taking the free bedrolls next to the player's. Then each follower couple takes two free bedrolls side by side. Everyone else takes the rest in camp order. The camp records which tent each bedroll is in and its place in the row (or around the ring), so this matching doesn't measure distances between bedrolls.

- A **partner** is a follower whose relationship rank with the player is *Lover* (vanilla spouses, and lovers in most romance mods), or who is in the optional `PartnerFaction`.
- A **couple** is two followers whose relationship rank with each other is *Lover*. A partner is always paired with the player rather than another follower.

The camp also uses these when it shares bedrolls out between the tents, so partners and couples get tents with room for both. A global variable tells the followers what to do:

| `LZC_CampActivity` | When | Followers |
|---|---|---|
| 0 | the player is away from camp (further than `CampRadius` from the fire) | follow the player as normal |
| 1 | the player has sat in a fire seat, or on a bedroll's seat | sit in the fire seats, or on the other bedrolls' seats (partners beside the player, couples side by side). They **stay seated after the player gets up**, until the player goes to bed (they go to their bedrolls), packs up the camp, or leaves it: more than `CampRadius` from the fire, or in another worldspace, checked every 5 seconds (they follow). Sitting somewhere else moves them to match. |
| 2 | the player is sleeping within `CampRadius` of the fire | sleep in their bedrolls |
| 3 | the player is at camp, awake and not sitting | sandbox around the campfire (`CampCenter` alias) |

**Making the follower packages (once, in the Creation Kit).** Packages are made in the Creation Kit (it couldn't load generated ones). For each N from 1 to 5, make two packages, both with **Owner Quest** `LZC_CampFollowersQuest`:

| Package | Template | Data | Condition (run on subject) |
|---|---|---|---|
| `LZC_FollowerSleepN` | *Sleep* | Sleep Location: *Near Reference Alias* `BedrollN`, radius 64 | `GetGlobalValue LZC_CampActivity == 2` |
| `LZC_FollowerSitN` | *Sit* | Sit Location: *Near Reference Alias* `SeatN`, radius 64; Search Criteria / target: alias `SeatN` | `GetGlobalValue LZC_CampActivity == 1` |
| `LZC_FollowerSandbox` (one for all) | *Sandbox* | Location: *Near Reference Alias* `CampCenter`, radius about 600 | `GetGlobalValue LZC_CampActivity == 3` |

Give `LZC_CampFollowersQuest` a high **Priority** on its *Quest Data* tab (the plugin uses 100): alias packages compete by quest priority, and the vanilla follower quest's follow package would otherwise win, so followers would never sit or sleep. Then open `LZC_CampFollowersQuest`'s **Quest Aliases** tab, and in each `FollowerN` alias add `LZC_FollowerSleepN` and `LZC_FollowerSitN` to its *Alias Package Data*. Without them followers just follow the player as normal: they don't sit or sleep at camp.

The follower quest's `CampRadius` setting (default 1024, in the follower quest's script) is how near the fire counts as "at camp", for sleeping and for seated followers staying put.

### 2c. Settings on the quest scripts

The number and true/false settings on `LZC_CampsiteQuestScript` and `LZC_CampFollowersScript` start at the defaults written in the scripts. These include `AlignToSlope`, `MaxSlope`, `MaxStep`, `HideGrassUnderCamp`, `PlaceCampfireLight`, the tent capacities, `FirewoodMechanic`, `FireBurnHours`, `PlaceChoppingBlock`, `PlaceTub`, `TubInLargeTent` and `CampRadius`. Change them on the quests' Scripts tab in the Creation Kit; each is described in a comment next to it in the `.psc` file. The camp's layout distances (seat ring, tent distances and sizes, bedroll spacing, where the tub stands and how it turns, the light's height) are fixed constants in `SKSE/src/CampLayout.cpp`: change them there and rebuild the DLL.

### 2d. How players get the kits

**Merchants.** `LazyCamping.esp` has a leveled list of every kit, `LZC_LItemCampingKits` (one random kit, 50% chance none), and a form list, `LZC_VendorLists`, of merchant leveled lists to add it to: fill that in the Creation Kit with the general goods vendor lists (the leveled lists their merchant chests use). LazyCamping.dll adds the kit list to each of them while the game runs, so no vanilla list is overridden in the plugin and nothing conflicts with other mods that edit them. The SKSE menu's *Sell kits at general goods merchants* turns it off (`KitsAtMerchants`); merchants show the change at their next restock.


The committed `LazyCamping.esp` has a recipe for every kit (`LZC_Recipe…`), all at a tanning rack (`CraftingTanningRack`), every colour alike:

- Bedroll = 2× Linen Wrap + 2× `LeatherStrips`
- Small tent kit = 3× Linen Wrap + 3× `LeatherStrips` + 2× `Firewood01`
- Large tent kit = 6× Linen Wrap + 5× `LeatherStrips` + 3× `Firewood01`

Kits added in the Creation Kit need a recipe of their own. Other ideas:
- **Merchants:** add the kits to a general goods leveled list, such as `LItemMiscVendorMiscItems75`.

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
2. Download the **LazyCamping** artifact from the run's page.

You can also start a build by hand with *Run workflow*.

xmake downloads the dependencies (such as spdlog) on its first run. The DLL ends up in `SKSE/dist/SKSE/Plugins/LazyCamping.dll`.

The plugin uses Address Library, so the same DLL works on SE (1.5.97), AE (1.6.x) and VR. Players need **SKSE** and **Address Library for SKSE Plugins** installed. The plugin writes a log to `Documents/My Games/Skyrim Special Edition/SKSE/LazyCamping.log`.

> CommonLibSSE-NG has maintained forks, such as alandtse's `CommonLibVR` (`ng` branch). If a future game update breaks the build, point the `SKSE/lib/commonlibsse-ng` submodule at one of those. The plugin only uses a handful of stable APIs.

## 4. Compile scripts and package

1. In the CK, compile all six scripts (*Gameplay > Papyrus Script Manager*). `LZC_Native` compiles without the DLL; its functions are only bound at runtime.
2. `LazyCamping.esp` adds only new records, so you can flag it as ESL with xEdit.
3. Package these files:
   ```
   LazyCamping.esp
   Scripts/LZC_CampsiteQuestScript.pex
   Scripts/LZC_SetUpCampEffect.pex
   Scripts/LZC_CampObjectScript.pex
   Scripts/LZC_Native.pex
   Scripts/LZC_CampFollowersScript.pex
   Scripts/LZC_CampPlayerAliasScript.pex
   Meshes/LazyCamping/...          (tents, seats, backpack: section 2a)
   Textures/LazyCamping/...        (the tent recolours)
   SKSE/Plugins/LazyCamping.dll
   Source/Scripts/*.psc            (optional but polite)
   ```
   List SKSE and Address Library for SKSE Plugins as requirements on your mod page.

## 5. Known limitations and ideas

- **Testing the tilt**: the slope math matches the game's `EulerAnglesToAxesZXY` rotation, and I checked it numerically. It still needs an in-game check on a hillside. If objects lean the wrong way, set `AlignToSlope` to false while you report or fix it; ground snapping and the site checks don't depend on it.
- **Hiding grass**: Skyrim builds a cell's grass when the cell loads, so grass doesn't move aside for objects placed later. `GrassHider.cpp` edits the grass that's already loaded:
  - it puts a circle around the placed objects (by their bounds), finds the grass instances inside it, then removes their group and adds it back without them. Grass well above or below the camp, such as on a cliff top, stays.
  - grass from *No Grass In Objects*' cache ends up in the same groups, so this works with it
  - while the camp is up, it checks once a second for grass loading in around the camp (walking back to it, loading a save) and hides that too
  - the camp is saved with the game through SKSE, so this survives a reload
  - on pack-up, the camp's cells rebuild their grass from scratch
  
  The grass layouts it relies on aren't in CommonLibSSE-NG; they come from *No Grass In Objects NG* (also GPL-3.0) and *Community Shaders*. **This is untested in game.** If grass isn't where the code expects (another game version, or a mod that changes grass), it stops and writes a warning to `LazyCamping.log` rather than removing the wrong grass. If grass flickers, the game stutters or crashes around a camp, set `HideGrassUnderCamp` to false. It doesn't run in Skyrim VR.
- **Footprint check**: the check samples 5 points (the centre and four corners). Something small that sits between those points can still poke through the tent. You can add more sample points in `IsAreaClear` if needed.
- **One camp at a time**: this is by design. Casting the power while a camp is up offers to pack that camp up, wherever it is; cast again to make a new one.
- **Bedroll fan**: neighbouring bedrolls in a tentless camp are 70° apart, about 218 units between centres at the default `BedrollRingRadius` (190). With the vanilla bedroll (128 long, assumed up to 64 wide) that leaves about 90 units between their ends; the limit is the seats in front, which are assumed to need 35 units behind the marker. If seated people have room to spare in game, `BedrollRingRadius` can go down to about 180.
- **Sneak to sit**: `BedrollSeats.cpp` hooks furniture activation. When the player sneaks and activates a camp bedroll, it activates that bedroll's seat instead, so the bedroll stays the plain vanilla one and anyone else (followers) uses it as normal.
- **Cooking**: the vanilla campfire can't be activated, so a **cook box** does it: make it once in the Creation Kit. In `LZC_HoldingCell`, click *Create Trigger* with nothing selected and choose `LZC_CookActivator`; on the box's *Primitive* tab tick **Player Activation** (so the crosshair picks it up) and make it about 60 × 60 × 40; tick *Persistent* and *Initially Disabled*, and fill `LZC_CookBox` on `LZC_CampsiteQuest` with it. While a camp's fire is lit, the box sits over it and shows *Cook*; activating it activates `LZC_CookingStation`, which has the cooking pot's crafting keyword and no furniture marker, so the Cook menu should open without moving the player. Check that in game.
- **Tub trigger** (optional): in `LZC_HoldingCell`, click *Create Trigger* with nothing selected and choose `LZC_CookActivator` again; leave the box on the trigger layer (don't tick Player Activation). Place and size it over the water in the tub clutter set, like any clutter item, so followers only step into it in the tub. Tick *Initially Disabled* and fill `LZC_TubTrigger` on `LZC_CampsiteQuest` with it (that makes it persistent). The camp moves it onto each tub, keeping its offset and turn from the tub anchor; the offset is read the first time, while the trigger is still in the holding cell.
- **Base Object Swapper**: in testing, BOS didn't swap the references the camp places at runtime, so variance mods don't change the camp's bedrolls. The camp's scripts don't depend on the base object, so this only affects looks.
- **Map marker**: moving a persistent map marker is how other camping mods do it. Still, check in game that the marker appears at the camp on the world map, moves with each new camp, and disappears when you pack up.
- **Clutter**: each copy takes its scale from its clutter item in the holding cell, set the way Papyrus's `SetScale` does it, so it shows at that size straight away and the save keeps it. A camp with many cluttered tents can place well over 100 objects; that's fine for the game, but keep sets modest.
- **Tent inside check**: "inside" means within the tent's `HalfWidth` × `HalfDepth` footprint. If activating the tent from just outside its entrance does nothing, shrink those values a little.
- **Follower count**: followers are counted when the camp goes up, up to 3, because the follower quest has 3 follower aliases. With more than 3 follower seats, the extra seats stay empty. Someone who joins or leaves afterwards keeps their old status until the next camp.
- **Expansion ideas**:
  - a "Well Rested" style bonus for sleeping in your own tent (check `OnSleepStop` with a player alias)
  - fire damage when standing in the fire (a hazard)
  - a storage chest (place a container ref and empty it into the player on pack-up)
  - weather-dependent warmth for survival mode
