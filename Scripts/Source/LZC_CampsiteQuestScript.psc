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
{Small tent kits by colour: 0 brown, 1 blue, 2 green, 3 red, 4 white. Every
tent kit the player carries is pitched (up to 4 tents).}
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

; ---------------------------------------------------------------------------
; World objects placed when the camp is set up
; ---------------------------------------------------------------------------
Form[] Property LZC_SmallTents Auto
{Small tent Activators, same colour order as LZC_SmallTentKits. Each needs
LZC_TentActivatorScript.}
Form[] Property LZC_LargeTents Auto
{Large tent Activators, same colour order as LZC_SmallTents.}
Form Property LZC_BedrollPlaced Auto
{Furniture with a sleep marker, so the player can sleep / wait in it. Can be
the vanilla bedroll: this script needs nothing on it.}
Form Property LZC_CampfirePlaced Auto
{Static or Activator: the burning campfire. Can be the vanilla one.}
Light Property LZC_CampfireLight Auto
{Optional. Flickering firelight placed above the campfire.}
Bool Property PlaceCampfireLight = True Auto
{Place LZC_CampfireLight. Turn off if a Light Placer config already gives the
campfire its own light.}
Form Property LZC_Firepit Auto
{The unlit firepit placed when FirewoodMechanic is on: an Activator with
LZC_FirepitScript. The fire and its light are placed hidden in its spot.}
Form Property LZC_ChoppingBlock Auto
{Optional. A woodcutting block placed beside the outermost tent (or along the
bedroll fan without tents) when FirewoodMechanic is on. Can be the vanilla one.}
Form Property LZC_SitMarkerPlayer Auto
{Optional. Visible cross-legged seat by the fire that the player can use.}
Form Property LZC_BedrollSitMarker Auto
{Optional. Cross-legged seat placed on every bedroll, facing the fire.
Invisible but selectable: its model is a small collision box with no visible
geometry, so the player can aim at it apart from the bedroll.}
Form Property LZC_SitMarkerFollower Auto
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
Float Property LightHeight = 40.0 Auto
Float Property SeatRadius = 110.0 Auto
{Distance of the seats from the centre of the fire. The fire goes this far in
front of the player, so they stand at the near edge of the seat ring; their
own seat is across the fire, in front of their tent.}

Float Property TentRingGap = 40.0 Auto
{Gap between the seats and the front of the deepest tent. All tents share one
ring: SeatRadius + TentRingGap + the largest HalfDepth among them, widened
when needed so neighbouring tents don't touch. The default leaves the tent
fronts just behind the seats; raise it if seated people brush the tents.}
Float Property SmallTentHalfWidth = 85.0 Auto
Float Property SmallTentHalfDepth = 85.0 Auto
Int Property SmallTentCapacity = 2 Auto
{Most bedrolls a small tent holds (1-4).}
Float Property LargeTentHalfWidth = 150.0 Auto
Float Property LargeTentHalfDepth = 100.0 Auto
Int Property LargeTentCapacity = 4 Auto
{Most bedrolls a large tent holds (1-4).}
Float Property PairedBedrollSpacing = 60.0 Auto
{Bedrolls in a tent lie in pairs that touch, for couples: the side-to-side
distance between the two of a pair. Set it to the bedroll's width.}
Float Property BedrollSpacing = 70.0 Auto
{Side-to-side distance from one pair's bedroll to the next pair's, in a tent.}

Float Property BedrollRingRadius = 190.0 Auto
{Without tents: distance of the bedrolls from the centre of the fire, just
outside the seats.}
Float Property BedrollRingYaw = 90.0 Auto
{Without tents: added to each bedroll's facing so it lies sideways along the
circle, head clockwise. Use 270 to flip head and foot, or adjust if your
bedroll model's long axis isn't its forward axis.}

; ---------------------------------------------------------------------------
; Followers
; ---------------------------------------------------------------------------
ReferenceAlias[] Property RingBedrollAliases Auto
{RingBedroll1-4 on this quest: optional, empty Specific Reference aliases with
LZC_BedrollAliasScript. In a camp without tents they hold the bedrolls, so
sneaking and activating one packs up the camp.}

Quest Property LZC_CampFollowersQuest Auto
{Optional. Seats followers by the fire and puts them to bed with the player.
Leave empty to place only the player's seat and the bedrolls.}

; ---------------------------------------------------------------------------
; Ground
; ---------------------------------------------------------------------------
Bool Property FadeToBlack = True Auto
{Fade the screen to black while the camp is set up or packed up.}
Float Property kFadeSeconds = 1.0 AutoReadOnly

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
Int Property kSlotCount = 37 AutoReadOnly
Int Property kMaxFollowerSeats = 5 AutoReadOnly    ; fire seats besides the player's

Int Property kMaxFollowers = 3 AutoReadOnly
Int Property kMaxTents = 3 AutoReadOnly
Int Property kMaxBedrolls = 12 AutoReadOnly
Int Property kRingBedrolls = 4 AutoReadOnly        ; without tents
Int Property kSmall = 0 AutoReadOnly
Int Property kLarge = 1 AutoReadOnly
Int Property kColors = 5 AutoReadOnly
Float Property kFireHalfSize = 50.0 AutoReadOnly     ; for the ground check
Float Property kFanStep = 70.0 AutoReadOnly          ; degrees between neighbours
Float Property kTentClearance = 15.0 AutoReadOnly    ; between neighbouring tents
Float Property kChoppingBlockGap = 60.0 AutoReadOnly ; from a tent's side to the block
Int Property kNativeVersion = 6 AutoReadOnly         ; LazyCamping.dll needed

ObjectReference[] placedRefs
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
	If LZC_Native.GetVersion() < kNativeVersion
		Debug.Notification("Lazy Camping needs SKSE and an up-to-date LazyCamping.dll.")
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

; Called by LZC_TentActivatorScript. Sneaking and activating any of the camp's
; tents from outside offers to pack up the camp. Anything else does nothing; in
; particular the player can't pack up by accident from inside a tent.
Function OnTentActivated(ObjectReference akTent)
	If !CampDeployed || busy || !PlayerRef.IsSneaking()
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

; Called by LZC_BedrollAliasScript. In a camp without tents, the bedrolls'
; activation is blocked so this runs first: sneaking packs up the camp, and
; anything else is passed through to the bedroll as normal.
Function OnBedrollActivated(ObjectReference akBedroll, ObjectReference akActionRef)
	If !CampDeployed || TentCount > 0 || !IsCampBedroll(akBedroll)
		Return  ; not blocked, so the bedroll has already done its normal thing
	EndIf

	If akActionRef == PlayerRef && PlayerRef.IsSneaking()
		If !busy
			busy = True
			ConfirmPackUp()
			busy = False
		EndIf
	Else
		akBedroll.Activate(akActionRef, True)
	EndIf
EndFunction

Bool Function IsCampBedroll(ObjectReference akRef)
	Int i = kFirstBedroll
	While i < kFirstBedroll + kMaxBedrolls
		If placedRefs[i] && placedRefs[i] == akRef
			Return True
		EndIf
		i += 1
	EndWhile
	Return False
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
	While color < kColors && tents == 0 && bedrollsCarried > 0
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
	While color < kColors && tents < kMaxTents && tents < bedrollsCarried
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
	Float deepest = 0.0
	t = 0
	While t < tents
		If TentHalfDepth(sizes[t]) > deepest
			deepest = TentHalfDepth(sizes[t])
		EndIf
		t += 1
	EndWhile
	Float radius = SeatRadius + TentRingGap + deepest
	; Seen from the fire, each tent spans an angle set by its front corners.
	; Push the ring out until the middle tent's span and each side tent's fit
	; within the kFanStep between them.
	t = 1
	While t < tents
		While TentSpan(sizes[0], radius) + TentSpan(sizes[t], radius) > kFanStep
			radius += 5.0
		EndWhile
		t += 1
	EndWhile

	Float[] tentFwd = new Float[4]
	Float[] tentSide = new Float[4]
	Float[] tentTurn = new Float[4]
	t = 0
	While t < tents
		Float bearing = FanBearing(t)
		tentFwd[t] = FireForward() + radius * Math.Cos(bearing)
		tentSide[t] = radius * Math.Sin(bearing)
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
	Form[] forms = new Form[37]
	Float[] fwd = new Float[37]
	Float[] side = new Float[37]
	Float[] lift = new Float[37]
	Float[] turn = new Float[37]

	forms[kCampfire] = LZC_CampfirePlaced
	fwd[kCampfire] = FireForward()
	If PlaceCampfireLight
		forms[kLight] = LZC_CampfireLight
	EndIf
	fwd[kLight] = FireForward()
	lift[kLight] = LightHeight

	; With the firewood mechanic, an unlit firepit stands in the fire's spot;
	; the fire and its light are placed too, but hidden until it's lit.
	If FirewoodMechanic && LZC_Firepit
		forms[kFirepit] = LZC_Firepit
		fwd[kFirepit] = FireForward()
		If LZC_ChoppingBlock
			SetChoppingBlock(forms, fwd, side, turn, tents, sizes, radius, bedrolls)
		EndIf
	EndIf

	; Where each bedroll is, for matching partners and couples at bedtime: its
	; group (the tent it's in, or 0 without tents) and its place in the row
	; (neighbours differ by 1).
	; Which clutter set (1-based; 0 = none) goes around each layout object.
	Int[] clutterSet = new Int[37]
	clutterSet[kCampfire] = 1

	Int[] bedGroup = new Int[12]
	Int[] bedOrder = new Int[12]
	Float[] ringSeatTurn = new Float[12]   ; without tents: each bedroll seat's facing

	; Seats ring the fire and face it. The player's is at 0, across the fire
	; in front of their tent. There's a follower seat for each bedroll beyond
	; the player's, up to kMaxFollowerSeats, fanning out to either side:
	; kFanStep apart while they fit, otherwise evenly round the ring.
	Int followerSeats = 0
	If LZC_SitMarkerFollower
		followerSeats = bedrolls - 1
		If followerSeats > kMaxFollowerSeats
			followerSeats = kMaxFollowerSeats
		EndIf
	EndIf
	Float seatStep = kFanStep
	If (1 + followerSeats) * kFanStep > 360.0
		seatStep = 360.0 / (1 + followerSeats)
	EndIf
	SetSeat(forms, fwd, side, turn, kPlayerSeat, LZC_SitMarkerPlayer, 0.0)
	Int f = 0
	While f < followerSeats
		SetSeat(forms, fwd, side, turn, kFirstFollowerSeat + f, LZC_SitMarkerFollower, FanPlace(f + 1) * seatStep)
		f += 1
	EndWhile

	If tents > 0
		t = 0
		While t < tents
			forms[kFirstTent + t] = TentForm(sizes[t], colors[t])
			fwd[kFirstTent + t] = tentFwd[t]
			side[kFirstTent + t] = tentSide[t]
			turn[kFirstTent + t] = tentTurn[t]
			clutterSet[kFirstTent + t] = 2 + sizes[t]   ; 2 small, 3 large
			t += 1
		EndWhile

		; Bedroll slots: first each tent's first bedroll (the player's is in the
		; first tent, followers' first beds in the others), then the rest.
		Int nextSlot = kFirstBedroll
		t = 0
		While t < tents
			SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], 0, bedsInTent[t])
			bedGroup[nextSlot - kFirstBedroll] = t
			bedOrder[nextSlot - kFirstBedroll] = PairedOrder(0)
			nextSlot += 1
			t += 1
		EndWhile
		t = 0
		While t < tents
			Int b = 1
			While b < bedsInTent[t]
				SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], b, bedsInTent[t])
				bedGroup[nextSlot - kFirstBedroll] = t
				bedOrder[nextSlot - kFirstBedroll] = PairedOrder(b)
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
					turn[kFirstBedrollSeat + bs] = turn[kFirstBedroll + bs]
				Else
					turn[kFirstBedrollSeat + bs] = ringSeatTurn[bs]
				EndIf
			EndIf
			bs += 1
		EndWhile
	EndIf

	; The plugin puts everything on the ground and places it in one frame.
	ObjectReference[] setAnchors = new ObjectReference[4]
	setAnchors[0] = LZC_FireClutterAnchor
	setAnchors[1] = LZC_SmallTentClutterAnchor
	setAnchors[2] = LZC_LargeTentClutterAnchor
	setAnchors[3] = LZC_RingBedrollClutterAnchor
	ObjectReference[] setItems = new ObjectReference[128]
	Int[] itemSet = new Int[128]
	Int itemCount = AddSet(setItems, itemSet, 0, LZC_FireClutter, 1)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_SmallTentClutter, 2)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_LargeTentClutter, 3)
	itemCount = AddSet(setItems, itemSet, itemCount, LZC_RingBedrollClutter, 4)

	FadeOut()
	placedRefs = LZC_Native.PlaceCamp(PlayerRef, heading, forms, fwd, side, lift, turn, AlignToSlope, MaxSlope, clutterSet, setAnchors, setItems, itemSet, LZC_CampStorageMarkers)
	If placedRefs.Length <= kSlotCount
		FadeIn()
		Debug.Notification("Your camp couldn't be set up.")
		placedRefs = new ObjectReference[37]
		If followers
			LZC_CampFollowersQuest.Stop()
		EndIf
		Return
	EndIf

	If placedRefs[kFirepit]
		SetFireLit(False)
	EndIf

	; Without tents, the bedrolls go into the RingBedroll aliases, whose
	; script handles activation first, so sneaking and activating one can
	; pack up the camp.
	Int i = 0
	If tents == 0
		While i < bedrolls && i < RingBedrollAliases.Length
			ObjectReference bed = placedRefs[kFirstBedroll + i]
			If bed
				bed.BlockActivation(True)
				RingBedrollAliases[i].ForceRefTo(bed)
			EndIf
			i += 1
		EndWhile
	EndIf

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
			firePlaces[i + 1] = FanPlace(i + 1)
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
	FadeIn()
EndFunction

; Called by LZC_FirepitScript. Sneak-activating the camp's unlit firepit
; lights the fire, burning one firewood, for FireBurnHours.
Function OnFirepitActivated(ObjectReference akFirepit, ObjectReference akActionRef)
	If akActionRef != PlayerRef || !PlayerRef.IsSneaking() || !CampDeployed || akFirepit != placedRefs[kFirepit]
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
	Else
		firepit.Enable()
		fire.Disable()
		If fireLight
			fireLight.Disable()
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
		FadeOut()
		PackUpCamp()
		FadeIn()
	EndIf
EndFunction

; Fades the screen to black (if FadeToBlack) and waits until it's dark.
Function FadeOut()
	If FadeToBlack
		Game.FadeOutGame(True, True, 0.0, kFadeSeconds)
		Utility.Wait(kFadeSeconds)
	EndIf
EndFunction

Function FadeIn()
	If FadeToBlack
		Game.FadeOutGame(False, True, 0.0, kFadeSeconds)
	EndIf
EndFunction

Function PackUpCamp()
	UnregisterForUpdateGameTime()
	LZC_Native.ShowGrass()

	If LZC_CampFollowersQuest && LZC_CampFollowersQuest.IsRunning()
		(LZC_CampFollowersQuest as LZC_CampFollowersScript).Shutdown()
	EndIf
	If LZC_CampMapMarker
		LZC_CampMapMarker.Disable()
	EndIf
	If LZC_CampStorage && LZC_CampStorageMarkers && LZC_CampStorageMarkers[0]
		LZC_CampStorage.MoveTo(LZC_CampStorageMarkers[0])
	EndIf

	Int i = 0
	While i < RingBedrollAliases.Length
		RingBedrollAliases[i].Clear()
		i += 1
	EndWhile

	i = 0
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
Function SetChoppingBlock(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int tents, Int[] sizes, Float radius, Int bedrolls)
	Float bearing = FanBearing(bedrolls)
	Float distance = BedrollRingRadius
	If tents > 0
		Int outer = tents - 1
		Float tentBearing = FanBearing(outer)
		Float outward = 1.0
		If tentBearing < 0.0
			outward = -1.0
		EndIf
		; Along the tent ring, past the tent's side by kChoppingBlockGap.
		bearing = tentBearing + outward * (TentHalfWidth(sizes[outer]) + kChoppingBlockGap) / radius * 57.29578
		distance = radius
	EndIf
	forms[kChoppingBlock] = LZC_ChoppingBlock
	fwd[kChoppingBlock] = FireForward() + distance * Math.Cos(bearing)
	side[kChoppingBlock] = distance * Math.Sin(bearing)
	turn[kChoppingBlock] = bearing + 180.0
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
; (tentFwd, tentSide) facing the same way as the tent.
Function SetTentBedroll(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int slot, Float tentFwd, Float tentSide, Float tentTurn, Int index, Int count)
	; Offset along the tent's right axis, which points at tentTurn + 90. The
	; row is centred in the tent.
	Float offset = RowPosition(index) - RowPosition(count - 1) / 2.0
	forms[slot] = LZC_BedrollPlaced
	fwd[slot] = tentFwd + offset * Math.Cos(tentTurn + 90.0)
	side[slot] = tentSide + offset * Math.Sin(tentTurn + 90.0)
	turn[slot] = tentTurn
EndFunction

; A bedroll in the ring (without tents), lying sideways at fan `place` round
; the fire with its seat facing the fire. Its head points clockwise, or
; anticlockwise if `pairHigh`: it's then the clockwise one of a head-to-head
; pair with the place before it. Orders are 2 per place, and a pair's two
; are 1 apart, so only pairs count as neighbours for couples.
Function SetRingBedroll(Form[] forms, Float[] fwd, Float[] side, Float[] turn, Int[] bedOrder, Int[] clutterSet, Float[] ringSeatTurn, Bool[] used, Int index, Int place, Bool pairHigh)
	Int slot = kFirstBedroll + index
	Float bearing = place * kFanStep
	forms[slot] = LZC_BedrollPlaced
	fwd[slot] = FireForward() + BedrollRingRadius * Math.Cos(bearing)
	side[slot] = BedrollRingRadius * Math.Sin(bearing)
	turn[slot] = bearing + BedrollRingYaw
	bedOrder[index] = place * 2
	If pairHigh
		turn[slot] = turn[slot] + 180.0
		bedOrder[index] = place * 2 - 1
	EndIf
	clutterSet[slot] = 4
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
	Form[] tents = LZC_SmallTents
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

; Half the angle, in degrees, that a tent of this size spans as seen from
; the fire when it stands `radius` from it (plus half the clearance).
Float Function TentSpan(Int size, Float radius)
	Return Math.atan((TentHalfWidth(size) + kTentClearance / 2.0) / (radius - TentHalfDepth(size)))
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
Int Function AddSet(ObjectReference[] setItems, Int[] itemSet, Int count, ObjectReference[] items, Int setNumber)
	Int i = 0
	While items && i < items.Length && count < setItems.Length
		If items[i]
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
