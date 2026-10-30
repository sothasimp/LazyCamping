Scriptname LZC_CampsiteQuestScript extends Quest
{Central controller for the Set Up Camp power. Tracks the deployed campsite,
handles the camp objects and packs the camp back up. LazyCamping.dll (SKSE)
lays out the camp from this script's properties, checks the ground and places
it in one go (LZC_Native.BuildCamp).}

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
4 tents), in this order.}
MiscObject[] Property LZC_LargeTentKits Auto
{Large tent kits, same colour order as LZC_SmallTentKits.}
MiscObject Property LZC_BedrollKit Auto
{At least one is needed to make camp; the campfire always comes with it.}

MiscObject Property Firewood01 Auto
{Vanilla firewood: lighting the fire burns kFirewoodPerFire when FirewoodMechanic
is on.}
Bool Property FirewoodMechanic = False Auto
{The camp's fire starts out unlit. Activating the firepit with firewood in
the inventory lights it, burning two; it burns for FireBurnHours, then goes
out. A chopping block stands by the tents. Off: the fire is always lit.}
Float Property FireBurnHours = 6.0 Auto
{How long a fire lit with firewood burns, in game hours.}
Bool Property PlaceChoppingBlock = True Auto
{With FirewoodMechanic, place LZC_ChoppingBlock beside the tents.}

; ---------------------------------------------------------------------------
; World objects placed when the camp is set up
; ---------------------------------------------------------------------------
Activator[] Property LZC_SmallTents Auto
{Small tent Activators, same colour order as LZC_SmallTentKits. Each needs
LZC_CampObjectScript.}
Activator[] Property LZC_LargeTents Auto
{Large tent Activators, same colour order as LZC_SmallTents.}
Furniture Property LZC_BedrollPlaced Auto
{Furniture with a sleep marker, so the player can sleep / wait in it. Can be
the vanilla bedroll: this script needs nothing on it.}
Bool Property KitsAtMerchants = True Auto
{General goods merchants can stock the kits (SKSE menu): LazyCamping.dll adds
LZC_LItemCampingKits to every leveled list in LZC_VendorLists while the game
runs.}
Int Property BedrollStyle = 0 Auto
{With Bedroll Alternative - BOS Color Variance installed (SKSE menu): 0 the
vanilla bedroll (LZC_BedrollPlaced), 1 green, 2 blue, 3 red, 4 mixed: green,
blue and red in turn round the camp, the two of a pair in a tent matching.
Looked up at run time, so it isn't a master.}
Form Property LZC_CampfirePlaced Auto
{Static or Activator: the burning campfire. Can be the vanilla one.}
Furniture Property LZC_CookingStation Auto
{Optional. Furniture with the cooking pot's crafting keyword and an empty
model (no furniture marker), placed hidden at the fire. Activating
LZC_CookBox activates it, which opens the Cook menu without an animation.}
ObjectReference Property LZC_CookBox Auto
{Optional. A persistent, initially disabled activator box in the holding cell:
a box primitive with Player Activation ticked, base LZC_CookActivator
(LZC_CampObjectScript). It moves onto the fire while the fire is lit, so looking
at the fire shows "Cook".}
Light Property LZC_CampfireLight Auto
{Optional. Flickering firelight placed above the campfire.}
Bool Property PlaceCampfireLight = True Auto
{Place LZC_CampfireLight. Turn off if a Light Placer config already gives the
campfire its own light.}
Activator Property LZC_Firepit Auto
{The unlit firepit placed when FirewoodMechanic is on: an Activator with
LZC_CampObjectScript. The fire and its light are placed hidden in its spot.}
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
(LZC_CampObjectScript). It moves onto the tub, matching its turn; followers who
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
; The footprints here are for telling whether the player is inside a tent.
; ---------------------------------------------------------------------------
Float Property SmallTentHalfWidth = 85.0 AutoReadOnly
Float Property SmallTentHalfDepth = 85.0 AutoReadOnly
Int Property SmallTentCapacity = 2 Auto
{Most bedrolls a small tent holds (1-4).}
Float Property LargeTentHalfWidth = 150.0 AutoReadOnly
Float Property LargeTentHalfDepth = 100.0 AutoReadOnly
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
Float Property MaxSlope = 25.0 Auto
{Refuse to camp where the ground is steeper than this (degrees). Objects
tilt by at most this much.}
Float Property MaxStep = 48.0 Auto
{Refuse to camp if the ground under a tent varies by more than this
(catches rocks, logs and ledges).}
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
Int Property TentCount = 0 Auto Hidden

; Slots in placedRefs.
Int Property kCampfire = 0 AutoReadOnly
Int Property kLight = 1 AutoReadOnly
Int Property kFirstTent = 8 AutoReadOnly           ; 8-10
Int Property kFirepit = 35 AutoReadOnly            ; unlit, with FirewoodMechanic
Int Property kCookingStation = 37 AutoReadOnly     ; hidden, at the fire
Int Property kTub = 38 AutoReadOnly
Int Property kSlotCount = 39 AutoReadOnly

Int Property kLarge = 1 AutoReadOnly
Int Property kFirewoodPerFire = 2 AutoReadOnly
Int Property kNativeVersion = 12 AutoReadOnly         ; LazyCamping.dll needed

ObjectReference[] placedRefs
Form[] clothesTaken   ; what a clothes basket took off, until it's put back on
Bool undressed = False
Int Property kTubClothes = 16 AutoReadOnly   ; per follower in the tub
Actor[] tubActors     ; followers undressed by the tub trigger
Form[] tubClothes     ; what each took off: kTubClothes per tubActors slot
Float[] tubTriggerOffset   ; the trigger's right, forward, up and turn from the tub anchor
Float[] tubWaterOffset     ; and the water's
Int[] tentSizesUsed
Bool busy = False

Event OnInit()
	PlayerRef.AddSpell(LZC_SetUpCampPower, False)
EndEvent

; Called by the power's magic effect. Casting it while a camp is up asks
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
	; Library), or no LZC_Native.pex.
	Int nativeVersion = LZC_Native.GetVersion()
	If nativeVersion == 0
		Debug.Notification("Lazy Camping: LazyCamping.dll isn't loaded (check SKSE, Address Library and LZC_Native.pex).")
		Return
	ElseIf nativeVersion < kNativeVersion
		Debug.Notification("Lazy Camping: LazyCamping.dll is out of date.")
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

; Called by LZC_CampObjectScript, on every camp object the player can use:
; the tents, the unlit firepit, the cook box over the fire and the clothes
; basket. Works out which one it is and reacts.
Function OnCampObjectActivated(ObjectReference akRef, ObjectReference akActionRef)
	If akActionRef != PlayerRef || !CampDeployed || !akRef
		Return
	EndIf
	If akRef == LZC_TubTrigger
		Return
	ElseIf akRef == LZC_CookBox
		Cook()
	ElseIf akRef == placedRefs[kFirepit]
		LightFire()
	ElseIf LZC_ClothesBasket && akRef.GetBaseObject() == LZC_ClothesBasket
		ChangeClothes()
	Else
		OnTentActivated(akRef)
	EndIf
EndFunction

; Sneaking and activating any of the camp's tents from outside offers to pack
; up the camp. Anything else does nothing; in particular the player can't pack
; up by accident from inside a tent.
Function OnTentActivated(ObjectReference akTent)
	If busy || !PlayerRef.IsSneaking()
		Return
	EndIf

	Int t = 0
	While t < TentCount
		If placedRefs[kFirstTent + t] == akTent
			If IsInsideTent(PlayerRef, akTent, tentSizesUsed[t])
				Return
			EndIf
			busy = True
			ConfirmPackUp()
			busy = False
			Return
		EndIf
		t += 1
	EndWhile
EndFunction

Bool Function CanCampHere()
	If PlayerRef.IsInInterior()
		Debug.Notification("You can't set up camp indoors.")
		Return False
	EndIf
	If PlayerRef.IsInCombat()
		Debug.Notification("You can't set up camp during combat.")
		Return False
	EndIf
	If PlayerRef.IsOnMount()
		Debug.Notification("You must dismount to set up camp.")
		Return False
	EndIf
	If PlayerRef.GetSitState() != 0
		Debug.Notification("You can't set up camp while seated.")
		Return False
	EndIf

	Location loc = PlayerRef.GetCurrentLocation()
	If loc && ((LocTypeCity && loc.HasKeyword(LocTypeCity)) || (LocTypeTown && loc.HasKeyword(LocTypeTown)))
		Debug.Notification("You can't set up camp in a settlement.")
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
		Debug.Notification("You need: " + missing + "to set up camp.")
		Return False
	EndIf
	Return True
EndFunction

Function SetUpCamp()
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
	If followers
		partners = followers.PartnerCount()
		couples = followers.CoupleCount()
	EndIf

	; LazyCamping.dll works out the whole layout from this script's
	; properties, checks the ground, places everything in one frame and moves
	; the cook box, backpack and map marker to the camp.
	placedRefs = LZC_Native.BuildCamp(Self, partners, couples)
	Int status = LZC_Native.GetBuildStatus()
	If status != 0 || placedRefs.Length <= kSlotCount
		NotifySiteProblem(status)
		placedRefs = new ObjectReference[39]
		If followers
			LZC_CampFollowersQuest.Stop()
		EndIf
		Return
	EndIf
	; The cloth sound plays as the camp appears.
	If LZC_UnpackSound
		LZC_UnpackSound.Play(PlayerRef)
	EndIf
	PlaceOnTub()

	If followers
		; Every camp bedroll goes to the follower quest, which hands them out
		; when the player sleeps; the fire seats, the player's first, with their
		; places in the fan; where each bedroll is, for partners and couples.
		followers.Assign(placedRefs[kCampfire], LZC_Native.GetFireSeats(), LZC_Native.GetFirePlaces(), LZC_Native.GetCampBeds(), LZC_Native.GetBedSeats(), LZC_Native.GetBedGroups(), LZC_Native.GetBedOrders())
		followers.SetTubSeats(LZC_Native.GetTubSeats())
	EndIf

	tentSizesUsed = LZC_Native.GetTentSizes()
	TentCount = LZC_Native.GetTentCount()
	CampDeployed = True
	; The leading space sorts the power to the top of the magic menu.
	LZC_Native.RenameForm(LZC_SetUpCampPower, " Pack Up Camp")
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
; kFirewoodPerFire firewood, for FireBurnHours.
Function LightFire()
	If !Firewood01 || PlayerRef.GetItemCount(Firewood01) < kFirewoodPerFire
		Debug.Notification("You need 2 firewood to light the fire.")
		Return
	EndIf
	PlayerRef.RemoveItem(Firewood01, kFirewoodPerFire)
	SetFireLit(True)
	RegisterForSingleUpdateGameTime(FireBurnHours)
EndFunction

; A clothes basket takes off whatever is worn in every armour slot (30-61),
; each piece once even if it covers several slots; the next activation puts
; it back on, if the player still has it.
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

; Moves the tub's persistent pieces (the trigger, the water) onto the camp's
; tub, each where it sits relative to the tub anchor in the holding cell.
Function PlaceOnTub()
	If !placedRefs[kTub]
		Return
	EndIf
	If LZC_TubTrigger
		If !tubTriggerOffset
			tubTriggerOffset = OffsetFromTubAnchor(LZC_TubTrigger)
		EndIf
		MoveOntoTub(LZC_TubTrigger, tubTriggerOffset)
	EndIf
	If LZC_TubWater
		If !tubWaterOffset
			tubWaterOffset = OffsetFromTubAnchor(LZC_TubWater)
		EndIf
		MoveOntoTub(LZC_TubWater, tubWaterOffset)
	EndIf
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

; Disabled first and enabled after, so akRef loads afresh at the tub: the
; game only treats placeable water as water when its reference loads.
Function MoveOntoTub(ObjectReference akRef, Float[] offset)
	ObjectReference tub = placedRefs[kTub]
	Float t = tub.GetAngleZ()
	Float wx = offset[0] * Math.Cos(t) + offset[1] * Math.Sin(t)
	Float wy = offset[1] * Math.Cos(t) - offset[0] * Math.Sin(t)
	akRef.Disable()
	akRef.MoveTo(tub, wx, wy, offset[2], False)
	akRef.SetAngle(0.0, 0.0, t + offset[3])
	akRef.Enable()
EndFunction

; From LZC_CampObjectScript: something stepped into (or out of) the tub
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
		If LZC_CookBox
			LZC_CookBox.Enable()
		EndIf
	Else
		firepit.Enable()
		fire.Disable()
		If fireLight
			fireLight.Disable()
		EndIf
		If LZC_CookBox
			LZC_CookBox.Disable()
		EndIf
	EndIf
EndFunction

; Asks "Pack up your camp?" and packs up if the player says yes.
Function ConfirmPackUp()
	If PlayerRef.GetSitState() != 0
		Debug.Notification("You must get up before packing up camp.")
		Return
	EndIf
	If !LZC_PackUpConfirmMsg || LZC_PackUpConfirmMsg.Show() == 0
		PackUpCamp()
	EndIf
EndFunction

Function PackUpCamp()
	If LZC_UnpackSound
		LZC_UnpackSound.Play(PlayerRef)
	EndIf
	UnregisterForUpdateGameTime()
	LZC_Native.ShowGrass()
	If LZC_CookBox
		LZC_CookBox.Disable()
	EndIf
	If LZC_TubTrigger
		LZC_TubTrigger.Disable()
	EndIf
	If LZC_TubWater
		LZC_TubWater.Disable()
	EndIf
	RedressAllFromTub()

	If LZC_CampFollowersQuest && LZC_CampFollowersQuest.IsRunning()
		(LZC_CampFollowersQuest as LZC_CampFollowersScript).Shutdown()
	EndIf
	If LZC_CampMapMarker
		LZC_CampMapMarker.Disable()
	EndIf
	If LZC_CampStorage && LZC_CampStorageMarkers && LZC_CampStorageMarkers[0]
		LZC_CampStorage.MoveTo(LZC_CampStorageMarkers[0])
	EndIf

	LZC_Native.ClearBedrollSeats()

	Int i = 0
	While i < placedRefs.Length
		If placedRefs[i]
			placedRefs[i].Disable()
			placedRefs[i].Delete()
			placedRefs[i] = None
		EndIf
		i += 1
	EndWhile

	TentCount = 0
	CampDeployed = False
	LZC_Native.RenameForm(LZC_SetUpCampPower, "")
EndFunction

Float Function TentHalfWidth(Int size)
	If size == kLarge
		Return LargeTentHalfWidth
	EndIf
	Return SmallTentHalfWidth
EndFunction

Float Function TentHalfDepth(Int size)
	If size == kLarge
		Return LargeTentHalfDepth
	EndIf
	Return SmallTentHalfDepth
EndFunction

; True if akActor stands within the footprint of a tent of the given size.
Bool Function IsInsideTent(ObjectReference akActor, ObjectReference akTent, Int size)
	; Express the actor's position in the tent's own right/forward axes.
	Float a = akTent.GetAngleZ()
	Float dx = akActor.GetPositionX() - akTent.GetPositionX()
	Float dy = akActor.GetPositionY() - akTent.GetPositionY()
	Float localRight = dx * Math.Cos(a) - dy * Math.Sin(a)
	Float localForward = dx * Math.Sin(a) + dy * Math.Cos(a)
	Return Math.Abs(localRight) < TentHalfWidth(size) && Math.Abs(localForward) < TentHalfDepth(size)
EndFunction

Function NotifySiteProblem(Int result)
	If result == 1
		Debug.Notification("There's no solid ground here to camp on.")
	ElseIf result == 2
		Debug.Notification("The ground here is too steep to camp on.")
	ElseIf result == 3
		Debug.Notification("There isn't enough flat, open ground here.")
	ElseIf result == 4
		Debug.Notification("You can't set up camp in the water.")
	Else
		Debug.Notification("Your camp couldn't be set up.")
	EndIf
EndFunction
