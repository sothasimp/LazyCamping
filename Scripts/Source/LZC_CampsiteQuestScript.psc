Scriptname LZC_CampsiteQuestScript extends Quest
{Central controller for the Set Up Camp power. Tracks the deployed campsite,
handles the camp objects and packs the camp back up. LazyCamping.dll (SKSE)
lays out the camp from this script's properties, checks the ground and places
it in one go (BuildCamp). It also passes on the game events the camp reacts
to: casting the power, activating camp objects, followers in the tub.}

; ---------------------------------------------------------------------------
; Core
; ---------------------------------------------------------------------------
Actor Property PlayerRef Auto
Spell Property LZC_SetUpCampPower Auto
{The Lesser Power. Granted to the player when this quest starts.}

; ---------------------------------------------------------------------------
; Inventory kits: MiscObjects the player carries. They're reusable: setting
; up camp doesn't use them up.
; ---------------------------------------------------------------------------
MiscObject[] Property LZC_SmallTentKits Auto
{Small tent kits by colour: 0 brown, 1 blue, 2 green, 3 red, 4 white, then
any you add (plaid, say). Every tent kit the player carries is pitched (up to
4 tents), in this order. LazyCamping.dll reads the form list
LZC_SmallTentKitList in the plugin instead when it has entries (an array keeps
the value an old save first saw; a form list doesn't): keep the two in step.}
MiscObject[] Property LZC_LargeTentKits Auto
{Large tent kits, same colour order as LZC_SmallTentKits. Read from the form
list LZC_LargeTentKitList instead when it has entries.}
MiscObject Property LZC_BedrollKit Auto
{At least one is needed to make camp; the campfire always comes with it.}

MiscObject Property Firewood01 Auto
{Vanilla firewood: lighting the fire burns FirewoodPerFire when FirewoodMechanic
is on.}
Bool Property FirewoodMechanic = False Auto
{The camp's fire starts out unlit. Activating the firepit with firewood in
the inventory lights it, burning FirewoodPerFire; it burns for FireBurnHours, then goes
out. A chopping block stands by the tents. Off: the fire is always lit.}
Float Property FireBurnHours = 6.0 Auto
{How long a fire lit with firewood burns, in game hours.}
Int Property FirewoodPerFire = 2 Auto
{How many firewood lighting the fire burns (SKSE menu: 1 to 6).}
Furniture Property LZC_FireLightingMarker Auto
{Optional (after Usable Campfires): furniture the player crouches at to strike
stones when lighting the fire with firewood. Without it the fire lights at
once.}
ImpactDataSet Property LZC_FireLightingSparks Auto
{Optional: the sparks of each strike.}
Activator Property LZC_FireLightingSmoke Auto
{Optional: the smoke that rises as the fire catches.}
Bool Property PlaceChoppingBlock = True Auto
{With FirewoodMechanic, place LZC_ChoppingBlock beside the tents.}

; ---------------------------------------------------------------------------
; World objects placed when the camp is set up
; ---------------------------------------------------------------------------
Activator[] Property LZC_SmallTents Auto
{Small tent Activators, same colour order as LZC_SmallTentKits. Read from the
form list LZC_SmallTentList instead when it has entries.}
Activator[] Property LZC_LargeTents Auto
{Large tent Activators, same colour order as LZC_SmallTents. Read from the
form list LZC_LargeTentList instead when it has entries.}
Furniture Property LZC_BedrollPlaced Auto
{Furniture with a sleep marker, so the player can sleep / wait in it. Can be
the vanilla bedroll: this script needs nothing on it.}
Bool Property KitsAtMerchants = True Auto
{General goods merchants can stock the kits (SKSE menu): LazyCamping.dll adds
LZC_LItemCampingKits to every leveled list in LZC_VendorLists while the game
runs.}
Bool Property BedrollsStartRolled = True Auto
{Each bedroll starts rolled up (LZC_BedrollRolled) where it will lie;
activating it unrolls it in its place, and followers unroll their own at
bedtime (SKSE menu).}
Activator Property LZC_BedrollRolled Auto
{The rolled-up bedroll dropped in place of each bedroll with
BedrollsStartRolled: an Activator with the kit's model.}
Activator[] Property LZC_BedrollRolledColors Auto
{The rolled-up bedrolls for Bedroll Alternative's green, blue and red
bedrolls, in that order: copies of LZC_BedrollRolled with the cloth (shape
index 0) taking LZC_BedrollTextureGreen, Blue or Red.}
Int Property BedrollStyle = 0 Auto
{The camp's bedrolls (SKSE menu): 0 the vanilla bedroll (LZC_BedrollPlaced);
with Bedroll Alternative - BOS Color Variance, 1 green, 2 blue, 3 red, 4
mixed; with Ash Nomad - Bedrolls - BOS Distributed, 5 blue, 6 red, 7 purple,
8 mixed. Mixed takes the colors in turn round the camp, the two of a pair in a
tent matching. Both are looked up at run time, so neither is a master.}
TextureSet Property LZC_BedrollTextureGreen Auto
{The bedroll kit's cloth with BedrollStyle 1 (green) or 4 (mixed).}
TextureSet Property LZC_BedrollTextureBlue Auto
{The bedroll kit's cloth with BedrollStyle 2 (blue).}
TextureSet Property LZC_BedrollTextureRed Auto
{The bedroll kit's cloth with BedrollStyle 3 (red).}
Form Property LZC_CampfirePlaced Auto
{Static or Activator: the burning campfire. Can be the vanilla one.}
Furniture Property LZC_CookingStation Auto
{Optional. Furniture with the cooking pot's crafting keyword and an empty
model (no furniture marker), placed hidden at the fire. Activating
LZC_CookBox activates it, which opens the Cook menu without an animation.}
ObjectReference Property LZC_CookBox Auto
{Optional. A persistent, initially disabled activator box in the holding cell:
a box primitive with Player Activation ticked, base LZC_CookActivator
It moves onto the fire while the fire is lit, so looking
at the fire shows "Cook".}
Light Property LZC_CampfireLight Auto
Light Property LZC_CampfireLightDim Auto
{The light for "Dim" (CampfireLightLevel 1): DefaultCandleLight01NS. Without
it, Dim gives the bright light.}
{Optional. Flickering firelight placed above the campfire.}
Bool Property PlaceCampfire = True Auto
{Place the campfire, its clutter, light and cooking (SKSE menu). Without it,
an invisible marker stands in the fire's place, and the firewood mechanic
and chopping block are off.}
Keyword Property LZC_TentAddonSmall Auto
{Marks a tent addon's form list: this keyword first, then the tent kit
(MiscObject), the tent (Activator) and optionally its own clutter anchor and
clutter item references. Any plugin can add one; LazyCamping.dll finds them.}
Keyword Property LZC_TentAddonLarge Auto
{As LZC_TentAddonSmall, for a large tent.}
Bool Property FadeOnSetUp = False Auto
{Fade the screen to black while the camp goes up or is packed up, and back in
once it's done (SKSE menu).}
Bool Property PlaceFireSeats = True Auto
{Place the seats round the fire, the player's and the followers' (SKSE menu).}
Int Property CampfireLightLevel = 2 Auto
{With PlaceCampfireLight, how bright the light is: 1 dim, 2 bright (SKSE
menu, where "Off (Light Placer)" turns PlaceCampfireLight off).}
Bool Property PlaceCampfireLight = True Auto
{Place LZC_CampfireLight. Turn off if a Light Placer config already gives the
campfire its own light.}
Bool Property PlaceMapMarker = True Auto
{Show the camp on the world map with LZC_CampMapMarker while it's up.}
Activator Property LZC_Firepit Auto
{The unlit firepit placed when FirewoodMechanic is on: an Activator. The fire
and its light are placed hidden in its spot.}
Furniture Property LZC_ChoppingBlock Auto
{Optional. A woodcutting block placed beside the outermost tent (or along the
bedroll fan without tents) when FirewoodMechanic is on. Can be the vanilla one.}
Static Property LZC_Tub Auto
{Optional. A bathtub, placed beside the outermost tent on the side away from
the chopping block (or along the bedroll fan without tents) when PlaceTub is
on. Its water, steps and so on are the tub clutter set.}
Bool Property PlaceTub = True Auto
Bool Property TubInLargeTent = False Auto
{When there's a large tent, the tub goes inside it, in place of its third and
fourth bedrolls: the large tent then holds 2 bedrolls at most.}
Bool Property TentCollision = True Auto
{Off: the tents don't collide, so the player and followers walk through their
walls (SKSE menu). LazyCamping.dll applies it whenever a tent's 3D loads.}
Bool Property FireOutsideLargeTent = False Auto
{When the player's tent is a large tent, it stands 285 units further back, so
the fire is outside it instead of under it, and the small tents beside it 265
further back, facing the fire, in an arc around it.}
Activator Property LZC_ClothesBasket Auto
{The clothes basket's base (in the tub clutter set). Activating one takes off
everything the player is wearing; activating it again puts it back on.}
ObjectReference Property LZC_TubWater Auto
{Optional. The water in the tub clutter set (a placeable water activator such
as LargeRainBarrelWater01). Instead of being copied like the other clutter, this
reference itself moves onto the tub, keeping its offset, turn and scale: the
game only treats placeable water as real water (for bathing mods, say) when it
was placed in the Creation Kit.}
ObjectReference Property LZC_TubTrigger Auto
{Optional. A persistent, initially disabled trigger box in the holding cell:
a box primitive on the L_TRIGGER layer, base LZC_CookActivator
It moves onto the tub, matching its turn; followers who
step into it take off their clothes, and put them back on when they step out.}
Furniture Property LZC_SitMarkerPlayer Auto
{Optional. Visible cross-legged seat by the fire that the player can use.}
Furniture[] Property LZC_SeatColors Auto
{Optional. The fire seat in each colour but brown, in the order of the SKSE
menu's seat colours after Brown: white, plaid green, plaid yellow, plaid
blue. Copies of LZC_SitMarkerPlayer with the small tents' texture sets as
alternate textures.}
Int Property SeatColor = 0 Auto
{The three player fire seats' colour (SKSE menu): 0 brown (LZC_SitMarkerPlayer
itself), then LZC_SeatColors from 1.}
Furniture Property LZC_BedrollSitMarker Auto
{Optional. Cross-legged seat placed on every bedroll, facing the fire.
Invisible but selectable: its model is a small collision box with no visible
geometry, so the player can aim at it apart from the bedroll.}
Furniture Property LZC_SitMarkerFollower Auto
{Optional. Cross-legged sit marker by the fire, one per follower.}

; ---------------------------------------------------------------------------
; Clutter sets: clutter laid out in the Creation Kit (in a holding cell)
; around a marker that stands in for a camp object. Each fire, tent and
; tentless bedroll gets a copy of its set, turned to match. Leave a set's
; anchor empty for no clutter.
; ---------------------------------------------------------------------------
ObjectReference Property LZC_FireClutterAnchor Auto
ObjectReference[] Property LZC_FireClutter Auto
ObjectReference Property LZC_FireClutterAnchor2 Auto
ObjectReference[] Property LZC_FireClutter2 Auto
{Optional. A second fire clutter set, laid out like the first around its own
anchor; each camp takes one of the two at random.}
ObjectReference Property LZC_SmallTentClutterAnchor Auto
{Faces the way the tent's entrance does.}
ObjectReference[] Property LZC_SmallTentClutter Auto
ObjectReference Property LZC_LargeTentClutterAnchor Auto
{Faces the way the tent's entrance does.}
ObjectReference[] Property LZC_LargeTentClutter Auto
ObjectReference Property LZC_RingBedrollClutterAnchor Auto
{For camps without tents; faces the way the bedroll does.}
ObjectReference[] Property LZC_RingBedrollClutter Auto
ObjectReference Property LZC_TubClutterAnchor Auto
{Faces the way the tub does. Its set holds the water in it, steps, the
clothes basket and seats inside.}
ObjectReference[] Property LZC_TubClutter Auto

ObjectReference Property LZC_CampStorage Auto
{Optional. A persistent, non-respawning, unowned container kept in the
holding cell. It's moved to the camp while one is up, so its contents carry
over from camp to camp.}
ObjectReference[] Property LZC_CampStorageMarkers Auto
{Items in the clutter sets that mark where the storage goes (they're also
listed in their set). The first one in camp order is used; the storage
returns to the first marker's spot in the holding cell on pack-up.}

Message Property LZC_PackUpConfirmMsg Auto
{Asks before packing up, whichever way the player does it (casting the
power, or sneak-activating a tent or tentless bedroll). Buttons: 0 "Yes",
1 "No". If empty, the camp packs up without asking.}

ObjectReference Property LZC_CampMapMarker Auto
{Optional. A persistent, initially disabled map marker reference kept in a
holding cell. It moves to the campfire while a camp is up and is disabled
(off the map) when the camp is packed up. The player can always fast travel
to it, unless fast travel is disabled globally.}

; ---------------------------------------------------------------------------
; Tents. The layout itself (distances, spacing, where the tub and chopping
; block go) is worked out in LazyCamping.dll; see SKSE/src/CampLayout.cpp.
; ---------------------------------------------------------------------------
Int Property SmallTentCapacity = 2 Auto
{Most bedrolls a small tent holds (1-4).}
Int Property LargeTentCapacity = 4 Auto
{Most bedrolls a large tent holds (1-4).}

; ---------------------------------------------------------------------------
; Followers
; ---------------------------------------------------------------------------
Quest Property LZC_CampFollowersQuest Auto
{Optional. Seats followers by the fire and puts them to bed with the player.
Leave empty to place only the player's seat and the bedrolls.}

; ---------------------------------------------------------------------------
; Ground
; ---------------------------------------------------------------------------
Sound Property LZC_UnpackSound Auto
{Optional. Played as the camp is set up and packed up, like gear being put
down or gathered.}

Bool Property AlignToSlope = True Auto
{Tilt the tents, bedrolls, seats and fire to match the ground.}
Bool Property HideGrassUnderCamp = True Auto
{Clear the grass in a circle around the camp while it's up, so it doesn't
poke through the tents, bedrolls and clutter. The circle just holds every
object the camp placed.}

; ---------------------------------------------------------------------------
; Restrictions
; ---------------------------------------------------------------------------
Keyword Property LocTypeCity Auto
{Optional. Camping is blocked in locations with this keyword.}
Keyword Property LocTypeTown Auto
{Optional. Camping is blocked in locations with this keyword.}

; ---------------------------------------------------------------------------
; State
; ---------------------------------------------------------------------------
Bool Property CampDeployed = False Auto Hidden
ObjectReference Property CampCookBox Auto Hidden
{This camp's cook box, a copy of LZC_CookBox placed with the camp.}
Form[] extraFirewood  ; other firewood from addons (AddFirewood)
Bool lightingFire = False
ObjectReference strikeSeat
ObjectReference strikeSmoke
Int extraFirewoodCount = 0
Bool Property FireSneakHandled = False Auto Hidden
{Set by an addon that does something with Wayfarer_FireSneakActivated (the
Campfire addon's skill trees): sneak-activating the fire then doesn't cook.}

; Slots in placedRefs.
Int Property kCampfire = 0 AutoReadOnly
Int Property kLight = 1 AutoReadOnly
Int Property kFirstTent = 8 AutoReadOnly           ; 8-10
Int Property kFirepit = 35 AutoReadOnly            ; unlit, with FirewoodMechanic
Int Property kCookingStation = 37 AutoReadOnly     ; hidden, at the fire
Int Property kTub = 38 AutoReadOnly
Int Property kSlotCount = 39 AutoReadOnly

Int Property kNativeVersion = 20 AutoReadOnly         ; LazyCamping.dll needed

ObjectReference[] placedRefs
; With BedrollsStartRolled: each bedroll's rolled-up bedroll (None once
; unrolled), and the slot each of placedRefs belongs to (clutter: the object it
; came with). Read and updated by LazyCamping.dll as bedrolls unroll.
ObjectReference[] Property RolledKits Auto Hidden
Int[] Property PlacedOwners Auto Hidden
Form[] clothesTaken   ; what a clothes basket took off, until it's put back on
Bool undressed = False
Int Property kTubClothes = 16 AutoReadOnly   ; per follower in the tub
Actor[] tubActors     ; followers undressed by the tub trigger
Form[] tubClothes     ; what each took off: kTubClothes per tubActors slot
Float[] tubTriggerOffset   ; the trigger's right, forward, up and turn from the tub anchor
Float[] tubWaterOffset     ; and the water's
Bool busy = False

; ---------------------------------------------------------------------------
; Native functions, provided by LazyCamping.dll (SKSE). If it isn't loaded
; they're unbound and GetVersion() returns 0.
; ---------------------------------------------------------------------------
Int Function GetVersion() Global Native
ObjectReference Function GetCookBox() Global Native
; The text for asKey in the game's language, from Interface/Translations/
; Wayfarer - A Convenient Camping Framework_<LANGUAGE>.txt, or asEnglish; any
; {} in it becomes asArg.
String Function Tr(String asKey, String asEnglish, String asArg = "") Global Native

; Sets up a camp in front of the player in one go: works out which tents and
; bedrolls go up and where everything stands from this script's properties
; (partners and couples are the follower counts; the tent and bedroll kits
; akFollowers carry count with the player's), checks the ground, places it
; all with its clutter, moves the cook box, backpack and map marker to the
; camp, sets up the bedroll seats and hides the grass. Returns the placed
; references in placedRefs' slots, then the clutter; or an empty array if the
; camp couldn't go up, GetBuildStatus saying why (see NotifySiteProblem).
ObjectReference[] Function BuildCamp(Quest campQuest, Int partners, Int couples, Actor[] akFollowers) Global Native
Int Function GetBuildStatus() Global Native

; What the last BuildCamp laid out, for the follower quest: the fire seats
; (the player's first) and their places in the fan, the bedrolls and the seat
; on each, each bedroll's group (its tent) and order in its row, the seats in
; the tub, and the tents' sizes and count.
ObjectReference[] Function GetFireSeats() Global Native
Int[] Function GetFirePlaces() Global Native
ObjectReference[] Function GetCampBeds() Global Native
ObjectReference[] Function GetBedSeats() Global Native
Int[] Function GetBedGroups() Global Native
Int[] Function GetBedOwners() Global Native
Int[] Function GetBedOrders() Global Native
ObjectReference[] Function GetTubSeats() Global Native
ObjectReference[] Function GetRolledKits() Global Native
Int[] Function GetPlacedOwners() Global Native
; Unrolls the camp bedrolls among akBeds that are still rolled up.
Function UnrollBeds(ObjectReference[] akBeds) Global Native

; Packs up the camp in the same frame: brings the grass back, forgets the
; bedroll seats, deletes akDelete, disables akDisable and moves akStorage to
; akStorageHome.
Function PackUpRefs(ObjectReference[] akDelete, ObjectReference[] akDisable, ObjectReference akStorage, ObjectReference akStorageHome) Global Native
; Renames a form, or gives it its own name back if name is empty; kept in the save.
Function RenameForm(Form akForm, String name) Global Native

Event OnInit()
	PlayerRef.AddSpell(LZC_SetUpCampPower, False)
EndEvent

; Called by LazyCamping.dll when the player casts the power. Casting it while a camp is up asks
; whether to pack the camp up, wherever the player is.
Function HandlePowerCast()
	If CampDeployed
		If !busy
			busy = True
			ConfirmPackUp()
			busy = False
		EndIf
		Return
	EndIf
	; 0 means the DLL's functions aren't bound: no SKSE, no DLL (or Address
	; Library).
	Int nativeVersion = GetVersion()
	If nativeVersion == 0
		Debug.Notification("Wayfarer: LazyCamping.dll isn't loaded (check SKSE and Address Library).")
		Return
	ElseIf nativeVersion < kNativeVersion
		Debug.Notification("Wayfarer: LazyCamping.dll is out of date.")
		Return
	EndIf

	If busy
		Return
	EndIf
	busy = True
	If CanCampHere() && HasAllKits()
		SetUpCamp()
	EndIf
	busy = False
EndFunction

; Called by LazyCamping.dll when the player activates a camp object: the
; unlit firepit, the cook box over the fire and the clothes basket. Works out
; which one it is and reacts; anything else (the tents) does nothing. Only
; casting the power again packs up the camp.
Function OnCampObjectActivated(ObjectReference akRef, ObjectReference akActionRef)
	If akActionRef != PlayerRef || !CampDeployed || !akRef
		Return
	EndIf
	If akRef == LZC_TubTrigger
		Return
	ElseIf akRef == CampCookBox
		; Sneaking: for addons (Wayfarer_FireSneakActivated). Cooks as usual
		; unless one has claimed it.
		If PlayerRef.IsSneaking()
			SendFireSneakEvent()
			If FireSneakHandled
				Return
			EndIf
		EndIf
		Cook()
	ElseIf akRef == placedRefs[kFirepit]
		LightFire()
	ElseIf LZC_ClothesBasket && akRef.GetBaseObject() == LZC_ClothesBasket
		ChangeClothes()
	EndIf
EndFunction

Bool Function CanCampHere()
	If PlayerRef.IsInInterior()
		Debug.Notification(Tr("$LZC_MsgIndoors", "You can't set up camp indoors."))
		Return False
	EndIf
	If PlayerRef.IsInCombat()
		Debug.Notification(Tr("$LZC_MsgCombat", "You can't set up camp during combat."))
		Return False
	EndIf
	If PlayerRef.IsOnMount()
		Debug.Notification(Tr("$LZC_MsgMounted", "You must dismount to set up camp."))
		Return False
	EndIf
	If PlayerRef.GetSitState() != 0
		Debug.Notification(Tr("$LZC_MsgSeated", "You can't set up camp while seated."))
		Return False
	EndIf

	Location loc = PlayerRef.GetCurrentLocation()
	If loc && ((LocTypeCity && loc.HasKeyword(LocTypeCity)) || (LocTypeTown && loc.HasKeyword(LocTypeTown)))
		Debug.Notification(Tr("$LZC_MsgSettlement", "You can't set up camp in a settlement."))
		Return False
	EndIf

	Return True
EndFunction

Bool Function HasAllKits()
	String missing = ""
	If PlayerRef.GetItemCount(LZC_BedrollKit) < 1
		missing += LZC_BedrollKit.GetName() + ", "
	EndIf

	If missing != ""
		; Without the last ", ".
		missing = StringUtil.Substring(missing, 0, StringUtil.GetLength(missing) - 2)
		Debug.Notification(Tr("$LZC_MsgNeedKits", "You need {} to set up camp.", missing))
		Return False
	EndIf
	Return True
EndFunction

Function SetUpCamp()
	If FadeOnSetUp
		FadeToBlack()
	EndIf

	; Start the follower quest first: its aliases find the followers who are
	; with the player. Partners and couples decide how the bedrolls are shared
	; out between the tents.
	LZC_CampFollowersScript followers = None
	If LZC_CampFollowersQuest
		If LZC_CampFollowersQuest.IsRunning()
			LZC_CampFollowersQuest.Stop()
		EndIf
		If LZC_CampFollowersQuest.Start()
			followers = LZC_CampFollowersQuest as LZC_CampFollowersScript
		EndIf
	EndIf
	Int partners = 0
	Int couples = 0
	Actor[] campers = None
	If followers
		partners = followers.PartnerCount()
		couples = followers.CoupleCount()
		; The tent and bedroll kits they carry count along with the player's.
		campers = followers.CampFollowers()
	EndIf

	; Where the tub's trigger box and water stand around the tub anchor in the
	; holding cell, worked out once, before they first leave it.
	If LZC_TubTrigger && !tubTriggerOffset
		tubTriggerOffset = OffsetFromTubAnchor(LZC_TubTrigger)
	EndIf
	If LZC_TubWater && !tubWaterOffset
		tubWaterOffset = OffsetFromTubAnchor(LZC_TubWater)
	EndIf
	; Disabled before they move and enabled at the tub, so they load afresh
	; there: the game only treats placeable water as water when its
	; reference loads. (Packing up leaves them disabled.)
	If LZC_TubTrigger && LZC_TubTrigger.IsEnabled()
		LZC_TubTrigger.Disable()
	EndIf
	If LZC_TubWater && LZC_TubWater.IsEnabled()
		LZC_TubWater.Disable()
	EndIf

	; LazyCamping.dll works out the whole layout from this script's
	; properties, checks the ground, places everything in one frame and moves
	; the cook box, backpack, map marker and the tub's trigger box and water
	; to the camp.
	placedRefs = BuildCamp(Self, partners, couples, campers)
	Int status = GetBuildStatus()
	If FadeOnSetUp
		FadeBackIn()
	EndIf
	If status != 0 || placedRefs.Length <= kSlotCount
		NotifySiteProblem(status)
		placedRefs = new ObjectReference[39]
		If followers
			LZC_CampFollowersQuest.Stop()
		EndIf
		Return
	EndIf
	RolledKits = GetRolledKits()
	CampCookBox = GetCookBox()
	PlacedOwners = GetPlacedOwners()
	; The cloth sound plays as the camp appears.
	If LZC_UnpackSound
		LZC_UnpackSound.Play(PlayerRef)
	EndIf

	If followers
		; Every camp bedroll goes to the follower quest, which hands them out
		; when the player sleeps; the fire seats, the player's first, with their
		; places in the fan; where each bedroll is, for partners and couples.
		followers.Assign(placedRefs[kCampfire], GetFireSeats(), GetFirePlaces(), GetCampBeds(), GetBedSeats(), GetBedGroups(), GetBedOrders(), GetBedOwners())
		followers.SetTubSeats(GetTubSeats())
	EndIf

	CampDeployed = True
	; The leading space sorts the power to the top of the magic menu.
	RenameForm(LZC_SetUpCampPower, Tr("$LZC_PackUpCampPower", " Pack Up Camp"))
	SendCampEvent("Wayfarer_CampSetUp")
EndFunction

; The cook box over the fire opens the Cook menu, through the hidden cooking
; station at the fire.
Function Cook()
	ObjectReference station = placedRefs[kCookingStation]
	If station
		station.Activate(PlayerRef)
	EndIf
EndFunction

; Activating the camp's unlit firepit lights the fire, burning
; FirewoodPerFire firewood, for FireBurnHours.
Function LightFire()
	; Vanilla firewood, then any other wood an addon has added, one for one.
	Int have = 0
	If Firewood01
		have = PlayerRef.GetItemCount(Firewood01)
	EndIf
	Int i = 0
	While i < extraFirewoodCount
		have += PlayerRef.GetItemCount(extraFirewood[i])
		i += 1
	EndWhile
	If have < FirewoodPerFire
		Debug.Notification(Tr("$LZC_MsgNeedFirewood", "You need {} firewood to light the fire.", FirewoodPerFire as String))
		Return
	EndIf
	; Striking the stones first, if the lighting records are set.
	If lightingFire
		Return
	EndIf
	If LZC_FireLightingMarker
		lightingFire = True
		Bool struck = StrikeStones(placedRefs[kFirepit])
		lightingFire = False
		If !struck
			Return
		EndIf
	EndIf
	Int left = FirewoodPerFire
	If Firewood01
		left -= TakeWood(Firewood01, left)
	EndIf
	i = 0
	While i < extraFirewoodCount && left > 0
		left -= TakeWood(extraFirewood[i], left)
		i += 1
	EndWhile
	SetFireLit(True)
	RegisterForSingleUpdateGameTime(FireBurnHours)
	FinishStriking()
EndFunction

; Lighting the fire by striking stones, after Usable Campfires (by its kind
; permission): the player crouches at the firepit (LZC_FireLightingMarker), a
; spray of sparks comes each second for three seconds (LZC_FireLightingSparks),
; smoke rises (LZC_FireLightingSmoke), and after four seconds the fire lights.
; Combat or the player not getting into position stops it, and nothing is
; used up. Returns whether the fire should light; FinishStriking() then lets
; the player up.
Bool Function StrikeStones(ObjectReference akFire)
	If !akFire
		Return True
	EndIf
	strikeSeat = akFire.PlaceAtMe(LZC_FireLightingMarker)
	Utility.Wait(0.2)
	strikeSeat.Activate(PlayerRef)
	Int tries = 0
	While !strikeSeat.IsFurnitureInUse() && tries < 10
		tries += 1
		Utility.Wait(0.2)
	EndWhile
	If !strikeSeat.IsFurnitureInUse()
		FinishStriking()
		Return False
	EndIf
	Game.DisablePlayerControls(True, True, True, False, True, False, False, False)
	ObjectReference sparks = akFire.PlaceAtMe(Game.GetFormFromFile(0x3B, "Skyrim.esm"))  ; XMarker
	Int second = 1
	While second <= 4
		Utility.Wait(1.0)
		If PlayerRef.GetCombatState() != 0
			If sparks
				sparks.Delete()
			EndIf
			FinishStriking()
			Return False
		EndIf
		If second <= 3 && sparks && LZC_FireLightingSparks
			sparks.PlayImpactEffect(LZC_FireLightingSparks)
		EndIf
		If second == 2 && LZC_FireLightingSmoke
			strikeSmoke = akFire.PlaceAtMe(LZC_FireLightingSmoke)
			strikeSmoke.SetAngle(akFire.GetAngleX(), akFire.GetAngleY(), akFire.GetAngleZ())
		EndIf
		second += 1
	EndWhile
	If sparks
		sparks.Delete()
	EndIf
	Return True
EndFunction

; Lets the player up from striking the stones and clears the smoke once it
; has had time to drift off.
Function FinishStriking()
	If !strikeSeat
		Return
	EndIf
	Game.EnablePlayerControls()
	ObjectReference seat = strikeSeat
	ObjectReference smoke = strikeSmoke
	strikeSeat = None
	strikeSmoke = None
	seat.Activate(PlayerRef)
	Utility.Wait(3.5)
	seat.Disable()
	seat.Delete()
	If smoke
		smoke.Disable(True)
		smoke.Delete()
	EndIf
EndFunction

; A clothes basket takes off whatever is worn in every armour slot (30-61),
; each piece once even if it covers several slots; the next activation puts
; it back on, if the player still has it.
; Removes up to aiMax of akWood from the player; returns how many.
Int Function TakeWood(Form akWood, Int aiMax)
	Int n = PlayerRef.GetItemCount(akWood)
	If n > aiMax
		n = aiMax
	EndIf
	If n > 0
		PlayerRef.RemoveItem(akWood, n)
	EndIf
	Return n
EndFunction

; For addons: akWood also counts as firewood for lighting the camp fire (one
; for one with vanilla firewood). Call on every game load; adding the same
; wood twice does nothing.
Function AddFirewood(Form akWood)
	If !akWood || akWood == Firewood01
		Return
	EndIf
	If !extraFirewood
		extraFirewood = new Form[32]
	EndIf
	If extraFirewood.Find(akWood) >= 0 || extraFirewoodCount >= extraFirewood.Length
		Return
	EndIf
	extraFirewood[extraFirewoodCount] = akWood
	extraFirewoodCount += 1
EndFunction

Function ChangeClothes()
	Int i = 0
	If undressed
		While i < clothesTaken.Length
			If clothesTaken[i] && PlayerRef.GetItemCount(clothesTaken[i]) > 0
				PlayerRef.EquipItem(clothesTaken[i], False, True)
			EndIf
			i += 1
		EndWhile
		undressed = False
		Return
	EndIf
	clothesTaken = WornItems(PlayerRef)
	While i < clothesTaken.Length && clothesTaken[i]
		PlayerRef.UnequipItem(clothesTaken[i], False, True)
		i += 1
	EndWhile
	undressed = True
EndFunction

; What akActor wears in the armour slots (30-61), each piece once even if it
; covers several slots, from the start of the array.
Form[] Function WornItems(Actor akActor)
	Form[] items = new Form[32]
	Int count = 0
	Int i = 0
	While i < 32
		Form item = akActor.GetWornForm(Math.LeftShift(1, i))
		If item && items.Find(item) < 0
			items[count] = item
			count += 1
		EndIf
		i += 1
	EndWhile
	Return items
EndFunction

; akRef's right, forward, up and turn from the tub anchor. Read while akRef
; is still in the holding cell (the first camp); zero if it isn't.
Float[] Function OffsetFromTubAnchor(ObjectReference akRef)
	Float[] offset = new Float[4]
	ObjectReference anchor = LZC_TubClutterAnchor
	If anchor && akRef.GetParentCell() == anchor.GetParentCell()
		Float h = anchor.GetAngleZ()
		Float dx = akRef.GetPositionX() - anchor.GetPositionX()
		Float dy = akRef.GetPositionY() - anchor.GetPositionY()
		offset[0] = dx * Math.Cos(h) - dy * Math.Sin(h)
		offset[1] = dx * Math.Sin(h) + dy * Math.Cos(h)
		offset[2] = akRef.GetPositionZ() - anchor.GetPositionZ()
		offset[3] = akRef.GetAngleZ() - h
	EndIf
	Return offset
EndFunction

; From LazyCamping.dll: a follower stepped into (or out of) the tub
; trigger. Followers who step in are undressed (the player has the clothes
; basket) and dress again when they step out.
Function OnCampTrigger(ObjectReference akRef, ObjectReference akActionRef, Bool entered)
	If !akRef || akRef != LZC_TubTrigger
		Return
	EndIf
	Actor follower = akActionRef as Actor
	If !follower || follower == PlayerRef || !follower.IsPlayerTeammate()
		Return
	EndIf
	If !tubActors
		tubActors = new Actor[8]
		tubClothes = new Form[128]
	EndIf
	If !entered
		RedressFromTub(tubActors.Find(follower))
		Return
	EndIf
	If !CampDeployed || tubActors.Find(follower) >= 0
		Return
	EndIf
	Int slot = tubActors.Find(None)
	If slot < 0
		Return
	EndIf
	tubActors[slot] = follower
	Form[] worn = WornItems(follower)
	Int i = 0
	While i < kTubClothes && worn[i]
		tubClothes[slot * kTubClothes + i] = worn[i]
		; Kept off until put back on, so the follower doesn't dress again.
		follower.UnequipItem(worn[i], True, True)
		i += 1
	EndWhile
EndFunction

; Puts back on what the tub trigger took off the follower in `slot`.
Function RedressFromTub(Int slot)
	If slot < 0 || !tubActors[slot]
		Return
	EndIf
	Actor follower = tubActors[slot]
	Int i = 0
	While i < kTubClothes
		Form item = tubClothes[slot * kTubClothes + i]
		If item && follower.GetItemCount(item) > 0
			follower.EquipItem(item, False, True)
		EndIf
		tubClothes[slot * kTubClothes + i] = None
		i += 1
	EndWhile
	tubActors[slot] = None
EndFunction

Function RedressAllFromTub()
	Int slot = 0
	While tubActors && slot < tubActors.Length
		RedressFromTub(slot)
		slot += 1
	EndWhile
EndFunction

; The fire lit with firewood burns out.
Event OnUpdateGameTime()
	If CampDeployed && placedRefs[kFirepit]
		SetFireLit(False)
	EndIf
EndEvent

; Shows the burning fire and its light, or the unlit firepit in their place.
Function SetFireLit(Bool lit)
	ObjectReference fire = placedRefs[kCampfire]
	ObjectReference fireLight = placedRefs[kLight]
	ObjectReference firepit = placedRefs[kFirepit]
	If lit
		fire.Enable()
		If fireLight
			fireLight.Enable()
		EndIf
		firepit.Disable()
		If CampCookBox
			CampCookBox.Enable()
		EndIf
	Else
		firepit.Enable()
		fire.Disable()
		If fireLight
			fireLight.Disable()
		EndIf
		If CampCookBox
			CampCookBox.Disable()
		EndIf
	EndIf
EndFunction

; Asks "Pack up your camp?" and packs up if the player says yes.
Function ConfirmPackUp()
	If PlayerRef.GetSitState() != 0
		Debug.Notification(Tr("$LZC_MsgGetUpFirst", "You must get up before packing up camp."))
		Return
	EndIf
	If !LZC_PackUpConfirmMsg || LZC_PackUpConfirmMsg.Show() == 0
		PackUpCamp()
	EndIf
EndFunction

; FadeOnSetUp: fades to black and holds it, with Skyrim's own fade image
; space modifiers (FadeToBlackImod, then FadeToBlackHoldImod), as vanilla
; scenes do; FadeBackIn ends it with FadeToBlackBackImod. If they can't be
; found, falls back to FadeOutGame (a fade "in" that only starts after a long
; wait holds the black).
ImageSpaceModifier Function SkyrimImod(Int formId)
	Return Game.GetFormFromFile(formId, "Skyrim.esm") as ImageSpaceModifier
EndFunction

Function FadeToBlack()
	ImageSpaceModifier fade = SkyrimImod(0x000F756D)  ; FadeToBlackImod
	ImageSpaceModifier hold = SkyrimImod(0x000F756E)  ; FadeToBlackHoldImod
	If fade && hold
		fade.Apply()
		Utility.Wait(2.0)
		fade.PopTo(hold)
	Else
		Game.FadeOutGame(True, True, 0.0, 0.5)
		Utility.Wait(0.5)
		Game.FadeOutGame(False, True, 60.0, 0.5)
	EndIf
EndFunction

; Holds the black for 3 seconds while the camp loads in, then fades back in.
Function FadeBackIn()
	Utility.Wait(3.0)
	ImageSpaceModifier hold = SkyrimImod(0x000F756E)  ; FadeToBlackHoldImod
	ImageSpaceModifier back = SkyrimImod(0x000F756F)  ; FadeToBlackBackImod
	If hold && back
		hold.PopTo(back)
	Else
		Game.FadeOutGame(False, True, 0.0, 0.5)
	EndIf
EndFunction

Function PackUpCamp()
	; Sent first, while the camp and its followers are still there.
	SendCampEvent("Wayfarer_CampPackedUp")
	If FadeOnSetUp
		FadeToBlack()
	EndIf
	If LZC_UnpackSound
		LZC_UnpackSound.Play(PlayerRef)
	EndIf
	UnregisterForUpdateGameTime()
	If LZC_TubTrigger
		LZC_TubTrigger.Disable()
	EndIf
	RedressAllFromTub()

	If LZC_CampFollowersQuest && LZC_CampFollowersQuest.IsRunning()
		(LZC_CampFollowersQuest as LZC_CampFollowersScript).Shutdown()
	EndIf

	; Everything visible goes at once, in the same frame.
	ObjectReference[] persistent = new ObjectReference[3]
	persistent[0] = LZC_CookBox
	persistent[1] = LZC_TubWater
	persistent[2] = LZC_CampMapMarker
	ObjectReference storageHome = None
	If LZC_CampStorageMarkers && LZC_CampStorageMarkers.Length > 0
		storageHome = LZC_CampStorageMarkers[0]
	EndIf
	PackUpRefs(placedRefs, persistent, LZC_CampStorage, storageHome)
	If FadeOnSetUp
		FadeBackIn()
	EndIf
	RolledKits = None
	PlacedOwners = None
	CampCookBox = None
	Int i = 0
	While i < placedRefs.Length
		placedRefs[i] = None
		i += 1
	EndWhile

	CampDeployed = False
	RenameForm(LZC_SetUpCampPower, "")
EndFunction

; --- API for other mods ---------------------------------------------------
;
; Mod events, no master needed. Register in any script with
;   RegisterForModEvent("Wayfarer_CampSetUp", "OnWayfarerCampSetUp")
;   RegisterForModEvent("Wayfarer_CampPackedUp", "OnWayfarerCampPackedUp")
; and handle them with
;   Event OnWayfarerCampSetUp(Form akCampCentre, Int aiFollowers)
;   Event OnWayfarerCampPackedUp(Form akCampCentre, Int aiFollowers)
; Also Wayfarer_FireSneakActivated (Form akCampCentre), when the player
; sneak-activates the fire; set FireSneakHandled to stop it cooking too.
; akCampCentre is the camp's centre: the campfire, or an invisible marker in
; its place when the firepit is turned off (cast it to ObjectReference).
; aiFollowers is how many followers are at the camp. Re-register on every game
; load (OnPlayerLoadGame), as with any mod event.
;
; Or ask at any time (with this script's source to compile against):
;   LZC_CampsiteQuestScript.IsCampUp()
;   LZC_CampsiteQuestScript.GetCampCentre()

Function SendFireSneakEvent()
	Int handle = ModEvent.Create("Wayfarer_FireSneakActivated")
	If handle
		ModEvent.PushForm(handle, placedRefs[kCampfire])
		ModEvent.Send(handle)
	EndIf
EndFunction

Function SendCampEvent(String eventName)
	Int followerCount = 0
	If LZC_CampFollowersQuest && LZC_CampFollowersQuest.IsRunning()
		Actor[] campers = (LZC_CampFollowersQuest as LZC_CampFollowersScript).CampFollowers()
		Int i = 0
		While campers && i < campers.Length
			If campers[i]
				followerCount += 1
			EndIf
			i += 1
		EndWhile
	EndIf
	Int handle = ModEvent.Create(eventName)
	If handle
		ModEvent.PushForm(handle, placedRefs[kCampfire])
		ModEvent.PushInt(handle, followerCount)
		ModEvent.Send(handle)
	EndIf
EndFunction

LZC_CampsiteQuestScript Function Camp() Global
	Return Game.GetFormFromFile(0x800, "Wayfarer - A Convenient Camping Framework.esp") as LZC_CampsiteQuestScript
EndFunction

; Whether the player's camp is up.
Bool Function IsCampUp() Global
	LZC_CampsiteQuestScript campScript = Camp()
	Return campScript && campScript.CampDeployed
EndFunction

; The camp's centre (see above), or None when there's no camp.
ObjectReference Function GetCampCentre() Global
	LZC_CampsiteQuestScript campScript = Camp()
	If !campScript || !campScript.CampDeployed
		Return None
	EndIf
	Return campScript.CampCentre()
EndFunction

ObjectReference Function CampCentre()
	Return placedRefs[kCampfire]
EndFunction

Function NotifySiteProblem(Int result)
	If result == 1
		Debug.Notification(Tr("$LZC_MsgNoGround", "There's no solid ground here to camp on."))
	ElseIf result == 2
		Debug.Notification(Tr("$LZC_MsgTooSteep", "The ground here is too steep to camp on."))
	ElseIf result == 3
		Debug.Notification(Tr("$LZC_MsgNoRoom", "There isn't enough flat, open ground here."))
	ElseIf result == 4
		Debug.Notification(Tr("$LZC_MsgWater", "You can't set up camp in the water."))
	Else
		Debug.Notification(Tr("$LZC_MsgFailed", "Your camp couldn't be set up."))
	EndIf
EndFunction
