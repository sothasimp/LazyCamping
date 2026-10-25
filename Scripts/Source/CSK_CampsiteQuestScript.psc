Scriptname CSK_CampsiteQuestScript extends Quest
{Central controller for the Set Up Camp power. Tracks the deployed campsite,
checks/consumes the camping kits in the player's inventory, lays out the camp
and packs it back up. CampsiteKit.dll (SKSE) checks the ground and places the
camp in one go.}

; ---------------------------------------------------------------------------
; Core
; ---------------------------------------------------------------------------
Actor Property PlayerRef Auto
Spell Property CSK_SetUpCampPower Auto
{The Lesser Power. Granted to the player when this quest starts.}

; ---------------------------------------------------------------------------
; Inventory kits (MiscObjects the player must carry)
; ---------------------------------------------------------------------------
MiscObject[] Property CSK_SmallTentKits Auto
{Small tent kits by colour: 0 brown, 1 blue, 2 green, 3 red, 4 white. Every
tent kit the player carries is pitched (up to 4 tents).}
MiscObject[] Property CSK_LargeTentKits Auto
{Large tent kits, same colour order as CSK_SmallTentKits.}
MiscObject Property CSK_BedrollKit Auto
MiscObject Property CSK_CampfireKit Auto

MiscObject Property Firewood01 Auto
{Optional. Vanilla firewood, consumed (not refunded) each time a fire is built.}
Int Property FirewoodRequired = 0 Auto
{How much firewood building the fire costs. 0 disables the requirement.}

; ---------------------------------------------------------------------------
; World objects placed when the camp is set up
; ---------------------------------------------------------------------------
Form[] Property CSK_SmallTents Auto
{Small tent Activators, same colour order as CSK_SmallTentKits. Each needs
CSK_TentActivatorScript.}
Form[] Property CSK_LargeTents Auto
{Large tent Activators, same colour order as CSK_SmallTents.}
Form Property CSK_BedrollPlaced Auto
{Furniture with a sleep marker, so the player can sleep / wait in it. Can be
the vanilla bedroll: this script needs nothing on it.}
Form Property CSK_CampfirePlaced Auto
{Static or Activator: the burning campfire. Can be the vanilla one.}
Light Property CSK_CampfireLight Auto
{Optional. Flickering firelight placed above the campfire.}
Form Property CSK_SitMarkerPlayer Auto
{Optional. Visible cross-legged seat by the fire that the player can use.}
Form Property CSK_SitMarkerFollower Auto
{Optional. Cross-legged sit marker by the fire, one per follower.}

; ---------------------------------------------------------------------------
; Dressing sets: clutter laid out in the Creation Kit (in a holding cell)
; around a marker that stands in for a camp object. Each fire, tent and
; tentless bedroll gets a copy of its set, turned to match. Leave a set's
; anchor empty for no dressing.
; ---------------------------------------------------------------------------
ObjectReference Property CSK_FireDressingAnchor Auto
ObjectReference[] Property CSK_FireDressing Auto
ObjectReference Property CSK_SmallTentDressingAnchor Auto
{Faces the way the tent's entrance does.}
ObjectReference[] Property CSK_SmallTentDressing Auto
ObjectReference Property CSK_LargeTentDressingAnchor Auto
{Faces the way the tent's entrance does.}
ObjectReference[] Property CSK_LargeTentDressing Auto
ObjectReference Property CSK_RingBedrollDressingAnchor Auto
{For camps without tents; faces the way the bedroll does.}
ObjectReference[] Property CSK_RingBedrollDressing Auto

ObjectReference Property CSK_CampStorage Auto
{Optional. A persistent, non-respawning, unowned container kept in the
holding cell. It's moved to the camp while one is up, so its contents carry
over from camp to camp.}
ObjectReference[] Property CSK_CampStorageMarkers Auto
{Items in the dressing sets that mark where the storage goes (they're also
listed in their set). The first one in camp order is used; the storage
returns to the first marker's spot in the holding cell on pack-up.}

ObjectReference Property CSK_CampMapMarker Auto
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

Float Property TentRingGap = 60.0 Auto
{Gap between the seats and the front of the deepest tent. All tents share one
ring: SeatRadius + TentRingGap + the largest HalfDepth among them.}
Float Property SmallTentHalfWidth = 90.0 Auto
Float Property SmallTentHalfDepth = 120.0 Auto
Int Property SmallTentCapacity = 2 Auto
{Most bedrolls a small tent holds (1-4).}
Float Property LargeTentHalfWidth = 150.0 Auto
Float Property LargeTentHalfDepth = 170.0 Auto
Int Property LargeTentCapacity = 4 Auto
{Most bedrolls a large tent holds (1-4).}
Float Property BedrollSpacing = 70.0 Auto
{Side-to-side distance between bedrolls in a tent.}

Float Property BedrollRingRadius = 230.0 Auto
{Without tents: distance of the bedrolls from the centre of the fire, just
outside the seats.}
Float Property BedrollRingYaw = 90.0 Auto
{Without tents: added to each bedroll's facing so it lies sideways along the
circle. Use 270 to flip head and foot, or adjust if your bedroll model's long
axis isn't its forward axis.}

; ---------------------------------------------------------------------------
; Followers
; ---------------------------------------------------------------------------
ReferenceAlias[] Property RingBedrollAliases Auto
{RingBedroll1-4 on this quest: optional, empty Specific Reference aliases with
CSK_BedrollAliasScript. In a camp without tents they hold the bedrolls, so
sneaking and activating one packs up the camp.}

Quest Property CSK_CampFollowersQuest Auto
{Optional. Seats followers by the fire and puts them to bed with the player.
Leave empty to place only the player's seat and the bedrolls.}

; ---------------------------------------------------------------------------
; Ground
; ---------------------------------------------------------------------------
Bool Property AlignToSlope = True Auto
{Tilt the tents, bedrolls, seats and fire to match the ground.}
Float Property MaxSlope = 25.0 Auto
{Refuse to camp where the ground is steeper than this (degrees). Objects
tilt by at most this much.}
Float Property MaxStep = 48.0 Auto
{Refuse to camp if the ground under a tent varies by more than this
(catches rocks, logs and ledges).}

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
Int Property BedrollsPlaced = 0 Auto Hidden

; Slots in placedRefs.
Int Property kCampfire = 0 AutoReadOnly
Int Property kLight = 1 AutoReadOnly
Int Property kPlayerSeat = 2 AutoReadOnly
Int Property kFirstFollowerSeat = 3 AutoReadOnly   ; 3-5
Int Property kFirstTent = 6 AutoReadOnly           ; 6-9
Int Property kFirstBedroll = 10 AutoReadOnly       ; 10-25, the player's first
Int Property kSlotCount = 26 AutoReadOnly

Int Property kMaxFollowers = 3 AutoReadOnly
Int Property kMaxTents = 4 AutoReadOnly
Int Property kMaxBedrolls = 16 AutoReadOnly
Int Property kRingBedrolls = 4 AutoReadOnly        ; without tents
Int Property kSmall = 0 AutoReadOnly
Int Property kLarge = 1 AutoReadOnly
Int Property kColors = 5 AutoReadOnly
Float Property kFireHalfSize = 50.0 AutoReadOnly     ; for the ground check
Int Property kNativeVersion = 3 AutoReadOnly         ; CampsiteKit.dll needed

ObjectReference[] placedRefs
MiscObject[] tentKitsUsed   ; refunded on pack-up
Int[] tentSizesUsed
Bool busy = False

Event OnInit()
	PlayerRef.AddSpell(CSK_SetUpCampPower, False)
EndEvent

; Called by the power's magic effect.
Function HandlePowerCast()
	If CampDeployed
		If TentCount == 0
			Debug.Notification("You already have a camp set up. Sneak and use a bedroll to pack it up.")
		Else
			Debug.Notification("You already have a camp set up. Sneak and use a tent to pack it up.")
		EndIf
		Return
	EndIf
	If CSK_Native.GetVersion() < kNativeVersion
		Debug.Notification("Campsite Kit needs SKSE and an up-to-date CampsiteKit.dll.")
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

; Called by CSK_TentActivatorScript. Sneaking and activating any of the camp's
; tents from outside packs up the camp. Anything else does nothing; in
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
			PackUpCamp()
			busy = False
			Return
		EndIf
		t += 1
	EndWhile
EndFunction

; Called by CSK_BedrollAliasScript. In a camp without tents, the bedrolls'
; activation is blocked so this runs first: sneaking packs up the camp, and
; anything else is passed through to the bedroll as normal.
Function OnBedrollActivated(ObjectReference akBedroll, ObjectReference akActionRef)
	If !CampDeployed || TentCount > 0 || !IsCampBedroll(akBedroll)
		Return  ; not blocked, so the bedroll has already done its normal thing
	EndIf

	If akActionRef == PlayerRef && PlayerRef.IsSneaking()
		If !busy
			busy = True
			PackUpCamp()
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
	If PlayerRef.GetItemCount(CSK_BedrollKit) < 1
		missing += CSK_BedrollKit.GetName() + ", "
	EndIf
	If PlayerRef.GetItemCount(CSK_CampfireKit) < 1
		missing += CSK_CampfireKit.GetName() + ", "
	EndIf
	If FirewoodRequired > 0 && Firewood01 && PlayerRef.GetItemCount(Firewood01) < FirewoodRequired
		missing += FirewoodRequired + " " + Firewood01.GetName() + ", "
	EndIf

	If missing != ""
		Debug.Notification("You need: " + missing + "to set up camp.")
		Return False
	EndIf
	Return True
EndFunction

Function SetUpCamp()
	Float heading = PlayerRef.GetAngleZ()
	Int bedrollsCarried = PlayerRef.GetItemCount(CSK_BedrollKit)

	; Start the follower quest first: its aliases find the followers who are
	; with the player. Each gets a seat by the fire, and partners and couples
	; decide how the bedrolls are shared out between the tents.
	CSK_CampFollowersScript followers = None
	If CSK_CampFollowersQuest
		If CSK_CampFollowersQuest.IsRunning()
			CSK_CampFollowersQuest.Stop()
		EndIf
		If CSK_CampFollowersQuest.Start()
			followers = CSK_CampFollowersQuest as CSK_CampFollowersScript
		EndIf
	EndIf
	Int partners = 0
	Int couples = 0
	If followers
		partners = followers.PartnerCount()
		couples = followers.CoupleCount()
	EndIf

	; Tents: every tent kit the player carries, in kit order (small brown ...
	; white, then large), up to kMaxTents. Each tent needs at least one
	; bedroll, so there are never more tents than bedrolls.
	MiscObject[] kits = new MiscObject[4]
	Int[] sizes = new Int[4]
	Int[] colors = new Int[4]
	Int tents = 0
	Int kit = 0
	While kit < 2 * kColors && tents < kMaxTents && tents < bedrollsCarried
		MiscObject k = TentKit(kit / kColors, kit % kColors)
		If k
			Int have = PlayerRef.GetItemCount(k)
			While have > 0 && tents < kMaxTents && tents < bedrollsCarried
				kits[tents] = k
				sizes[tents] = kit / kColors
				colors[tents] = kit % kColors
				tents += 1
				have -= 1
			EndWhile
		EndIf
		kit += 1
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

		; The tent with the most bedrolls becomes the first tent: it stands
		; across the fire from the player, entrance facing them, and holds the
		; player's bedroll.
		Int main = 0
		t = 1
		While t < tents
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

	; Tents stand on one ring outside the seats, entrances facing the fire.
	; Bearings are degrees clockwise from the player's facing, seen from the fire.
	Float radius = SeatRadius + TentRingGap
	Float deepest = 0.0
	t = 0
	While t < tents
		If TentHalfDepth(sizes[t]) > deepest
			deepest = TentHalfDepth(sizes[t])
		EndIf
		t += 1
	EndWhile
	radius += deepest

	Float[] tentFwd = new Float[4]
	Float[] tentSide = new Float[4]
	Float[] tentTurn = new Float[4]
	t = 0
	While t < tents
		Float bearing = TentBearing(t, tents)
		tentFwd[t] = FireForward() + radius * Math.Cos(bearing)
		tentSide[t] = radius * Math.Sin(bearing)
		tentTurn[t] = bearing + 180.0
		t += 1
	EndWhile

	; Check the ground under the fire and every tent in one go.
	Float[] siteFwd = new Float[5]
	Float[] siteSide = new Float[5]
	Float[] siteTurn = new Float[5]
	Float[] siteHalfWidth = new Float[5]
	Float[] siteHalfDepth = new Float[5]
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
	Int site = CSK_Native.CheckCampSite(PlayerRef, heading, 1 + tents, siteFwd, siteSide, siteTurn, siteHalfWidth, siteHalfDepth, MaxStep, MaxSlope)
	If site != 0
		NotifySiteProblem(site)
		If followers
			CSK_CampFollowersQuest.Stop()
		EndIf
		Return
	EndIf

	; Slot layout (see placedRefs). Index 0, the campfire, is the camp's anchor.
	Form[] forms = new Form[26]
	Float[] fwd = new Float[26]
	Float[] side = new Float[26]
	Float[] lift = new Float[26]
	Float[] turn = new Float[26]

	forms[kCampfire] = CSK_CampfirePlaced
	fwd[kCampfire] = FireForward()
	forms[kLight] = CSK_CampfireLight
	fwd[kLight] = FireForward()
	lift[kLight] = LightHeight

	; Where each bedroll is, for matching partners and couples at bedtime: its
	; group (the tent it's in, or 0 for the ring) and its order within it.
	; Which dressing set (1-based; 0 = none) goes around each layout object.
	Int[] dressSet = new Int[26]
	dressSet[kCampfire] = 1

	Int[] bedGroup = new Int[16]
	Int[] bedOrder = new Int[16]
	Int ringSize = 0

	; Seats ring the fire and face it. The player's is at 0, across the fire
	; in front of their tent; followers' seats are spaced evenly from there.
	SetSeat(forms, fwd, side, turn, kPlayerSeat, CSK_SitMarkerPlayer, 0.0)
	If followers && CSK_SitMarkerFollower
		Int present = 0
		Int f = 0
		While f < kMaxFollowers
			If followers.HasFollower(f)
				present += 1
			EndIf
			f += 1
		EndWhile

		Int rank = 0
		f = 0
		While f < kMaxFollowers
			If followers.HasFollower(f)
				rank += 1
				SetSeat(forms, fwd, side, turn, kFirstFollowerSeat + f, CSK_SitMarkerFollower, rank * 360.0 / (present + 1))
			EndIf
			f += 1
		EndWhile
	EndIf

	If tents > 0
		t = 0
		While t < tents
			forms[kFirstTent + t] = TentForm(sizes[t], colors[t])
			fwd[kFirstTent + t] = tentFwd[t]
			side[kFirstTent + t] = tentSide[t]
			turn[kFirstTent + t] = tentTurn[t]
			dressSet[kFirstTent + t] = 2 + sizes[t]   ; 2 small, 3 large
			t += 1
		EndWhile

		; Bedroll slots: first each tent's first bedroll (the player's is in the
		; first tent, followers' first beds in the others), then the rest.
		Int nextSlot = kFirstBedroll
		t = 0
		While t < tents
			SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], 0, bedsInTent[t])
			bedGroup[nextSlot - kFirstBedroll] = t
			bedOrder[nextSlot - kFirstBedroll] = 0
			nextSlot += 1
			t += 1
		EndWhile
		t = 0
		While t < tents
			Int b = 1
			While b < bedsInTent[t]
				SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], b, bedsInTent[t])
				bedGroup[nextSlot - kFirstBedroll] = t
				bedOrder[nextSlot - kFirstBedroll] = b
				nextSlot += 1
				b += 1
			EndWhile
			t += 1
		EndWhile
	Else
		; Without tents, the bedrolls lie sideways, spaced evenly along a circle
		; just outside the seats, starting behind the player's seat.
		Int r = 0
		While r < bedrolls
			Float ringBearing = r * 360.0 / bedrolls
			forms[kFirstBedroll + r] = CSK_BedrollPlaced
			fwd[kFirstBedroll + r] = FireForward() + BedrollRingRadius * Math.Cos(ringBearing)
			side[kFirstBedroll + r] = BedrollRingRadius * Math.Sin(ringBearing)
			turn[kFirstBedroll + r] = ringBearing + BedrollRingYaw
			bedOrder[r] = r
			dressSet[kFirstBedroll + r] = 4
			r += 1
		EndWhile
		ringSize = bedrolls
	EndIf

	; The plugin puts everything on the ground and places it in one frame.
	ObjectReference[] setAnchors = new ObjectReference[4]
	setAnchors[0] = CSK_FireDressingAnchor
	setAnchors[1] = CSK_SmallTentDressingAnchor
	setAnchors[2] = CSK_LargeTentDressingAnchor
	setAnchors[3] = CSK_RingBedrollDressingAnchor
	ObjectReference[] setItems = new ObjectReference[128]
	Int[] itemSet = new Int[128]
	Int itemCount = AddSet(setItems, itemSet, 0, CSK_FireDressing, 1)
	itemCount = AddSet(setItems, itemSet, itemCount, CSK_SmallTentDressing, 2)
	itemCount = AddSet(setItems, itemSet, itemCount, CSK_LargeTentDressing, 3)
	itemCount = AddSet(setItems, itemSet, itemCount, CSK_RingBedrollDressing, 4)

	placedRefs = CSK_Native.PlaceCamp(PlayerRef, heading, forms, fwd, side, lift, turn, AlignToSlope, MaxSlope, dressSet, setAnchors, setItems, itemSet, CSK_CampStorageMarkers)
	If placedRefs.Length <= kSlotCount
		Debug.Notification("Your camp couldn't be set up.")
		placedRefs = new ObjectReference[26]
		If followers
			CSK_CampFollowersQuest.Stop()
		EndIf
		Return
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

	t = 0
	While t < tents
		PlayerRef.RemoveItem(kits[t], 1, True)
		t += 1
	EndWhile
	PlayerRef.RemoveItem(CSK_BedrollKit, bedrolls, True)
	PlayerRef.RemoveItem(CSK_CampfireKit, 1, True)
	If FirewoodRequired > 0 && Firewood01
		PlayerRef.RemoveItem(Firewood01, FirewoodRequired, True)
	EndIf

	If followers
		; Every camp bedroll goes to the follower quest, which hands them out
		; when the player sleeps, skipping whichever one the player is in.
		ObjectReference[] beds = new ObjectReference[16]
		i = 0
		While i < kMaxBedrolls
			beds[i] = placedRefs[kFirstBedroll + i]
			i += 1
		EndWhile
		ObjectReference[] seats = new ObjectReference[3]
		i = 0
		While i < kMaxFollowers
			seats[i] = placedRefs[kFirstFollowerSeat + i]
			i += 1
		EndWhile
		followers.Assign(placedRefs[kCampfire], placedRefs[kPlayerSeat], beds, bedGroup, bedOrder, ringSize, seats)
	EndIf

	; The player's storage moves onto the spot the plugin marked, which then
	; goes; its contents come along from camp to camp.
	ObjectReference storageSpot = placedRefs[kSlotCount]
	If storageSpot
		If CSK_CampStorage
			CSK_CampStorage.MoveTo(storageSpot)
		EndIf
		storageSpot.Delete()
		placedRefs[kSlotCount] = None
	EndIf

	If CSK_CampMapMarker
		CSK_CampMapMarker.MoveTo(placedRefs[kCampfire])
		CSK_CampMapMarker.Enable()
		CSK_CampMapMarker.AddToMap(True)
	EndIf

	tentKitsUsed = kits
	tentSizesUsed = sizes
	TentCount = tents
	BedrollsPlaced = bedrolls
	CampDeployed = True
	Debug.Notification("You set up camp.")
EndFunction

Function PackUpCamp()
	If PlayerRef.GetSitState() != 0
		Debug.Notification("You must get up before packing up camp.")
		Return
	EndIf

	If CSK_CampFollowersQuest && CSK_CampFollowersQuest.IsRunning()
		(CSK_CampFollowersQuest as CSK_CampFollowersScript).Shutdown()
	EndIf
	If CSK_CampMapMarker
		CSK_CampMapMarker.Disable()
	EndIf
	If CSK_CampStorage && CSK_CampStorageMarkers && CSK_CampStorageMarkers[0]
		CSK_CampStorage.MoveTo(CSK_CampStorageMarkers[0])
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

	i = 0
	While i < TentCount
		If tentKitsUsed[i]
			PlayerRef.AddItem(tentKitsUsed[i], 1, True)
		EndIf
		i += 1
	EndWhile
	PlayerRef.AddItem(CSK_BedrollKit, BedrollsPlaced, True)
	PlayerRef.AddItem(CSK_CampfireKit, 1, True)

	TentCount = 0
	BedrollsPlaced = 0
	CampDeployed = False
	Debug.Notification("You pack up your camp.")
EndFunction

; How far in front of the player the fire goes: one seat radius, so the
; player stands at the near edge of the seat ring.
Float Function FireForward()
	Return SeatRadius
EndFunction

; Bearing of tent `index` out of `count`, spaced evenly around the fire. The
; first tent is at 0, straight across the fire from the player. The player
; stands on the ring of seats, inside the ring of tents, so a tent at 180
; simply stands behind them.
Float Function TentBearing(Int index, Int count)
	Return index * 360.0 / count
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
	; Offset along the tent's right axis, which points at tentTurn + 90.
	Float offset = (index - (count - 1) / 2.0) * BedrollSpacing
	forms[slot] = CSK_BedrollPlaced
	fwd[slot] = tentFwd + offset * Math.Cos(tentTurn + 90.0)
	side[slot] = tentSide + offset * Math.Sin(tentTurn + 90.0)
	turn[slot] = tentTurn
EndFunction

MiscObject Function TentKit(Int size, Int color)
	MiscObject[] kits = CSK_SmallTentKits
	If size == kLarge
		kits = CSK_LargeTentKits
	EndIf
	If color < 0 || color >= kits.Length
		Return None
	EndIf
	Return kits[color]
EndFunction

; Returns the tent for a size and colour, falling back to the first (brown).
Form Function TentForm(Int size, Int color)
	Form[] tents = CSK_SmallTents
	If size == kLarge
		tents = CSK_LargeTents
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

; Appends a dressing set's items to the flattened lists for PlaceCamp,
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
