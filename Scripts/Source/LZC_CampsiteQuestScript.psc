Scriptname LZC_CampsiteQuestScript extends Quest
{Central controller for the Set Up Camp power. Tracks the deployed campsite,
checks/consumes the camping kits in the player's inventory, lays out the camp
and packs it back up. LazyCamping.dll (SKSE) checks the ground and places the
camp in one go.}

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
{Vanilla firewood: lighting the fire burns one when FirewoodMechanic is on.}
Bool Property FirewoodMechanic = False Auto
{The camp's fire starts out unlit. Sneak-activating the firepit with firewood
in the inventory lights it, burning one; it burns for FireBurnHours, then goes
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
Float Property TubHalfWidth = 110.0 AutoReadOnly
{Half the width of the tub and its clutter (basket, steps...) out from the tent
side, for keeping them clear of the tent beside them. Raise it if they clip.}
Bool Property TubInLargeTent = False Auto
{When there's a large tent, the tub goes inside it, in place of its third and
fourth bedrolls: the large tent then holds 2 bedrolls at most.}
Float Property TubInTentRight = 137.0 AutoReadOnly
{How far right of the large tent's centre (as seen from the fire) the tub
stands when it's in the tent.}
Float Property TubInTentTurn = 20.0 AutoReadOnly
{Turns the tub and its clutter in the large tent, in degrees clockwise seen
from above, on top of the tub anchor's own turn.}
Float Property TubBesideSmallTentForward = 170.0 AutoReadOnly
{How much nearer the fire than the small tent's centre the tub stands when
it's beside a small tent (negative: further back).}
Float Property TubBesideSmallTentSwing = 80.0 AutoReadOnly
{Then how far the tub swings round the fire toward that small tent, measured
along the circle (negative: away from it). It turns with the swing.}
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
; Layout, in game units. ~70 units is roughly one metre. Everything is
; arranged around the fire; tweak to fit your meshes.
; ---------------------------------------------------------------------------
Float Property LightHeight = 40.0 AutoReadOnly
Float Property SeatRadius = 110.0 AutoReadOnly
{Distance of the seats from the centre of the fire. The fire goes this far in
front of the player, so they stand at the near edge of the seat ring; their
own seat is across the fire, in front of their tent.}

Float Property SmallTentDistance = 250.0 AutoReadOnly
{Distance from the centre of the fire to the centre of each small tent.}
Float Property LargeTentDistance = 107.0 AutoReadOnly
{Distance from the centre of the fire to the centre of the large tent, which
always stands in the middle.}
Float Property SmallTentHalfWidth = 85.0 AutoReadOnly
Float Property SmallTentHalfDepth = 85.0 AutoReadOnly
Int Property SmallTentCapacity = 2 Auto
{Most bedrolls a small tent holds (1-4).}
Float Property LargeTentHalfWidth = 150.0 AutoReadOnly
Float Property LargeTentHalfDepth = 100.0 AutoReadOnly
Int Property LargeTentCapacity = 4 Auto
{Most bedrolls a large tent holds (1-4).}
Float Property PairedBedrollSpacing = 60.0 AutoReadOnly
{Bedrolls in a tent lie in pairs that touch, for couples: the side-to-side
distance between the two of a pair. Set it to the bedroll's width.}
Float Property BedrollSpacing = 214.0 AutoReadOnly
{Side-to-side distance from one pair's bedroll to the next pair's, in a tent.}
Float Property LargeTentBedrollBack = 60.0 AutoReadOnly
{Moves the large tent's bedrolls this much further from the fire (negative:
nearer), from the tent's centre.}
Float Property LargeTentBedrollLeft = 0.0 AutoReadOnly
{Moves the large tent's row of bedrolls this much to the left as seen from the
fire (negative: right), from the centre of the tent.}

Float Property BedrollRingRadius = 190.0 AutoReadOnly
{Without tents: distance of the bedrolls from the centre of the fire, just
outside the seats.}
Float Property BedrollRingYaw = 90.0 AutoReadOnly
{Without tents: added to each bedroll's facing so it lies sideways along the
circle, head clockwise. Use 270 to flip head and foot, or adjust if your
bedroll model's long axis isn't its forward axis.}

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
Int Property kPlayerSeat = 2 AutoReadOnly
Int Property kFirstFollowerSeat = 3 AutoReadOnly   ; 3-7
Int Property kFirstTent = 8 AutoReadOnly           ; 8-10
Int Property kFirstBedroll = 11 AutoReadOnly       ; 11-22, the player's first
Int Property kFirstBedrollSeat = 23 AutoReadOnly   ; 23-34, one on each bedroll
Int Property kFirepit = 35 AutoReadOnly            ; unlit, with FirewoodMechanic
Int Property kChoppingBlock = 36 AutoReadOnly
Int Property kCookingStation = 37 AutoReadOnly     ; hidden, at the fire
Int Property kTub = 38 AutoReadOnly
Int Property kSlotCount = 39 AutoReadOnly
Int Property kMaxFollowerSeats = 5 AutoReadOnly    ; fire seats besides the player's

Int Property kMaxFollowers = 3 AutoReadOnly
Int Property kMaxTents = 3 AutoReadOnly
Int Property kMaxBedrolls = 12 AutoReadOnly
Int Property kRingBedrolls = 4 AutoReadOnly        ; without tents
Int Property kSmall = 0 AutoReadOnly
Int Property kLarge = 1 AutoReadOnly
Float Property kFireHalfSize = 50.0 AutoReadOnly     ; for the ground check
Float Property kFanStep = 70.0 AutoReadOnly          ; degrees between neighbours
Float Property kRingStep = 50.0 AutoReadOnly         ; between bedrolls without tents
Float Property kSeatStep = 60.0 AutoReadOnly         ; between neighbouring fire seats
Float Property kChoppingBlockGap = 60.0 AutoReadOnly ; from a tent's side to the block
Float Property kBedrollHalfLength = 100.0 AutoReadOnly ; head to foot, halved
Int Property kNativeVersion = 11 AutoReadOnly         ; LazyCamping.dll needed

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
	Float heading = PlayerRef.GetAngleZ()
	Int bedrollsCarried = PlayerRef.GetItemCount(LZC_BedrollKit)

	; Start the follower quest first: its aliases find the followers who are
	; with the player. Each gets a seat by the fire, and partners and couples
	; decide how the bedrolls are shared out between the tents.
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

	; Tents, up to kMaxTents and never more than bedrolls (each tent needs at
	; least one). At most one large tent: the first large kit carried (in
	; colour order) becomes the player's tent, in the middle. Every small kit
	; carried fills the rest, in colour order.
	MiscObject[] kits = new MiscObject[4]
	Int[] sizes = new Int[4]
	Int[] colors = new Int[4]
	Int tents = 0
	Int color = 0
	While color < KitCount(kLarge) && tents == 0 && bedrollsCarried > 0
		MiscObject large = TentKit(kLarge, color)
		If large && PlayerRef.GetItemCount(large) > 0
			kits[0] = large
			sizes[0] = kLarge
			colors[0] = color
			tents = 1
		EndIf
		color += 1
	EndWhile
	color = 0
	While color < KitCount(kSmall) && tents < kMaxTents && tents < bedrollsCarried
		MiscObject small = TentKit(kSmall, color)
		If small
			Int have = PlayerRef.GetItemCount(small)
			While have > 0 && tents < kMaxTents && tents < bedrollsCarried
				kits[tents] = small
				sizes[tents] = kSmall
				colors[tents] = color
				tents += 1
				have -= 1
			EndWhile
		EndIf
		color += 1
	EndWhile

	; Bedrolls: each tent starts with its minimum (see MinBeds: room for the
	; player's partners in the first tent, a couple in each of the next ones,
	; one otherwise). If the bedrolls don't cover that, fewer tents go up
	; rather than splitting anyone up. The rest are then shared out in turn up
	; to each tent's capacity. Without tents, up to kRingBedrolls ring the fire.
	Int[] bedsInTent = new Int[4]
	Int bedrolls = 0
	Int t = 0
	If tents > 0
		While tents > 1 && BedsNeeded(tents, sizes, partners, couples) > bedrollsCarried
			tents -= 1
		EndWhile
		While t < tents
			bedsInTent[t] = MinBeds(t, sizes[t], partners, couples)
			bedrolls += bedsInTent[t]
			t += 1
		EndWhile
		If bedrolls > bedrollsCarried
			; A single tent and too few bedrolls for everyone's minimum.
			bedsInTent[0] = bedrollsCarried
			bedrolls = bedrollsCarried
		EndIf
		Bool added = True
		While added && bedrolls < bedrollsCarried
			added = False
			t = 0
			While t < tents && bedrolls < bedrollsCarried
				If bedsInTent[t] < TentCapacity(sizes[t])
					bedsInTent[t] = bedsInTent[t] + 1
					bedrolls += 1
					added = True
				EndIf
				t += 1
			EndWhile
		EndWhile

		; Without a large tent, the tent with the most bedrolls becomes the
		; first tent: it stands across the fire from the player, entrance
		; facing them, and holds the player's bedroll. A large tent is always
		; first already.
		Int main = 0
		t = 1
		While t < tents && sizes[0] != kLarge
			If bedsInTent[t] > bedsInTent[main]
				main = t
			EndIf
			t += 1
		EndWhile
		If main != 0
			MiscObject swapKit = kits[0]
			kits[0] = kits[main]
			kits[main] = swapKit
			Int swap = sizes[0]
			sizes[0] = sizes[main]
			sizes[main] = swap
			swap = colors[0]
			colors[0] = colors[main]
			colors[main] = swap
			swap = bedsInTent[0]
			bedsInTent[0] = bedsInTent[main]
			bedsInTent[main] = swap
		EndIf
	Else
		bedrolls = bedrollsCarried
		If bedrolls > kRingBedrolls
			bedrolls = kRingBedrolls
		EndIf
	EndIf

	; Tents stand in an amphitheater on the far side of the fire, entrances
	; facing it and the open side where the player stands: the player's tent
	; straight across, the others kFanStep degrees to either side. Bearings
	; are degrees clockwise from the player's facing, seen from the fire.
	Float[] tentFwd = new Float[4]
	Float[] tentSide = new Float[4]
	Float[] tentTurn = new Float[4]
	Float[] tentDistance = new Float[4]
	t = 0
	While t < tents
		Float bearing = FanBearing(t)
		tentDistance[t] = SmallTentDistance
		If sizes[t] == kLarge
			tentDistance[t] = LargeTentDistance
		EndIf
		tentFwd[t] = FireForward() + tentDistance[t] * Math.Cos(bearing)
		tentSide[t] = tentDistance[t] * Math.Sin(bearing)
		tentTurn[t] = bearing + 180.0
		t += 1
	EndWhile

	; Check the ground under the fire and every tent in one go.
	Float[] siteFwd = new Float[4]
	Float[] siteSide = new Float[4]
	Float[] siteTurn = new Float[4]
	Float[] siteHalfWidth = new Float[4]
	Float[] siteHalfDepth = new Float[4]
	siteFwd[0] = FireForward()
	siteHalfWidth[0] = kFireHalfSize
	siteHalfDepth[0] = kFireHalfSize
	t = 0
	While t < tents
		siteFwd[t + 1] = tentFwd[t]
		siteSide[t + 1] = tentSide[t]
		siteTurn[t + 1] = tentTurn[t]
		siteHalfWidth[t + 1] = TentHalfWidth(sizes[t])
		siteHalfDepth[t + 1] = TentHalfDepth(sizes[t])
		t += 1
	EndWhile
	Int site = LZC_Native.CheckCampSite(PlayerRef, heading, 1 + tents, siteFwd, siteSide, siteTurn, siteHalfWidth, siteHalfDepth, MaxStep, MaxSlope)
	If site != 0
		NotifySiteProblem(site)
		If followers
			LZC_CampFollowersQuest.Stop()
		EndIf
		Return
	EndIf

	; Slot layout (see placedRefs). Index 0, the campfire, is the camp's anchor.
	Form[] forms = new Form[39]
	Float[] fwd = new Float[39]
	Float[] side = new Float[39]
	Float[] lift = new Float[39]
	Float[] turn = new Float[39]
	; Which clutter set (1-based; 0 = none) goes around each layout object.
	Int[] clutterSet = new Int[39]

	forms[kCampfire] = LZC_CampfirePlaced
	clutterSet[kCampfire] = 1
	fwd[kCampfire] = FireForward()
	If PlaceCampfireLight
		forms[kLight] = LZC_CampfireLight
	EndIf
	fwd[kLight] = FireForward()
	lift[kLight] = LightHeight
	forms[kCookingStation] = LZC_CookingStation
	If LZC_Tub && PlaceTub
		SetTub(forms, fwd, side, turn, clutterSet, tents, sizes, tentFwd, tentSide, tentTurn, bedrolls)
	EndIf
	fwd[kCookingStation] = FireForward()

	; With the firewood mechanic, an unlit firepit stands in the fire's spot;
	; the fire and its light are placed too, but hidden until it's lit.
	If FirewoodMechanic && LZC_Firepit
		forms[kFirepit] = LZC_Firepit
		fwd[kFirepit] = FireForward()
		If LZC_ChoppingBlock && PlaceChoppingBlock
			SetChoppingBlock(forms, fwd, side, turn, tents, sizes, tentFwd, tentSide, tentTurn, bedrolls)
		EndIf
	EndIf

	; Where each bedroll is, for matching partners and couples at bedtime: its
	; group (the tent it's in, or 0 without tents) and its place in the row
	; (neighbours differ by 1).

	Int[] bedGroup = new Int[12]
	Int[] bedOrder = new Int[12]
	Float[] ringSeatTurn = new Float[12]   ; without tents: each bedroll seat's facing

	; Seats ring the fire and face it. The player's is at 0, across the fire
	; in front of their tent. There's a follower seat for each bedroll beyond
	; the player's, up to kMaxFollowerSeats, fanning out to either side
	; kSeatStep apart: with all 5, the six seats ring the fire evenly.
	Int followerSeats = 0
	If LZC_SitMarkerFollower
		followerSeats = bedrolls - 1
		If followerSeats > kMaxFollowerSeats
			followerSeats = kMaxFollowerSeats
		EndIf
	EndIf
	Float seatStep = kSeatStep
	SetSeat(forms, fwd, side, turn, kPlayerSeat, LZC_SitMarkerPlayer, 0.0)
	; The two follower seats either side of the player's, in front of the
	; middle tent, are the player's kind too, so the player can sit there, and
	; they're placed even without followers.
	Int fireSideSeats = followerSeats
	If LZC_SitMarkerPlayer && fireSideSeats < 2
		fireSideSeats = 2
	EndIf
	Int f = 0
	While f < fireSideSeats
		Form seat = LZC_SitMarkerFollower
		If f < 2 && LZC_SitMarkerPlayer
			seat = LZC_SitMarkerPlayer
		EndIf
		SetSeat(forms, fwd, side, turn, kFirstFollowerSeat + f, seat, FanPlace(f + 1) * seatStep)
		f += 1
	EndWhile

	If tents > 0
		t = 0
		While t < tents
			forms[kFirstTent + t] = TentForm(sizes[t], colors[t])
			fwd[kFirstTent + t] = tentFwd[t]
			side[kFirstTent + t] = tentSide[t]
			turn[kFirstTent + t] = tentTurn[t]
			; Index and value in locals: the Papyrus compiler can write an Int
			; array element computed this way with its index for the value.
			Int slot = kFirstTent + t
			Int setNumber = 2 + sizes[t]   ; 2 small, 3 large
			clutterSet[slot] = setNumber
			t += 1
		EndWhile

		; Bedroll slots: first each tent's first bedroll (the player's is in the
		; first tent, followers' first beds in the others), then the rest.
		Int nextSlot = kFirstBedroll
		t = 0
		While t < tents
			SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], 0, bedsInTent[t], sizes[t])
			bedGroup[nextSlot - kFirstBedroll] = t
			Int firstOrder = PairedOrder(0)
			bedOrder[nextSlot - kFirstBedroll] = firstOrder
			nextSlot += 1
			t += 1
		EndWhile
		t = 0
		While t < tents
			Int b = 1
			While b < bedsInTent[t]
				SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], b, bedsInTent[t], sizes[t])
				bedGroup[nextSlot - kFirstBedroll] = t
				Int nextOrder = PairedOrder(b)
				bedOrder[nextSlot - kFirstBedroll] = nextOrder
				nextSlot += 1
				b += 1
			EndWhile
			t += 1
		EndWhile
	Else
		; Without tents, the bedrolls lie sideways in the same fan, just
		; outside the seats: the player's behind their seat, the rest beside it.
		; The player and a partner, and a follower couple, get neighbouring
		; places with their bedrolls head to head. The partner goes on the
		; right, or on the left when a couple takes the two places on the right.
		Bool partnerPair = partners > 0 && bedrolls >= 2
		Bool couplePair = couples > 0 && ((partnerPair && bedrolls >= 4) || (!partnerPair && bedrolls >= 3))
		Bool[] used = new Bool[5]   ; fan places -2 to 2, at place + 2
		SetRingBedroll(forms, fwd, side, turn, bedOrder, clutterSet, ringSeatTurn, used, 0, 0, partnerPair && couplePair)
		Int r = 1
		If partnerPair && couplePair
			SetRingBedroll(forms, fwd, side, turn, bedOrder, clutterSet, ringSeatTurn, used, r, -1, False)
			r += 1
		ElseIf partnerPair
			SetRingBedroll(forms, fwd, side, turn, bedOrder, clutterSet, ringSeatTurn, used, r, 1, True)
			r += 1
		EndIf
		If couplePair
			SetRingBedroll(forms, fwd, side, turn, bedOrder, clutterSet, ringSeatTurn, used, r, 1, False)
			SetRingBedroll(forms, fwd, side, turn, bedOrder, clutterSet, ringSeatTurn, used, r + 1, 2, True)
			r += 2
		EndIf
		; Everyone else takes the free places in fan order.
		Int k = 1
		While r < bedrolls && k < 5
			Int place = FanPlace(k)
			If !used[place + 2]
				SetRingBedroll(forms, fwd, side, turn, bedOrder, clutterSet, ringSeatTurn, used, r, place, False)
				r += 1
			EndIf
			k += 1
		EndWhile
	EndIf

	; A seat on each bedroll, facing the fire: in a tent that's the way the
	; bedroll faces; in the ring, back toward the fire.
	If LZC_BedrollSitMarker
		Int bs = 0
		While bs < kMaxBedrolls
			If forms[kFirstBedroll + bs]
				forms[kFirstBedrollSeat + bs] = LZC_BedrollSitMarker
				fwd[kFirstBedrollSeat + bs] = fwd[kFirstBedroll + bs]
				side[kFirstBedrollSeat + bs] = side[kFirstBedroll + bs]
				If tents > 0
					; The way the tent faces, whichever way its bedrolls lie.
					Int seatSlot = kFirstBedrollSeat + bs
					Int tentSlot = kFirstTent + bedGroup[bs]
					Float tentFacing = turn[tentSlot]
					turn[seatSlot] = tentFacing
				Else
					turn[kFirstBedrollSeat + bs] = ringSeatTurn[bs]
				EndIf
			EndIf
			bs += 1
		EndWhile
	EndIf

	; The plugin puts everything on the ground and places it in one frame.
	Int[] setAnchorIDs = new Int[5]
	setAnchorIDs[0] = AnchorID(LZC_FireClutterAnchor)
	setAnchorIDs[1] = AnchorID(LZC_SmallTentClutterAnchor)
	setAnchorIDs[2] = AnchorID(LZC_LargeTentClutterAnchor)
	setAnchorIDs[3] = AnchorID(LZC_RingBedrollClutterAnchor)
	setAnchorIDs[4] = AnchorID(LZC_TubClutterAnchor)
	ObjectReference[] setItems = new ObjectReference[128]
	Int[] itemSet = new Int[128]
	Int itemCount = AddSet(setItems, itemSet, 0, LZC_FireClutter, 1)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_SmallTentClutter, 2)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_LargeTentClutter, 3)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_RingBedrollClutter, 4)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_TubClutter, 5)

	; The cloth sound plays as the camp appears.
	If LZC_UnpackSound
		LZC_UnpackSound.Play(PlayerRef)
	EndIf
	placedRefs = LZC_Native.PlaceCamp(PlayerRef, heading, forms, fwd, side, lift, turn, AlignToSlope, MaxSlope, clutterSet, setAnchorIDs, setItems, itemSet, LZC_CampStorageMarkers)
	If placedRefs.Length <= kSlotCount
		Debug.Notification("Your camp couldn't be set up.")
		placedRefs = new ObjectReference[39]
		If followers
			LZC_CampFollowersQuest.Stop()
		EndIf
		Return
	EndIf

	; The cook box goes over the fire, enabled only while the fire is lit.
	If LZC_CookBox
		LZC_CookBox.MoveTo(placedRefs[kCampfire])
		LZC_CookBox.Enable()
	EndIf
	If placedRefs[kFirepit]
		SetFireLit(False)
	EndIf
	PlaceOnTub()

	; Sneaking and activating a bedroll sits the player on its seat; the DLL
	; handles it, so the bedrolls stay the plain vanilla ones.
	ObjectReference[] campBeds = new ObjectReference[12]
	ObjectReference[] campBedSeats = new ObjectReference[12]
	Int i = 0
	While i < kMaxBedrolls
		campBeds[i] = placedRefs[kFirstBedroll + i]
		campBedSeats[i] = placedRefs[kFirstBedrollSeat + i]
		i += 1
	EndWhile
	LZC_Native.SetBedrollSeats(campBeds, campBedSeats)

	If followers
		; Every camp bedroll goes to the follower quest, which hands them out
		; when the player sleeps, skipping whichever one the player is in.
		ObjectReference[] beds = new ObjectReference[12]
		i = 0
		While i < kMaxBedrolls
			beds[i] = placedRefs[kFirstBedroll + i]
			i += 1
		EndWhile
		; The fire seats, the player's first, with their places in the fan.
		ObjectReference[] fireSeats = new ObjectReference[6]
		Int[] firePlaces = new Int[6]
		fireSeats[0] = placedRefs[kPlayerSeat]
		i = 0
		While i < kMaxFollowerSeats
			fireSeats[i + 1] = placedRefs[kFirstFollowerSeat + i]
			Int firePlace = FanPlace(i + 1)
			firePlaces[i + 1] = firePlace
			i += 1
		EndWhile
		ObjectReference[] bedSeats = new ObjectReference[12]
		i = 0
		While i < kMaxBedrolls
			bedSeats[i] = placedRefs[kFirstBedrollSeat + i]
			i += 1
		EndWhile
		followers.Assign(placedRefs[kCampfire], fireSeats, firePlaces, beds, bedSeats, bedGroup, bedOrder)
	EndIf

	; The player's storage moves onto the spot the plugin marked, which then
	; goes; its contents come along from camp to camp.
	ObjectReference storageSpot = placedRefs[kSlotCount]
	If storageSpot
		If LZC_CampStorage
			LZC_CampStorage.MoveTo(storageSpot)
		EndIf
		storageSpot.Delete()
		placedRefs[kSlotCount] = None
	EndIf

	If HideGrassUnderCamp
		; The storage borrows the spot's slot so its grass goes too.
		placedRefs[kSlotCount] = LZC_CampStorage
		LZC_Native.HideGrass(placedRefs)
		placedRefs[kSlotCount] = None
	EndIf

	If LZC_CampMapMarker
		LZC_CampMapMarker.MoveTo(placedRefs[kCampfire])
		LZC_CampMapMarker.Enable()
		LZC_CampMapMarker.AddToMap(True)
	EndIf

	tentSizesUsed = sizes
	TentCount = tents
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

; Sneak-activating the camp's unlit firepit lights the fire, burning one
; firewood, for FireBurnHours.
Function LightFire()
	If !PlayerRef.IsSneaking()
		Return
	EndIf
	If !Firewood01 || PlayerRef.GetItemCount(Firewood01) < 1
		Debug.Notification("You need firewood to light the fire.")
		Return
	EndIf
	PlayerRef.RemoveItem(Firewood01, 1)
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

; The chopping block stands beside the outermost tent, on its far side from
; the middle one; without tents, in the next place along the bedroll fan.
Function SetChoppingBlock(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int tents, Int[] sizes, Float[] tentFwd, Float[] tentSide, Float[] tentTurn, Int bedrolls)
	forms[kChoppingBlock] = LZC_ChoppingBlock
	If tents > 0
		Int outer = tents - 1
		Float outward = 1.0
		If FanBearing(outer) < 0.0
			outward = -1.0
		EndIf
		; Straight out from the tent's side, kChoppingBlockGap clear of it.
		SetBesideTent(fwd, side, turn, kChoppingBlock, tentFwd[outer], tentSide[outer], tentTurn[outer], outward, TentHalfWidth(sizes[outer]) + kChoppingBlockGap)
		Return
	EndIf
	Float bearing = FanPlace(bedrolls) * kRingStep
	fwd[kChoppingBlock] = FireForward() + BedrollRingRadius * Math.Cos(bearing)
	side[kChoppingBlock] = BedrollRingRadius * Math.Sin(bearing)
	turn[kChoppingBlock] = bearing + 180.0
EndFunction

; Puts slot `slot` `distance` from the centre of the tent at (tentFwd,
; tentSide), straight out from its side and facing the way it does: towards
; higher fan bearings if `outward` is 1, lower if -1.
Function SetBesideTent(Float[] fwd, Float[] side, Float[] turn, Int slot, Float tentFwd, Float tentSide, Float tentTurn, Float outward, Float distance)
	; The tent's right axis, at tentTurn + 90, points to lower bearings.
	fwd[slot] = tentFwd - outward * distance * Math.Cos(tentTurn + 90.0)
	side[slot] = tentSide - outward * distance * Math.Sin(tentTurn + 90.0)
	turn[slot] = tentTurn
EndFunction

; The tub stands beside the outermost tent on the other side from the
; chopping block (left of the middle tent with up to two tents, right of the
; right-hand tent with three), or left of the player's bedroll without
; tents. The tub clutter set (water, steps...) goes
; around it. With TubInLargeTent, it goes in the large tent instead, where
; its third and fourth bedrolls would lie.
Function SetTub(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int[] clutterSet, Int tents, Int[] sizes, Float[] tentFwd, Float[] tentSide, Float[] tentTurn, Int bedrolls)
	forms[kTub] = LZC_Tub
	clutterSet[kTub] = 5
	; The large tent, if any, is always the middle one.
	If TubInLargeTent && tents > 0 && sizes[0] == kLarge
		; TubInTentRight to the right of the tent's centre, as seen from the
		; fire (see SetTentBedroll), as far back as the bedrolls.
		Float offset = LargeTentBedrollLeft - TubInTentRight
		fwd[kTub] = tentFwd[0] + offset * Math.Cos(tentTurn[0] + 90.0) - LargeTentBedrollBack * Math.Cos(tentTurn[0])
		side[kTub] = tentSide[0] + offset * Math.Sin(tentTurn[0] + 90.0) - LargeTentBedrollBack * Math.Sin(tentTurn[0])
		turn[kTub] = tentTurn[0] + TubInTentTurn
		Return
	EndIf
	If tents > 0
		Int beside = 0
		Float outward = -1.0
		If tents >= 3
			beside = 1
			outward = 1.0
		EndIf
		; Straight out from the tent's side, with room for the tub's clutter.
		SetBesideTent(fwd, side, turn, kTub, tentFwd[beside], tentSide[beside], tentTurn[beside], outward, TentHalfWidth(sizes[beside]) + kChoppingBlockGap + TubHalfWidth)
		If sizes[beside] == kSmall
			; Toward the fire, along the way the tent faces.
			fwd[kTub] = fwd[kTub] + TubBesideSmallTentForward * Math.Cos(tentTurn[beside])
			side[kTub] = side[kTub] + TubBesideSmallTentForward * Math.Sin(tentTurn[beside])
			; Then round the fire toward the tent, turning with it.
			Float dx = fwd[kTub] - FireForward()
			Float dy = side[kTub]
			Float radius = Math.Sqrt(dx * dx + dy * dy)
			If radius > 1.0
				Float swing = -outward * TubBesideSmallTentSwing / radius * 57.29578
				Float tubBearing = 90.0
				If dy < 0.0
					tubBearing = -90.0
				EndIf
				If Math.abs(dx) > 0.001
					tubBearing = Math.atan(dy / dx)
					If dx < 0.0
						tubBearing += 180.0
					EndIf
				EndIf
				tubBearing += swing
				fwd[kTub] = FireForward() + radius * Math.Cos(tubBearing)
				side[kTub] = radius * Math.Sin(tubBearing)
				turn[kTub] = turn[kTub] + swing
			EndIf
		EndIf
		Return
	EndIf
	; Without tents: straight out to the left of the player's bedroll (as the
	; player sees it across the fire), facing the fire like the bedroll's seat.
	SetBesideTent(fwd, side, turn, kTub, FireForward() + BedrollRingRadius, 0.0, 180.0, -1.0, kBedrollHalfLength + kChoppingBlockGap + TubHalfWidth)
EndFunction

; How far in front of the player the fire goes: one seat radius, so the
; player stands at the near edge of the seat ring.
Float Function FireForward()
	Return SeatRadius
EndFunction

; Place `index` in the amphitheater fan: 0 in the middle, then alternately
; right and left, further out each pair: 0, 1, -1, 2, -2...
Int Function FanPlace(Int index)
	Int place = (index + 1) / 2
	If index % 2 == 0
		Return -place
	EndIf
	Return place
EndFunction

; Bearing of fan place `index` around the fire, in degrees clockwise from the
; player's facing: 0 straight across the fire, then kFanStep to either side.
Float Function FanBearing(Int index)
	Return FanPlace(index) * kFanStep
EndFunction

; Puts a seat in a slot, SeatRadius from the fire at the given bearing, facing it.
Function SetSeat(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int slot, Form seat, Float bearing)
	If !seat
		Return
	EndIf
	forms[slot] = seat
	fwd[slot] = FireForward() + SeatRadius * Math.Cos(bearing)
	side[slot] = SeatRadius * Math.Sin(bearing)
	turn[slot] = bearing + 180.0
EndFunction

; Puts bedroll `index` of `count` in a slot, side by side across the tent at
; (tentFwd, tentSide) facing the same way as the tent. The row runs from left
; to right as seen from the fire. A small tent's row is centred in it; a large
; tent's is laid out for a full tent and filled from the left, so its pairs
; always sit 1+2, 3+4.
Function SetTentBedroll(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int slot, Float tentFwd, Float tentSide, Float tentTurn, Int index, Int count, Int size)
	Int rowLength = count
	Float back = 0.0
	Float left = 0.0
	If size == kLarge
		rowLength = LargeTentCapacity
		back = LargeTentBedrollBack
		left = LargeTentBedrollLeft
	EndIf
	; Offset along the tent's right axis, which points at tentTurn + 90. The
	; tent faces the fire, so its right is the left as seen from the fire.
	; The tent's forward axis, at tentTurn, points at the fire.
	Float offset = RowPosition(rowLength - 1) / 2.0 - RowPosition(index) + left
	forms[slot] = LZC_BedrollPlaced
	fwd[slot] = tentFwd + offset * Math.Cos(tentTurn + 90.0) - back * Math.Cos(tentTurn)
	side[slot] = tentSide + offset * Math.Sin(tentTurn + 90.0) - back * Math.Sin(tentTurn)
	turn[slot] = tentTurn
	If size == kLarge
		; Turned round, pillows at the back, away from the entrance.
		turn[slot] = tentTurn + 180.0
	EndIf
EndFunction

; A bedroll in the ring (without tents), lying sideways at fan `place` round
; the fire with its seat facing the fire. Its head points clockwise, or
; anticlockwise if `pairHigh`: it's then the clockwise one of a head-to-head
; pair with the place before it. Orders are 2 per place, and a pair's two
; are 1 apart, so only pairs count as neighbours for couples.
Function SetRingBedroll(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int[] bedOrder, Int[] clutterSet, Float[] ringSeatTurn, Bool[] used, Int index, Int place, Bool pairHigh)
	Int slot = kFirstBedroll + index
	Float bearing = place * kRingStep
	forms[slot] = LZC_BedrollPlaced
	fwd[slot] = FireForward() + BedrollRingRadius * Math.Cos(bearing)
	side[slot] = BedrollRingRadius * Math.Sin(bearing)
	turn[slot] = bearing + BedrollRingYaw
	bedOrder[index] = place * 2
	If pairHigh
		turn[slot] = turn[slot] + 180.0
		bedOrder[index] = place * 2 - 1
	EndIf
	; Only the player's bedroll gets the bedroll clutter set.
	If index == 0
		clutterSet[slot] = 4
	EndIf
	ringSeatTurn[index] = bearing + 180.0
	used[place + 2] = True
EndFunction

; Where bedroll `index` lies across a tent's row, from the first one: in pairs
; that touch, PairedBedrollSpacing apart, with BedrollSpacing between pairs.
Float Function RowPosition(Int index)
	Return (index / 2) * (PairedBedrollSpacing + BedrollSpacing) + (index % 2) * PairedBedrollSpacing
EndFunction

; A tent bedroll's place in its row for matching couples, where neighbours
; differ by 1: the two of a pair do, but pairs are 2 apart, so a couple
; takes a pair that touches.
Int Function PairedOrder(Int index)
	Return index + index / 2
EndFunction

; How many kits (colours and variants) there are of this size.
Int Function KitCount(Int size)
	MiscObject[] kits = LZC_SmallTentKits
	If size == kLarge
		kits = LZC_LargeTentKits
	EndIf
	If !kits
		Return 0
	EndIf
	Return kits.Length
EndFunction

MiscObject Function TentKit(Int size, Int color)
	MiscObject[] kits = LZC_SmallTentKits
	If size == kLarge
		kits = LZC_LargeTentKits
	EndIf
	If color < 0 || color >= kits.Length
		Return None
	EndIf
	Return kits[color]
EndFunction

; Returns the tent for a size and colour, falling back to the first (brown).
Form Function TentForm(Int size, Int color)
	Activator[] tents = LZC_SmallTents
	If size == kLarge
		tents = LZC_LargeTents
	EndIf
	If !tents
		Return None
	EndIf
	If color >= 0 && color < tents.Length && tents[color]
		Return tents[color]
	EndIf
	Return tents[0]
EndFunction

; The fewest bedrolls tent `index` starts with: the first (the player's)
; holds the player and their partners, the next `couples` tents hold a
; follower couple each, and the rest one bedroll. Never more than capacity.
Int Function MinBeds(Int index, Int size, Int partners, Int couples)
	Int want = 1
	If index == 0
		want = 1 + partners
	ElseIf index <= couples
		want = 2
	EndIf
	If want > TentCapacity(size)
		Return TentCapacity(size)
	EndIf
	Return want
EndFunction

Int Function BedsNeeded(Int tents, Int[] sizes, Int partners, Int couples)
	Int needed = 0
	Int t = 0
	While t < tents
		needed += MinBeds(t, sizes[t], partners, couples)
		t += 1
	EndWhile
	Return needed
EndFunction

Int Function TentCapacity(Int size)
	Int capacity = SmallTentCapacity
	If size == kLarge
		capacity = LargeTentCapacity
		; The tub takes the third and fourth bedrolls' place.
		If TubInLargeTent && PlaceTub && LZC_Tub && capacity > 2
			capacity = 2
		EndIf
	EndIf
	If capacity < 1
		Return 1
	ElseIf capacity > 4
		Return 4
	EndIf
	Return capacity
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

; Appends a clutter set's items to the flattened lists for PlaceCamp,
; tagged with the set's number. Returns the new count.
Int Function AnchorID(ObjectReference anchor)
	If anchor
		Return anchor.GetFormID()
	EndIf
	Return 0
EndFunction

Int Function AddSet(ObjectReference[] setItems, Int[] itemSet, Int count, ObjectReference[] items, Int setNumber)
	Int i = 0
	While items && i < items.Length && count < setItems.Length
		; The tub water isn't copied; it moves onto the tub itself.
		If items[i] && items[i] != LZC_TubWater
			setItems[count] = items[i]
			itemSet[count] = setNumber
			count += 1
		EndIf
		i += 1
	EndWhile
	Return count
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
