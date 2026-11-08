# Wayfarer - A Convenient Camping Framework

A camping mod for Skyrim Special Edition.

The mod adds a **Set Up Camp** Lesser Power, named " Set Up Camp" with a leading space so it sorts to the top of the magic menu. When the player has at least one **Bedroll** in their inventory, casting the power builds a camp in front of the player, always with a campfire. Tent kits are optional. The kits are **reusable**: setting up camp doesn't take them from the inventory. The mod requires **SKSE**, **Address Library for SKSE Plugins**, and its own `LazyCamping.dll`. The camp includes:

- a burning campfire with flickering light. **Activating it opens the Cook menu** (no animation, as there's no pot to stir).
  - with the optional **firewood mechanic** (`FirewoodMechanic`), the fire starts out as an unlit firepit. Activating it with `Firewood01` in the inventory lights it, burning 2 (`FirewoodPerFire`, 1 to 6 on the SKSE menu); it burns for 6 game hours (`FireBurnHours`), then goes out and can be lit again. A woodcutting block stands beside the outermost tent (or along the bedroll fan without tents) for chopping more.
- cross-legged seats around the fire, facing it:
  - one for the player, which the player can sit in. It's across the fire from where they stood to cast the power, in front of their tent's entrance.
  - one for followers per bedroll beyond the player's, up to 5, fanning out from the player's seat every 60°: 60° right, 60° left, 120° right, 120° left, and the fifth on the near side, where the player stood. The two at 60° use the player's seat, so the player can sit in them too, and they're there even when camping alone.
- **tents in an amphitheater** on the far side of the fire, outside the seats, with their entrances facing the fire and the open side where the player stands:
  - up to 3 tents, with **at most one large tent**: the first large tent kit the player carries, in colour order. Every small tent kit carried, duplicates included, fills the rest.
  - **followers' kits count too**: tent and bedroll kits in the inventories of the followers at camp are counted with the player's, so a follower carrying a tent kit and a bedroll kit brings their own tent. At bedtime (and when sitting on bedrolls), a follower takes a bedroll in a tent from their own kit first, their partner among the followers beside them; followers' kits are used for tents before the player's. No other mod is needed. (The player still needs a bedroll kit of their own to cast the power.) Kits a follower framework keeps for them count too: with *Nether's Follower Framework* (looked up at run time, not a master), a follower's extra storage (their trading inventory), or their satchel if they have a custom NFF outfit.
  - each tent's size and colour come from its kit. There are small and large tents, each in brown, blue, green, red and white: 10 tent kits in all.
  - every tent gets at least one bedroll, so there are never more tents than bedrolls. With 3 tent kits and 2 bedrolls, for example, only 2 tents go up.
  - small tents are taken in colour order: brown, blue, green, red, white
- **bedrolls in the tents**:
  - the player's tent gets a bedroll for the player and one for each partner (spouse or lover) travelling with them
  - each follower couple gets a tent with two bedrolls
  - every other tent gets one bedroll
  - if there aren't enough bedrolls for that, fewer tents go up rather than splitting anyone up. With 2 tent kits, 2 bedrolls and a spouse, for example, one tent goes up with both bedrolls in it.
  - spare bedrolls then fill the tents in turn, up to each tent's capacity: 2 for a small tent, 4 for a large one
  - bedrolls the full tents can't hold (up to 3) lie outside, sideways just outside the seats on the player's side of the fire, where no tent stands: straight behind where the player stood, then 50° to either side. Followers get them after the tents fill up. With 1 small tent and 4 bedrolls, for example, the tent holds 2 and 2 lie outside.
  - in a tent, the bedrolls lie in pairs that touch, with a small gap between pairs (`BedrollSpacing` minus `PairedBedrollSpacing`), so a couple (or the player and a partner) shares a touching pair. They run left to right as seen from the fire, the player's first. A large tent's row is laid out for a full tent, 1+2, gap, 3+4, and fills from the far left; it sits 60 further from the fire than the tent's centre (`LargeTentBedrollBack`), and can be shifted sideways (`LargeTentBedrollLeft`). A small tent's row is centred.
- **the player's tent** stands in the middle, straight across the fire from the player, entrance facing them, with the player's bedroll in it. It's the large tent if there is one; otherwise it's the small tent with the most bedrolls.
- **the other tents** go to either side of it: the second on the right, the third on the left. With only small tents, they stand 70° round the fire to either side, all 250 from its centre, facing it. Beside the large tent (107 from the fire, so the fire sits under its front; with `FireOutsideLargeTent` on, 285 further back, so the fire is outside in front of it, and the small tents beside it 265 further back too, facing the fire, so the three stand in an arc around it), the small tents come round toward the player's side and angle in, so the three entrances face each other around the fire. All of these spots are constants in `SKSE/src/CampLayout.cpp`.
- **with no tent kits**, there are no tents. The bedrolls (up to 6) lie sideways just outside the seats: up to 4 in a close amphitheater arc, 50° apart round the fire, and 5 or 6 in a full ring, evenly spaced all the way round (72° or 60° apart). The tub stands outside the bedrolls on the left of the player's, and the chopping block on its right, just past its clutter, a little further from the fire than the tub. The player's is straight across the fire, behind their seat, the rest to either side. The player and a partner, and a follower couple, get neighbouring places with their bedrolls head to head.

A **bathtub** (`LZC_Tub`, a static with your tub mesh) stands beside the outermost tent, on the other side from the chopping block, or, without tents, just left of the player's bedroll as the player sees it across the fire. It can be turned off (`PlaceTub`). With `TubInLargeTent` on, it goes inside the large tent instead, in place of the third and fourth bedrolls, 137 right of the tent's centre (`TubInTentRight`) and turned like the bedrolls; the large tent then holds 2 bedrolls at most, and the chopping block stays outside. What's in and around it is the **tub clutter set**, arranged in the Creation Kit: by default the vanilla rain barrel water (`LargeRainBarrelWater01`) in it and a **clothes basket** (`LZC_ClothesBasket`): activating it takes off everything the player is wearing, and activating it again puts it all back on; add steps, invisible seats inside the tub and so on. The **water** should be the clutter set's own reference, filled into `LZC_TubWater`: instead of being copied it moves onto the tub (keeping its offset, turn and scale), because the game only treats placeable water as real water, which bathing mods such as *Bathing in Skyrim* look for, when the reference was placed in the Creation Kit. An optional **tub trigger** (`LZC_TubTrigger`) moves onto the tub: followers who step into it take off their clothes, and put them back on when they step out or the camp is packed up. Its water is real water to the game, so *Bathing in Skyrim* lets you bathe in it.

**Clutter sets** add clutter around the camp, such as pelts, packs, firewood and lanterns. You arrange each set by eye in the Creation Kit around a stand-in marker, and every fire, tent or tentless bedroll gets a copy, turned to match. One **backpack** can be the player's storage: it's a single real container that moves from camp to camp, so whatever's in it stays there.

While a camp is up, **the grass is cleared** in a circle around it, just big enough to hold the tents, bedrolls, fire, seats, clutter and storage, so nothing pokes through them. It comes back when the camp is packed up. This works with or without *No Grass In Objects* and its grass cache.

While a camp is up, a **map marker** shows where it is, and the player can fast travel to it, unless fast travel is disabled globally (for example in Survival Mode). Packing up removes the marker. It can be turned off on the SKSE menu page (`PlaceMapMarker`).

Every bedroll also has a **seat on it**, facing the fire. Followers sit at the fire when the player sits in any of the three fire seats they can use, and on the other bedrolls when the player sits on a bedroll, matched the same way as at bedtime. Once seated, they stay seated after the player gets up, until the player goes to bed, packs up the camp, or leaves it. When the player sits in one of the tub's seats (furniture in the tub clutter set), one follower, a spouse or lover first, sits in another. When the player sleeps at camp, in any of the camp's bedrolls, followers sleep in the bedrolls the player isn't using. A spouse or lover gets the bedroll next to the player's: the one beside them in the same tent, or the neighbouring one in a camp without tents. Two followers who are a couple get two free bedrolls side by side, normally in their own tent.

With **Bedrolls start rolled up** on (SKSE menu, on by default), every bedroll, in the tents or not, starts as a rolled-up bedroll (`LZC_BedrollRolled`, an Activator with the kit's model) dropped where it will lie. Activating one unrolls the bedroll in its place, with its seat. The player's bedroll clutter (without tents) is there from the start. The bedroll's spot is recorded when the camp goes up, so a rolled-up bedroll knocked away (its model can have Havok) still unrolls in the right place. Followers unroll their own at bedtime if the player hasn't, and don't sit on bedrolls still rolled up. Packing up removes them, rolled or not.

**With Campfire installed** (and so **Frostfall**), looked up at run time rather than as a master: the camp's tents get Campfire's *warm* and *waterproof* tent keywords (`isCampfireTentWarm`, `isCampfireTentWaterproof`), and while a camp is up the DLL tells Campfire which camp tent the player is standing in (`_CampInternal.SetCurrentTent`), as Campfire's own tents do. Frostfall then treats the player inside one as sheltered from rain and snow and warm.

**Sneaking and activating any of the camp's bedrolls** sits the player cross-legged on it (on its seat), with or without tents. Activating a bedroll without sneaking opens the usual Sleep/Wait menu.

While a camp is up, the power is called **Pack Up Camp** (" Pack Up Camp"). It keeps its place in favourites and the voice slot. **Casting it** asks *Pack up your camp?* and packs it up if confirmed, from anywhere, so a camp left behind never stops the player making a new one. It's the only way to pack up: activating the tents does nothing.

With **SKSE Menu Framework** installed (optional), its Mod Control Panel gets a *Convenient Camping (Wayfarer)* page in four collapsible sections: **Camp** (aligning the camp to the slope, clearing the grass, fading to black during setup and pack-up (off by default), tent collision (on by default; off lets you walk through the tents' walls, applied straight away to a camp that's up), the map marker, the bathtub and whether it goes in the large tent), **Bedrolls** (starting them rolled up, and their style: with *Bedroll Alternative - BOS Color Variance*, its green, blue or red bedroll, or all three in turn, each touching pair in a tent matching, the bedroll kit item taking the chosen colour too, green for mixed; with *Ash Nomad - Bedrolls - BOS Distributed*, its blue, red or purple bedroll, or all three in turn; each mod's styles greyed out without it), **Fire** (the firepit, which off means no fire, fire clutter, light or cooking, and so no firewood mechanic or chopping block; the fire outside the large tent (off by default); the campfire light marker; the firewood mechanic, how long a fire burns, how many firewood lighting it takes and the wood chopping block; the fireside cushions and their colour) and **Leveled lists** (selling the kits at general goods merchants). The settings are kept in the save. Without it, change them on the quest in the Creation Kit.

All EditorIDs use the prefix `LZC_`.

---

## 1. Files

```
Scripts/Source/
  LZC_CampsiteQuestScript.psc   main logic: checks, camp objects, pack-up, state
  LZC_CampFollowersScript.psc   followers' seats and bedrolls; sit/sleep with the player
  LZC_TubTriggerScript.psc      on the tub's trigger box; passes followers stepping in and out to the quest
SKSE/                           C++ plugin (LazyCamping.dll), required
  xmake.lua
  src/main.cpp                  plugin entry point, logging
  src/Papyrus.cpp               native functions (declared in LZC_CampsiteQuestScript); ground checks and placing
  src/Events.cpp                casting the power, activating camp objects, sitting down
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
Wayfarer - A Convenient Camping Framework.esp
                                the plugin, edited in the Creation Kit
```

### How the scripts and the plugin split the work

Setting up a camp is one call into `LazyCamping.dll`, **`BuildCamp`**, so it happens in a single frame instead of being spread over many by Papyrus's small time budget. From `LZC_CampsiteQuestScript`'s properties it:

1. works out which tents and bedrolls go up and where everything stands relative to the player (`SKSE/src/CampLayout.cpp`, where the layout distances and angles live);
2. checks the ground under the fire and every tent's footprint, and refuses the camp if the ground is too steep, uneven, blocked by rocks or logs, underwater, or missing;
3. finds the ground under every object, tilts each one to the slope of the ground under its whole footprint (a plane through the ground at its corners, up to 10 degrees, 6 for the tub), and creates them all with their clutter at their final positions. It finds the ground for everything before placing anything, so a bedroll's ground check can't hit the tent standing over it;
4. moves the cook box, storage backpack, map marker and the tub's trigger box and water to the camp, sets up the bedroll seats, and clears the grass in a circle around the camp (see *Hiding grass* below).

The script keeps the rest: what happens on the camp's events, the follower quest, which it hands the camp's seats and bedrolls, and who is in the tub. Packing up is one native call too (`PackUpRefs`), so the whole camp disappears in the same frame. The events themselves (casting the power, activating a camp object, sitting down) are caught by the DLL and passed to the scripts, so only the two quests carry scripts, plus `LZC_TubTriggerScript` on the tub's trigger box, which tells the camp script when a follower steps into or out of the tub. Packing up calls **`ShowGrass`**.

If the DLL isn't loaded, or is older than the scripts expect, casting the power tells the player that Wayfarer needs SKSE, and nothing is placed.

Compiled `.pex` files go in `Data/Scripts/`.

---

## 2. The plugin (`Wayfarer - A Convenient Camping Framework.esp`)

`Wayfarer - A Convenient Camping Framework.esp` is edited in the Creation Kit and committed to the repository like any other file.

**Before editing it in the Creation Kit,** take the latest `Wayfarer - A Convenient Camping Framework.esp` from the repository: some records (the recipes, the merchant list) are sometimes added to it directly, and saving over an older copy loses them. After saving, check that the quest's object properties are still filled.

### What's in it

| Records | EditorIDs | Notes |
|---|---|---|
| Kits | `LZC_BedrollKit`, `LZC_TentKitSmallBrown` … `LZC_TentKitLargePlaidBlue` | MiscItems: the bedroll and a small and large tent in each colour (brown, blue, green, red, white, plaid green, plaid yellow, plaid blue) |
| Rolled-up bedroll | `LZC_BedrollRolled`, `LZC_BedrollRolledGreen`, `LZC_BedrollRolledBlue`, `LZC_BedrollRolledRed` | Activator with the bedroll kit's model (the coloured copies, in `LZC_BedrollRolledColors`, match Bedroll Alternative's bedrolls: their cloth, shape index 0, takes `LZC_BedrollTextureGreen`, `Blue` or `Red`) and *Unroll* as its activation text, dropped in place of each bedroll with `BedrollsStartRolled`; give its model Havok for it to roll about |
| Recipes | `LZC_RecipeBedrollKit`, `LZC_RecipeTentKit…` | one per kit, at a tanning rack (see 2d) |
| Merchants | `LZC_LItemCampingKits`, `LZC_VendorLists` | a leveled list of the kits, and the merchant lists it's added to (see 2d) |
| Tents | `LZC_TentSmallBrown` … `LZC_TentLargeFur` | Activators; colours are texture sets swapped onto the canvas |
| Tent lists | `LZC_SmallTentKitList`, `LZC_LargeTentKitList`, `LZC_SmallTentList`, `LZC_LargeTentList` | form lists of the plugin's own tent kits and tents, in colour order, kit list and tent list matching entry for entry. LazyCamping.dll reads these rather than the quest's `LZC_…TentKits` / `LZC_…Tents` arrays (used only if a list is missing or empty), because a script's array keeps the value it had when a save first ran it, while a form list is read from the plugin on every load: a tent added to the lists shows up in existing saves. Add new tents here, and to the arrays too if you like |
| Bath | `LZC_Tub`, `LZC_ClothesBasket` | the tub (static, your mesh); a clothes basket activator (*Use*: undress, then dress again; model from `models.clothesBasket`) |
| Cooking | `LZC_CookActivator`, `LZC_CookingStation` | the base for the cook box (named *Campfire*, shows *Cook*); a copy of the vanilla cooking pot furniture with the XMarker's mesh and *Is Marker*, placed hidden at the fire, which activating the box activates |
| Firepit | `LZC_Firepit` | an Activator with the vanilla unlit firepit's model, for the firewood mechanic |
| Seats | `LZC_SitMarkerPlayer`, `LZC_BedrollSitMarker`, `LZC_SitMarkerPlayer{White,Plaid…}` | copies of the vanilla cross-legged marker; the coloured fire seats are listed in `LZC_SeatColors` |
| Storage | `LZC_CampStorageContainer` | a container that never respawns |
| Power | `LZC_SetUpCampPower`, `LZC_SetUpCampEffect` | lesser power in the Voice slot; LazyCamping.dll notices it being cast |
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

The camp copies the arrangement around each fire, tent or tentless bedroll, keeping each item's offset, height, turn, lean and scale. The holding cell floor is z = 0: an anchor's height above it is how far its camp object is raised in game (lift a tent anchor 10 units and that tent stands 10 units higher in every camp), and each item is as high above the ground under its camp object as it is above the floor, so moving an anchor up or down doesn't carry the clutter with it. The clutter is fixed rigidly to its object, level (the tub's tilts with it): the ground and slope under each item play no part, so it sits around the object exactly as arranged. Likewise an anchor's turn turns its camp object: rotate the tub anchor and its clutter together by 40° in the holding cell, and the tub and everything around it stand turned 40° in every camp. With `AlignToSlope` on, the camp objects tilt with the slope, but their clutter stays level, except the tub's, which tilts with the tub (the tub's water still stands level, lowered by however far the tilted rim drops on its low side, so it doesn't show through). All the sets together can hold up to 128 items.

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

The number and true/false settings on `LZC_CampsiteQuestScript` and `LZC_CampFollowersScript` start at the defaults written in the scripts. These include `AlignToSlope`, `HideGrassUnderCamp`, `PlaceCampfire`, `PlaceFireSeats`, `FadeOnSetUp`, `PlaceCampfireLight`, `PlaceMapMarker`, `BedrollsStartRolled`, the tent capacities, `FirewoodMechanic`, `FireBurnHours`, `PlaceChoppingBlock`, `PlaceTub`, `TubInLargeTent`, `FireOutsideLargeTent`, `TentCollision` and `CampRadius`. Change them on the quests' Scripts tab in the Creation Kit; each is described in a comment next to it in the `.psc` file. The camp's layout distances (seat ring, tent distances and sizes, bedroll spacing, where the tub stands and how it turns, the light's height) and the ground limits (steepest ground allowed, 30°; most an object tilts, 10°, or 6° for the tub; how uneven a tent's footprint may be, 48) are fixed constants in `SKSE/src/CampLayout.cpp`: change them there and rebuild the DLL.

### 2d. How players get the kits

**Merchants.** `Wayfarer - A Convenient Camping Framework.esp` has a leveled list of every kit, `LZC_LItemCampingKits` (one random kit, 50% chance none), and a form list, `LZC_VendorLists`, of merchant leveled lists to add it to: fill that in the Creation Kit with the general goods vendor lists (the leveled lists their merchant chests use). LazyCamping.dll adds the kit list to each of them once, when the game's data has loaded, so no vanilla list is overridden in the plugin and nothing conflicts with other mods that edit them. It never changes a merchant list after that: once a save is loaded, other mods' scripts may have added entries to it (Papyrus `AddForm`), and the game keeps pointers into the list that resizing it would leave dangling, crashing the next save. So the SKSE menu's *Sell kits at general goods merchants* (`KitsAtMerchants`) only sets `LZC_LItemCampingKits`' own chance of giving nothing (its 50% when on, 100% when off); merchants show the change at their next restock.


The committed `Wayfarer - A Convenient Camping Framework.esp` has a recipe for every kit (`LZC_Recipe…`), all at a tanning rack (`CraftingTanningRack`), every colour alike:

- Bedroll = 2× Linen Wrap + 2× `LeatherStrips`
- Small tent kit = 3× Linen Wrap + 3× `LeatherStrips` + 2× `Firewood01`
- Large tent kit = 6× Linen Wrap + 5× `LeatherStrips` + 3× `Firewood01`

Kits added in the Creation Kit need a recipe of their own. Other ideas:
- **Merchants:** add the kits to a general goods leveled list, such as `LItemMiscVendorMiscItems75`.

---

### 2e. Tent addons

Other plugins can add tents, each with its own clutter, without editing this one: an **addon** is a separate plugin with `Wayfarer - A Convenient Camping Framework.esp` as a master (and any mod whose meshes it uses). LazyCamping.dll finds addon tents when a camp goes up; nothing in this plugin needs changing, and several addons can be installed at once.

For each tent, the addon has:

1. a **tent kit**: a MiscObject the player (or a follower) carries, like `LZC_TentKitSmallBrown`;
2. a **tent**: an Activator with the tent's mesh, like `LZC_TentSmallBrown` (same footprint as the plugin's small or large tents: about 170 × 170 or 300 × 200 units);
3. optionally its **own clutter set**: in a holding cell of the addon's own, an anchor standing in for the tent (its turn and height turn and raise the tent, as for the plugin's sets) and the clutter items arranged around it. The anchor's base object must be its own (a copy of `XMarkerHeading`, say, used for nothing else), each item's *Linked Ref* must be the anchor, and the anchor and items must all be *Persistent References* (so they're in memory although the holding cell never loads). A copy of the storage backpack among them marks where the player's storage goes when it's the player's tent. Without a set of its own, the tent gets the plugin's small or large tent clutter;
4. a **form list** tying them together, in this order:
   - `LZC_TentAddonSmall` or `LZC_TentAddonLarge` (keywords from this plugin), for the size of every tent in the list,
   - each tent's kit followed by its tent (as many kit/tent pairs as you like, e.g. one per color),
   - and, for their own clutter, the anchor's **base object** (the Creation Kit's form lists hold base objects, not references; the items are found through their linked ref). All the tents in the list share this clutter set; put a tent in a list of its own to give it different clutter. (A list edited in xEdit can hold the anchor and item references themselves instead.)

   - optionally one or more **bedrolls** (Furniture) for the tents in the list, used in place of the style chosen in the SKSE menu. With several (Ash Nomad's blue, red and purple, say), they're taken in turn, the two of a touching pair matching.
   - optionally a **form list** (nested) holding the **fire's clutter** for when one of these tents is the player's: its anchor's base object (items linked to it, as above), or the anchor and item references. It replaces the plugin's fire clutter sets for that camp; an empty nested list means no fire clutter at all.
   - or, to use another mod's bedrolls without making it a master, a **Global Variable** of the addon's own whose value is a bedroll style, as in the SKSE menu's *Bedroll style*: 1-4 Bedroll Alternative's green, blue, red or mixed; 5-8 Ash Nomad's blue, red, purple or mixed. Its bedrolls are looked up at run time; if that mod isn't installed, the tents follow the player's chosen style instead.

   A list that repeats tents another list already added **updates** them rather than adding them twice: its bedroll, and its clutter if it has any, replace theirs. That's how an optional patch, with the addon and another mod as masters, can give the addon's tents that mod's bedrolls only when both are installed: a form list with the same keyword, the same kit/tent pairs, and the other mod's bedroll.

   The editor ID of the list doesn't matter (for example `LZC_TentAddonAshSmall`); the keyword first is what marks it.

Tents from addon kits are chosen like the plugin's own (one large tent at most, then small tents; addon tents after the plugin's colours), hold bedrolls the same way, and are packed up with the rest. `LazyCamping.log` lists each addon tent it finds (`Tent addon …`) and warns about a list that's missing its kit or tent. Addon kits can be given recipes or added to merchants by the addon itself.

### 2f. For other mods: camp events

Other mods can react to the camp without making this plugin a master, for example a follower mod switching its followers to a relaxed mode while the camp is up. `LZC_CampsiteQuestScript` sends two SKSE mod events:

| Event | When |
|---|---|
| `Wayfarer_CampSetUp` | once the camp is up |
| `Wayfarer_CampPackedUp` | as pack-up starts, while the camp and its followers are still there |

Both carry the camp's centre (the campfire, or an invisible marker in its place when the firepit is off) and how many followers are at the camp:

```papyrus
Event OnPlayerLoadGame()  ; and once when your script starts: mod events need registering on every load
	RegisterForModEvent("Wayfarer_CampSetUp", "OnWayfarerCampSetUp")
	RegisterForModEvent("Wayfarer_CampPackedUp", "OnWayfarerCampPackedUp")
EndEvent

Event OnWayfarerCampSetUp(Form akCampCentre, Int aiFollowers)
	ObjectReference centre = akCampCentre as ObjectReference
	; e.g. put your followers in relax mode around centre
EndEvent

Event OnWayfarerCampPackedUp(Form akCampCentre, Int aiFollowers)
	; e.g. back to following
EndEvent
```

To check the camp at any other time (a save loaded with the camp up, say), with `LZC_CampsiteQuestScript.psc` to compile against, still with no master: `LZC_CampsiteQuestScript.IsCampUp()` and `LZC_CampsiteQuestScript.GetCampCentre()` (None without a camp). Both are global functions that find the quest with `Game.GetFormFromFile`. Without this mod installed its script isn't there either, so check first with `Game.IsPluginInstalled("Wayfarer - A Convenient Camping Framework.esp")` (not `GetModByName`, which doesn't find light plugins) (otherwise the call just fails, with an error in the Papyrus log). The events need no such check: without this mod they simply never arrive.

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

(If you already cloned without `--recursive`, run `git submodule update --init` first. CommonLibSSE-NG, alandtse's maintained fork, is pinned as a submodule in `SKSE/lib/commonlibsse-ng`, at a release tag.)

The full Visual Studio IDE isn't required. The free **Build Tools for Visual Studio**, with only the *Desktop development with C++* workload selected, are enough.

### Building without a Windows PC

Every push to `main`, and every pull request, runs `.github/workflows/build.yml` on GitHub's Windows machines. To get the DLL:
1. Open the **Actions** tab on GitHub and select a run.
2. Download the **LazyCamping** artifact from the run's page.

You can also start a build by hand with *Run workflow*.

xmake downloads the dependencies (such as spdlog) on its first run. The DLL ends up in `SKSE/dist/SKSE/Plugins/LazyCamping.dll`.

The plugin uses Address Library, so the same DLL works on SE (1.5.97), AE (1.6.x and 1.7.x) and VR. Players need **SKSE** and **Address Library for SKSE Plugins** installed, both matching their game version: on 1.7.99, SKSE 2.3.0 or newer and Address Library 12 or newer; on 1.7.104, Address Library 13 or newer. The plugin writes a log to `Documents/My Games/Skyrim Special Edition/SKSE/LazyCamping.log`.

> If a future game update stops the plugin loading (as 1.7.99 did for plugins built on older CommonLib), move the `SKSE/lib/commonlibsse-ng` submodule to the latest release tag of [alandtse/CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG) and rebuild. The plugin only uses a handful of stable APIs.

## 4. Compile scripts and package

1. In the CK, compile the three scripts, `LZC_CampsiteQuestScript`, `LZC_CampFollowersScript` and `LZC_TubTriggerScript` (*Gameplay > Papyrus Script Manager*). The quest script's native functions compile without the DLL; they're only bound at runtime.
2. `Wayfarer - A Convenient Camping Framework.esp` adds only new records, so it can be flagged as ESL, but first its FormIDs must be compacted into the light plugin range (up to `0xFFF`): in xEdit, right-click it, *Compact FormIDs for ESL*, then set the ESL flag (or *File > Compact Active File Form IDs* in the Creation Kit). Compacting renumbers its records, which breaks saves made with it. The DLL finds every form through the quest scripts' properties (the bedroll kit's texture sets are `LZC_BedrollTextureGreen`, `LZC_BedrollTextureBlue` and `LZC_BedrollTextureRed`), so nothing in it needs changing.
3. Package these files:
   ```
   Wayfarer - A Convenient Camping Framework.esp
   Scripts/LZC_CampsiteQuestScript.pex
   Scripts/LZC_CampFollowersScript.pex
   Scripts/LZC_TubTriggerScript.pex
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
  - **not with No Grass In Objects' `Extend-grass-distance = true` and `Only-load-from-cache = true` together**: in that mode NGIO keeps all grass, the camp's cells included, on stand-in cells of its own that other plugins can't reach, so there's nothing for the camp to clear (the log shows `0 grass type(s)`). With `Extend-grass-distance = false` in `GrassControl.ini`, grass clearing works. LazyCamping.dll reads `GrassControl.ini` at start-up and writes a warning to `LazyCamping.log` when both are on
  - while the camp is up, it checks once a second for grass loading in around the camp (walking back to it, loading a save) and hides that too
  - the camp is saved with the game through SKSE, so this survives a reload
  - the removed grass is kept and, on pack-up, added back to the same grass shapes it came from, so nothing is regenerated and it works the same with vanilla grass, a grass cache or No Grass In Objects generating grass (cells that unloaded meanwhile simply regrow it when they load)
  
  The grass layouts it relies on aren't in CommonLibSSE-NG; they come from *No Grass In Objects NG* (also GPL-3.0) and *Community Shaders*. **This is untested in game.** If grass isn't where the code expects (another game version, or a mod that changes grass), it stops and writes a warning to `LazyCamping.log` rather than removing the wrong grass. If grass flickers, the game stutters or crashes around a camp, set `HideGrassUnderCamp` to false. It doesn't run in Skyrim VR.
- **Footprint check**: the check samples 5 points (the centre and four corners). Something small that sits between those points can still poke through the tent. You can add more sample points in `IsAreaClear` if needed.
- **One camp at a time**: this is by design. Casting the power while a camp is up offers to pack that camp up, wherever it is; cast again to make a new one.
- **Bedroll fan**: neighbouring bedrolls in a tentless camp are 70° apart, about 218 units between centres at the default `BedrollRingRadius` (190). With the vanilla bedroll (128 long, assumed up to 64 wide) that leaves about 90 units between their ends; the limit is the seats in front, which are assumed to need 35 units behind the marker. If seated people have room to spare in game, `BedrollRingRadius` can go down to about 180.
- **Sneak to sit**: `BedrollSeats.cpp` hooks furniture activation. When the player sneaks and activates a camp bedroll, it activates that bedroll's seat instead, so the bedroll stays the plain vanilla one and anyone else (followers) uses it as normal.
- **Cooking**: the vanilla campfire can't be activated, so a **cook box** does it: make it once in the Creation Kit. In `LZC_HoldingCell`, click *Create Trigger* with nothing selected and choose `LZC_CookActivator`; on the box's *Primitive* tab tick **Player Activation** (so the crosshair picks it up) and make it about 60 × 60 × 40; tick *Persistent* and *Initially Disabled*, and fill `LZC_CookBox` on `LZC_CampsiteQuest` with it. While a camp's fire is lit, the box sits over it and shows *Cook*; activating it activates `LZC_CookingStation`, which has the cooking pot's crafting keyword and no furniture marker, so the Cook menu should open without moving the player. Check that in game.
- **Tub trigger** (optional): in `LZC_HoldingCell`, click *Create Trigger* with nothing selected and choose `LZC_CookActivator` again; leave the box on the trigger layer (don't tick Player Activation). Place and size it over the water in the tub clutter set, like any clutter item, so followers only step into it in the tub. Attach `LZC_TubTriggerScript` to the box (the reference itself, not its base, which the cook box shares) and fill its `CampQuest` with `LZC_CampsiteQuest`. Tick *Initially Disabled* and fill `LZC_TubTrigger` on `LZC_CampsiteQuest` with it (that makes it persistent). The camp moves it onto each tub, keeping its offset and turn from the tub anchor; the offset is read the first time, while the trigger is still in the holding cell.
- **Base Object Swapper**: in testing, BOS didn't swap the references the camp places at runtime, so variance mods don't change the camp's bedrolls. The camp's scripts don't depend on the base object, so this only affects looks.
- **Map marker**: moving a persistent map marker is how other camping mods do it. Still, check in game that the marker appears at the camp on the world map, moves with each new camp, and disappears when you pack up.
- **Clutter**: each copy takes its scale from its clutter item in the holding cell, set the way Papyrus's `SetScale` does it, so it shows at that size straight away and the save keeps it. A camp with many cluttered tents can place well over 100 objects; that's fine for the game, but keep sets modest.
- **Follower count**: followers are counted when the camp goes up, up to 3, because the follower quest has 3 follower aliases. With more than 3 follower seats, the extra seats stay empty. Someone who joins or leaves afterwards keeps their old status until the next camp.
- **Expansion ideas**:
  - a "Well Rested" style bonus for sleeping in your own tent (check `OnSleepStop` with a player alias)
  - fire damage when standing in the fire (a hazard)
  - a storage chest (place a container ref and empty it into the player on pack-up)
  - weather-dependent warmth for survival mode
