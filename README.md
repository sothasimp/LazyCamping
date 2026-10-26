# Lazy Camping: a mod for Skyrim Special Edition

The mod adds a **Prepare Camp** Lesser Power. When the player has at least one **Bedroll** in their inventory, casting the power builds a camp in front of the player, always with a campfire. Tent kits are optional. The kits are **reusable**: setting up camp doesn't take them from the inventory. The mod requires **SKSE**, **Address Library for SKSE Plugins**, and its own `LazyCamping.dll`. The camp includes:

- a burning campfire with flickering light. For cooking on it, use a mod that makes campfires usable, such as *Useable Campfires*.
- cross-legged seats around the fire, facing it:
  - one for the player, which the player can sit in. It's across the fire from where they stood to cast the power, in front of their tent's entrance.
  - one for followers per bedroll beyond the player's, up to 5, fanning out from the player's seat: 70° right, 70° left, 140° right, 140° left. A seat 70° out sits in front of a side tent. With all 5 follower seats, the six seats space out evenly every 60° instead, and the last lands on the near side, where the player stood.
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
- **the player's tent** stands in the middle, straight across the fire from the player, entrance facing them, with the player's bedroll in it. It's the large tent if there is one; otherwise it's the small tent with the most bedrolls.
- **the other tents** stand 70° to either side of it: the second to the right, the third to the left. The near side of the fire stays open. Tent fronts sit just behind the seats: small tents stand 235 units from the fire, a lone large tent 250, and a large tent with small ones beside it about 270, just far enough that the tents' front corners don't touch.
- **with no tent kits**, there are no tents. The bedrolls (up to 4) lie sideways in the same fan, just outside the seats: the player's behind their seat, the rest beside it. With one bedroll per person, each lies behind its owner's seat.

**Dressing sets** add clutter around the camp, such as pelts, packs, firewood and lanterns. You lay each set out by eye in the Creation Kit around a stand-in marker, and every fire, tent or tentless bedroll gets a copy, turned to match. One **backpack** can be the player's storage: it's a single real container that moves from camp to camp, so whatever's in it stays there.

While a camp is up, a **map marker** shows where it is, and the player can fast travel to it, unless fast travel is disabled globally (for example in Survival Mode). Packing up removes the marker.

Every bedroll also has a **seat on it**, facing the fire. Followers sit at the fire when the player sits in their fire seat, and on the other bedrolls when the player sits on a bedroll, matched the same way as at bedtime. Once seated, they stay seated after the player gets up, until the player goes to bed, packs up the camp, or leaves it. When the player sleeps at camp, in any of the camp's bedrolls, followers sleep in the bedrolls the player isn't using. A spouse or lover gets the bedroll next to the player's: the one beside them in the same tent, or the neighbouring one in a camp without tents. Two followers who are a couple get two free bedrolls side by side, normally in their own tent.

**Sneaking and activating any of the tents from outside** asks *Pack up your camp?*; on *Yes*, everything the camp placed is removed. Activating a tent without sneaking, or from inside, does nothing. So the player can't pack up by accident while getting into bed.

In a camp without tents, **sneaking and activating any of the camp's bedrolls** asks the same question. Activating a bedroll without sneaking opens the usual Sleep/Wait menu.

**Casting the power again** while a camp is up asks *Pack up your camp?* and packs it up if confirmed, from anywhere, so a camp left behind never stops the player making a new one.

All EditorIDs use the prefix `LZC_`.

---

## 1. Files

```
Scripts/Source/
  LZC_CampsiteQuestScript.psc   main logic: checks, placement, tent menu, pack-up, state
  LZC_PrepareCampEffect.psc     on the magic effect; calls the quest
  LZC_TentActivatorScript.psc   on every tent; sneak-activating it from outside offers to pack up the camp
  LZC_BedrollAliasScript.psc    on the RingBedroll aliases; sneak-activating a bedroll offers to pack up a camp without tents
  LZC_CampFollowersScript.psc   followers' seats and bedrolls; sit/sleep with the player
  LZC_CampPlayerAliasScript.psc on the follower quest's Player alias; notices the player sitting
  LZC_Native.psc                declarations of the SKSE plugin's functions
SKSE/                           C++ plugin (LazyCamping.dll), required
  xmake.lua
  src/main.cpp                  plugin entry point, logging
  src/Papyrus.cpp               native functions for LZC_Native
  src/Placement.cpp             ground raycast, slope tilt, water check
```

### How the scripts and the plugin split the work

The Papyrus scripts decide the layout: which tents and bedrolls go up, and where each object stands relative to the player. `LazyCamping.dll` handles the ground and the placing, in two calls:

1. **`CheckCampSite`** checks the ground under the fire and every tent's footprint, and refuses the camp if the ground is too steep, uneven, blocked by rocks or logs, underwater, or missing.
2. **`PlaceCamp`** finds the ground under every object, tilts each one to the slope (up to `MaxSlope` degrees), and creates them all at their final positions in a single frame. It finds the ground for everything before placing anything, so a bedroll's ground check can't hit the tent standing over it.

If the DLL isn't loaded, or is older than the scripts expect, casting the power tells the player that Lazy Camping needs SKSE, and nothing is placed.

Compiled `.pex` files go in `Data/Scripts/`.

---

## 2. Creation Kit setup

Open the Creation Kit with `Skyrim.esm` loaded. Before you compile anything, extract `Data/Scripts.zip` once. Copy the seven `.psc` files into `Data/Source/Scripts/` (the SE CK uses this folder).

### 2a. Inventory kits (Items > MiscItem)

| EditorID | Name | Suggested model | Weight / Value |
|---|---|---|---|
| `LZC_TentKitSmallBrown`, `…Blue`, `…Green`, `…Red`, `…White` | Small Brown Tent, Small Blue Tent, … | a rolled hide or a leather bundle clutter mesh | 8 / 40 |
| `LZC_TentKitLargeBrown`, `…Blue`, `…Green`, `…Red`, `…White` | Large Brown Tent, Large Blue Tent, … | the same, or a bigger bundle | 15 / 80 |
| `LZC_BedrollKit` | Bedroll | the rolled-up bedroll clutter mesh | 4 / 15 |

That's 10 tent kits: two sizes in five colours. They can all share one inventory model, or use the coloured TextureSets from step 2b (on the model's *Alternate Textures*) so each kit looks its colour in the inventory.

To use custom meshes, put them in `Data/Meshes/LazyCamping/` and point the Model field at them.

### 2b. Objects placed in the world

**Use vanilla objects directly where you can.** The campfire, light, bedroll and follower sit marker need nothing added to them, so fill their properties (step 2d) with the vanilla objects themselves rather than duplicates. That way, mods that change those vanilla objects also change the camp's. In particular, **Base Object Swapper (BOS) variance mods**, which swap vanilla objects for varied versions, also vary the camp's campfire and bedrolls. Find the objects by filtering the Object Window with the search term shown.

| Property | Type | Vanilla object (search term) | Notes |
|---|---|---|---|
| `LZC_CampfirePlaced` | Static | `campfire` (a *burning* one) | |
| `LZC_CampfireLight` | Light | `fire` (any flickering fire light) | |
| `LZC_BedrollPlaced` | Furniture | `bedroll` (the ground bedroll furniture) | Its sleep marker makes Sleep and Wait work. |
| `LZC_SitMarkerFollower` | Furniture | `CrossLegged` (a cross-legged sit marker) | Followers use these. Invisible is fine. |
| `LZC_SitMarkerPlayer` | Furniture | **duplicate** the same marker | Needs a **visible model** so the player can activate it (see below). |
| `LZC_BedrollSitMarker` | Furniture | **duplicate** the same marker | Placed on every bedroll, facing the fire. It's **invisible but selectable**: give it an invisible collision box over the middle of the bedroll (see below). The player aims at it to sit and at the rest of the bedroll to sleep. Optional. |

**The player's seat.** The player can only activate furniture they can point the crosshair at, and plain sit markers have no visible model or collision. So after duplicating the cross-legged marker:
1. Set its model to something to sit on, such as a fur pelt, a folded bedroll or a log.
2. Check that the furniture's sit marker still lines up with the model. Adjust it in the furniture's marker settings if needed.
3. Set the Activate Text Override to `Sit`.

The followers' seats can use the same visible model if you want all the seats to match. The player's seat and the bedroll seat are the only placed objects that need new records.

**The bedroll seat.** The game picks what the crosshair is on by its collision, not by what's drawn, so an invisible model can still be selected:
1. In NifSkope, open a small vanilla clutter model that has collision, such as a folded pelt, and save a copy as `Meshes/LazyCamping/BedrollSeat.nif`.
2. Delete its visible geometry (the `BSTriShape` or `NiTriShape` blocks), keeping its collision object (`bhkCollisionObject`).
3. Set `LZC_BedrollSitMarker`'s model to that file, and its Activate Text Override to `Sit`.

Keep the collision box small and centred on the bedroll, so its head and foot stay easy to target for sleeping. Check that the furniture's sit marker still sits at the bedroll's height.

**Tents: 2 sizes × 5 colours = 10 Activators.** Make each tent a new **Activator**:
- Model: a tent mesh. Search `tent` in Statics to find the vanilla `.nif` paths; pick a smaller and a larger one, or use your own meshes.
- Script: attach `LZC_TentActivatorScript` and set its `CampQuest` property to `LZC_CampsiteQuest`.
- Activate Text Override: `Camp`.

| | Brown | Blue | Green | Red | White |
|---|---|---|---|---|---|
| Small | `LZC_TentSmallBrown` | `LZC_TentSmallBlue` | `LZC_TentSmallGreen` | `LZC_TentSmallRed` | `LZC_TentSmallWhite` |
| Large | `LZC_TentLargeBrown` | `LZC_TentLargeBlue` | `LZC_TentLargeGreen` | `LZC_TentLargeRed` | `LZC_TentLargeWhite` |

You don't need a separate mesh for each colour:
1. Make recoloured copies of the tent's canvas or hide texture (`.dds`) in `Data/Textures/LazyCamping/`.
2. Make a **TextureSet** record for each colour that points at its recoloured texture.
3. On each coloured Activator, open the model's *Alternate Textures* and swap the canvas texture for that colour's TextureSet.

Brown can stay as the mesh's original texture.

**Camp map marker (optional).**

Skyrim can't create new map markers during play, so the mod moves one map marker made in the Creation Kit:

1. Create an interior cell to hold it, for example `LZC_HoldingCell`. It just needs to exist; nothing else goes in it.
2. Place a `MapMarker` (Statics) in that cell and edit the reference:
   - **Marker Data** tab: check *Map Marker*, name it `Your Camp`, Type *Camp*, check *Can Travel To*
   - check *Persistent Reference* and *Initially Disabled*
   - give the reference an Editor ID, for example `LZC_CampMapMarkerRef`
3. Set `LZC_CampMapMarker` on the controller quest's script (step 2d) to that reference.

When a camp goes up, the script moves the marker to the campfire, enables it and adds it to the map. When the camp is packed up, it disables the marker again, which takes it off the map.

**Dressing sets (optional).** Clutter is laid out in the holding cell (the same one as the map marker is fine), around a marker for each kind of camp object:

| Set | Marker stands in for | Properties (step 2d) |
|---|---|---|
| Fire | the campfire | `LZC_FireDressingAnchor`, `LZC_FireDressing` |
| Small tent | each small tent | `LZC_SmallTentDressingAnchor`, `LZC_SmallTentDressing` |
| Large tent | each large tent | `LZC_LargeTentDressingAnchor`, `LZC_LargeTentDressing` |
| Ring bedroll | each bedroll in a camp without tents | `LZC_RingBedrollDressingAnchor`, `LZC_RingBedrollDressing` |

For each set:
1. Place an `XMarkerHeading` as the anchor, pointing the way its object faces: a tent's entrance, or a bedroll's head-to-foot direction. It helps to place a real tent or bedroll on it while you work, then delete that before saving.
2. Arrange the clutter around it, at the size and angle you want. Use **static** clutter, not items that can be picked up, or players could take a fresh pelt from every camp. Vanilla objects get Base Object Swapper variants too.
3. Give the anchor and each item an Editor ID if that helps you keep track, and fill the set's two properties: the anchor, and the list of items.

Each copy keeps the item's position, height, turn, lean and scale relative to the anchor. Items sit on the ground at their own spot (raised by their height above the anchor), and tilt with the slope when `AlignToSlope` is on. All the sets together can hold up to 128 items.

**Storage backpack (optional).**
1. Make a container with a backpack model, **Respawns unchecked** and no owner, and place it in the holding cell as a persistent reference.
2. In one or more dressing sets, place a marker where the backpack should go (an `XMarkerHeading`, or a static backpack to see it while you work). Add it to that set's item list, and also to `LZC_CampStorageMarkers`. For example, put one by the entrance of each tent size, and one by the ring bedroll.
3. Set `LZC_CampStorage` to the container.

When a camp goes up, the storage moves to the first storage marker in camp order: the main tent, or the player's bedroll without tents. It returns to the first marker's spot in the holding cell when the camp is packed up. Its contents stay with it, and are only reachable while a camp is up.

### 2c. The power

1. **Magic Effect** `LZC_PrepareCampEffect`
   - Archetype: *Script*, Casting Type: *Fire and Forget*, Delivery: *Self*
   - Check *No Duration*, *No Magnitude*, *No Area*, *No Hit Event*, *Painless*
   - Papyrus Scripts: add `LZC_PrepareCampEffect`, then set the `CampQuest` property to `LZC_CampsiteQuest`
2. **Spell** `LZC_PrepareCampPower`, named "Prepare Camp"
   - Type: *Lesser Power*, Casting: *Fire and Forget*, Delivery: *Self*
   - Effects: `LZC_PrepareCampEffect`, magnitude 0, duration 0
   - Set Equip Type to *Voice*, the slot powers use
3. **Message** `LZC_PackUpConfirmMsg`, shown before packing up, whichever way the player does it
   - Check *Message Box*
   - Text: "Pack up your camp?"
   - Menu Buttons, in this order: `Yes` (0), `No` (1)

### 2d. Controller quest

**Quest** `LZC_CampsiteQuest`
- Check *Start Game Enabled* and *Run Once*
- Scripts tab: add `LZC_CampsiteQuestScript`, then fill these properties:

| Property | Value |
|---|---|
| `PlayerRef` | `PlayerRef` (use auto-fill) |
| `LZC_PrepareCampPower` | the spell |
| `LZC_SmallTentKits`, `LZC_LargeTentKits` | the five tent kits of each size, **in the order brown, blue, green, red, white** |
| `LZC_SmallTents`, `LZC_LargeTents` | the five tents of each size, **in the same order** as the kits. Each kit pitches the tent at the same position in its list. |
| `LZC_BedrollKit` | the bedroll kit |
| `LZC_BedrollPlaced`, `LZC_CampfirePlaced` | the vanilla bedroll and burning campfire (step 2b) |
| `RingBedrollAliases` | the aliases `RingBedroll1`–`4`, in order (below) |
| `LZC_CampfireLight` | optional; leave empty for no light |
| `LZC_SitMarkerPlayer`, `LZC_SitMarkerFollower`, `LZC_BedrollSitMarker` | the seats; leave any empty to skip it |
| `LZC_PackUpConfirmMsg` | the pack-up confirmation from step 2c, used for the power, tents and tentless bedrolls; leave empty to pack up without asking |
| `LZC_CampMapMarker` | the map marker reference from step 2b; leave empty for no marker |
| `LZC_*DressingAnchor`, `LZC_*Dressing` | the dressing sets from step 2b; leave a set's anchor empty for no dressing |
| `LZC_CampStorage`, `LZC_CampStorageMarkers` | the storage container and its markers from step 2b; leave empty for no storage |
| `LocTypeCity`, `LocTypeTown` | vanilla keywords (use auto-fill); blocks camping in settlements |
| `Firewood01`, `FirewoodRequired` | optional; for example `2` to burn 2 firewood per fire |
| `SmallTentCapacity`, `LargeTentCapacity` | bedrolls per tent (default 2 and 4, maximum 4) |
| `SmallTentHalfWidth`, `SmallTentHalfDepth`, `LargeTentHalfWidth`, `LargeTentHalfDepth` | half of each size's footprint; match these to your meshes. They place the tents in the ring, drive the flat-ground check, and decide what counts as inside a tent. |
| `TentRingGap` | all tents share one ring, `SeatRadius + TentRingGap + the largest HalfDepth among them` from the fire (gap default 40, which leaves the tent fronts just behind the seats). With a large tent and side tents, the ring widens automatically, just enough that neighbours 70° apart don't touch. Raise the gap by 10–15 if seated people brush the tents. |
| `BedrollSpacing` | side-to-side gap between bedrolls; check that 4 bedrolls fit in the large tent |
| `SeatRadius`, `LightHeight` | how far the seats are from the fire (default 110), and the light's height. The fire goes `SeatRadius` in front of the player, so they stand at the near edge of the seat ring. |
| `BedrollRingRadius`, `BedrollRingYaw` | without tents: how far the bedrolls are from the fire (default 190, just outside the seats), and the turn that lays them sideways (default 90; use 270 to swap head and foot) |
| `AlignToSlope` | tilt objects to match the ground (default on) |
| `MaxSlope`, `MaxStep` | steepest ground allowed, which is also the most an object will tilt (default 25°), and how uneven a tent's footprint may be (default 48) |
| `LZC_CampFollowersQuest` | the follower quest from step 2e; leave empty to skip follower seats and behaviour |

**Quest Aliases** tab: create `RingBedroll1` to `RingBedroll4`, each with:
- Fill Type: *Specific Reference*, left empty
- Check *Optional*
- Attach `LZC_BedrollAliasScript`

In a camp without tents, the script puts the bedrolls in these aliases, so sneaking and activating one offers to pack up the camp. Using aliases instead of a script on the bedroll means the bedroll can stay the vanilla object.

When the quest starts, it gives the player the power.

### 2e. Followers at camp (optional)

This quest runs while a camp is set up. When the camp goes up, its aliases find the followers who are with the player. When the player sits in their fire seat, followers take the other fire seats; when the player sits on a bedroll, they take the other bedrolls' seats. When the player goes to sleep near camp, the script works out which camp bedroll the player is lying in, if any, and gives the followers the others. Partners choose first, taking the free bedrolls next to the player's. Then each follower couple takes two free bedrolls side by side. Everyone else takes the rest in camp order. The camp records which tent each bedroll is in and its place in the row (or around the ring), so this matching doesn't measure distances between bedrolls.

- A **partner** is a follower whose relationship rank with the player is *Lover* (vanilla spouses, and lovers in most romance mods), or who is in the optional `PartnerFaction`.
- A **couple** is two followers whose relationship rank with each other is *Lover*. A partner is always paired with the player rather than another follower.

The camp also uses these when it shares bedrolls out between the tents, so partners and couples get tents with room for both. A global variable tells the followers what to do:

| `LZC_CampActivity` | When | Followers |
|---|---|---|
| 0 | the camp is up, before anyone has sat down; after the player wakes; after the player leaves camp | follow the player as normal |
| 1 | the player has sat in their fire seat, or on a bedroll's seat | sit in the fire seats, or on the other bedrolls' seats (partners beside the player, couples side by side). They **stay seated after the player gets up**, until the player goes to bed (they go to their bedrolls), packs up the camp, or leaves it: more than `CampRadius` from the fire, or in another worldspace, checked every 5 seconds (they follow). Sitting somewhere else moves them to match. |
| 2 | the player is sleeping within `CampRadius` of the fire | sleep in their bedrolls |

**Global** `LZC_CampActivity`: Short, value 0.

**Quest** `LZC_CampFollowersQuest`
- Leave *Start Game Enabled* **unchecked**; the camp script starts and stops it. Leave *Run Once* unchecked too.
- **Quest Aliases** tab, create:
  - `Player`:
    - Fill Type: *Specific Reference* `PlayerRef`
    - Attach `LZC_CampPlayerAliasScript`
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
  1. `LZC_FollowerSleepN`
     - Template: *Sleep*
     - Location: *Near Reference Alias* `BedrollN`, radius about 64. Keep the radius small so the follower picks that bedroll. If the template also has a bed or target field, point it at `BedrollN` too.
     - Condition: `GetGlobalValue LZC_CampActivity == 2`
  2. `LZC_FollowerSitN`
     - Template: *Sit*
     - Furniture or target: Reference Alias `SeatN`
     - Condition: `GetGlobalValue LZC_CampActivity == 1`

  For both packages, set the schedule to any time, with no duration limit.
- **Scripts** tab: add `LZC_CampFollowersScript`, and fill:
  - `PlayerRef`
  - `LZC_CampActivity`
  - `CampRadius`, default 1024: how near the fire counts as "at camp", for sleeping and for seated followers staying put
  - `PartnerFaction`, optional: for example vanilla `PlayerMarriedFaction`, or a romance mod's lover faction if it doesn't set relationship rank
  - `Followers`, `Bedrolls` and `Seats`: the three aliases of each kind, in order 1–3

Then set `LZC_CampFollowersQuest` on the main quest's script (step 2d).

A follower without a seat or bedroll keeps their normal behaviour, because that package's alias is empty. This also covers having more followers than free bedrolls or seats. Followers are counted when the camp goes up, so someone who joins afterwards is included the next time you make camp.

### 2f. How players get the kits

Pick one or more of these:

- **Crafting**: add a Constructible Object (COBJ) for each kit with Workbench Keyword `CraftingTanningRack`. For example:
  - Small tent kit = 3× `Leather01` + 3× `LeatherStrips` + 2× `Firewood01`
  - Large tent kit = 6× `Leather01` + 5× `LeatherStrips` + 3× `Firewood01`
  - For colours, start from the brown recipe and add a dye ingredient, for example Blue Mountain Flower for blue, Red Mountain Flower for red, Hanging Moss for green and Bone Meal for white. Alternatively, make a colour recipe that converts a brown kit plus the dye into the coloured kit.
  - Bedroll = 2× `Leather01` + 2× `LeatherStrips`
- **Merchants**: add the kits to a general goods leveled list, such as `LItemMiscVendorMiscItems75`.
- **Testing**: in the console, run `help LZC_ 4` to find the form IDs, then `player.additem <id> 1`.

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

1. In the CK, compile all seven scripts (*Gameplay > Papyrus Script Manager*). `LZC_Native` compiles without the DLL; its functions are only bound at runtime.
2. Save the plugin as `LazyCamping.esp`. It adds only new records, so you can flag it as ESL with xEdit.
3. Package these files:
   ```
   LazyCamping.esp
   Scripts/LZC_CampsiteQuestScript.pex
   Scripts/LZC_PrepareCampEffect.pex
   Scripts/LZC_TentActivatorScript.pex
   Scripts/LZC_BedrollAliasScript.pex
   Scripts/LZC_Native.pex
   Scripts/LZC_CampFollowersScript.pex
   Scripts/LZC_CampPlayerAliasScript.pex
   Textures/LazyCamping/...        (the tent recolours)
   Meshes/LazyCamping/BedrollSeat.nif (the invisible bedroll seat)
   SKSE/Plugins/LazyCamping.dll
   Source/Scripts/*.psc            (optional but polite)
   Meshes/LazyCamping/...          (if you made custom meshes)
   ```
   List SKSE and Address Library for SKSE Plugins as requirements on your mod page.

## 5. Known limitations and ideas

- **Testing the tilt**: the slope math matches the game's `EulerAnglesToAxesZXY` rotation, and I checked it numerically. It still needs an in-game check on a hillside. If objects lean the wrong way, set `AlignToSlope` to false while you report or fix it; ground snapping and the site checks don't depend on it.
- **Footprint check**: the check samples 5 points (the centre and four corners). Something small that sits between those points can still poke through the tent. You can add more sample points in `IsAreaClear` if needed.
- **One camp at a time**: this is by design. Casting the power while a camp is up offers to pack that camp up, wherever it is; cast again to make a new one.
- **Bedroll fan**: neighbouring bedrolls in a tentless camp are 70° apart, about 218 units between centres at the default `BedrollRingRadius` (190). With the vanilla bedroll (128 long, assumed up to 64 wide) that leaves about 90 units between their ends; the limit is the seats in front, which are assumed to need 35 units behind the marker. If seated people have room to spare in game, `BedrollRingRadius` can go down to about 180.
- **Sneak to pack up**: in a camp without tents, the bedrolls' normal activation is blocked and `LZC_BedrollAliasScript` passes it on itself. Followers should be unaffected, because anyone else's activation is passed straight through. If followers ever stop using the ring bedrolls, check this first.
- **Cooking**: the camp doesn't place a cooking pot. Mods that make campfires usable, such as *Useable Campfires*, should work at the camp's fire if they apply to the vanilla campfire object it uses. That's untested.
- **Base Object Swapper**: BOS swaps a reference's base object when the reference loads. That should include references the camp places, but test it: with a campfire or bedroll variance mod installed, set up camp a few times and check that the campfire and bedrolls vary. The camp's scripts don't depend on the base object, so a swapped bedroll still sleeps, gets followers, and packs up.
- **Map marker**: moving a persistent map marker is how other camping mods do it. Still, check in game that the marker appears at the camp on the world map, moves with each new camp, and disappears when you pack up.
- **Dressing**: each copy takes its scale from its Creation Kit item, set as the copy is created. Check in game that scaled items appear at the right size. A camp with many dressed tents can place well over 100 objects; that's fine for the game, but keep sets modest.
- **Tent inside check**: "inside" means within the tent's `HalfWidth` × `HalfDepth` footprint. If activating the tent from just outside its entrance does nothing, shrink those values a little.
- **Follower count**: followers are counted when the camp goes up, up to 3, because the follower quest has 3 follower aliases. With more than 3 follower seats, the extra seats stay empty. Someone who joins or leaves afterwards keeps their old status until the next camp.
- **Expansion ideas**:
  - a "Well Rested" style bonus for sleeping in your own tent (check `OnSleepStop` with a player alias)
  - fire damage when standing in the fire (a hazard)
  - chopping firewood as a requirement (already supported through `FirewoodRequired`)
  - a storage chest (place a container ref and empty it into the player on pack-up)
  - weather-dependent warmth for survival mode
