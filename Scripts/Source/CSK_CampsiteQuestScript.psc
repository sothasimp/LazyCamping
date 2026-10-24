Scriptname CSK_CampsiteQuestScript extends Quest
{Central controller for the Set Up Camp power. Tracks the deployed campsite,
checks/consumes the camping kits in the player's inventory, places the camp
in the world and packs it back up.}

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
{Furniture with a sleep marker, so the player can sleep / wait in it. Needs
CSK_BedrollScript: in a camp without tents, sneaking and activating a
bedroll packs up the camp.}
Form Property CSK_CampfirePlaced Auto
{Static or Activator: the burning campfire.}
Light Property CSK_CampfireLight Auto
{Optional. Flickering firelight placed above the campfire.}
Form Property CSK_CookingStation Auto
{Optional. Cooking pot / spit furniture placed over the fire.}
Form Property CSK_SitMarkerPlayer Auto
{Optional. Visible cross-legged seat by the fire that the player can use.}
Form Property CSK_SitMarkerFollower Auto
{Optional. Cross-legged sit marker by the fire, one per follower.}

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
Quest Property CSK_CampFollowersQuest Auto
{Optional. Seats followers by the fire and puts them to bed with the player.
Leave empty to place only the player's seat and the bedrolls.}

; ---------------------------------------------------------------------------
; Native placement (CampsiteKit.dll). Ignored if the SKSE plugin isn't loaded.
; ---------------------------------------------------------------------------
Bool Property UseNativePlacement = True Auto
{Use the SKSE plugin to snap the camp to the ground when it is available.}
Bool Property AlignToSlope = True Auto
{Tilt the tents, bedrolls, seats and fire to match the ground.}
Float Property MaxTilt = 20.0 Auto
{Objects never tilt more than this many degrees, even on steeper ground.}
Float Property MaxSlope = 25.0 Auto
{Refuse to camp where the ground is steeper than this (degrees).}
Float Property MaxStep = 48.0 Auto
{Refuse to camp if the ground under a tent varies by more than this
(catches rocks, logs and ledges).}
Float Property CampfireHalfSize = 50.0 Auto
Float Property SearchUp = 256.0 Auto
Float Property SearchDown = 1024.0 Auto
{How far above/below the player to look for ground.}

; ---------------------------------------------------------------------------
; Restrictions
; ---------------------------------------------------------------------------
Keyword Property LocTypeCity Auto
{Optional. Camping is blocked in locations with this keyword.}
Keyword Property LocTypeTown Auto
{Optional. Camping is blocked in locations with this keyword.}
Bool Property AllowPackUpFromPower = False Auto
{If true, casting the power while a camp is up packs it up, so a camp left
far away can still be retrieved. Otherwise only sneaking and activating a
tent (or, without tents, a bedroll) does.}
Float Property MaxPackUpDistance = 0.0 Auto
{With AllowPackUpFromPower: if > 0, the player must be this close to the camp.}

; ---------------------------------------------------------------------------
; State
; ---------------------------------------------------------------------------
Bool Property CampDeployed = False Auto Hidden
Int Property TentCount = 0 Auto Hidden
Int Property BedrollsPlaced = 0 Auto Hidden

; Slots in placedRefs.
Int Property kCampfire = 0 AutoReadOnly
Int Property kLight = 1 AutoReadOnly
Int Property kCooking = 2 AutoReadOnly
Int Property kPlayerSeat = 3 AutoReadOnly
Int Property kFirstFollowerSeat = 4 AutoReadOnly   ; 4-6
Int Property kFirstTent = 7 AutoReadOnly           ; 7-10
Int Property kFirstBedroll = 11 AutoReadOnly       ; 11-26, the player's first
Int Property kSlotCount = 27 AutoReadOnly

Int Property kMaxFollowers = 3 AutoReadOnly
Int Property kMaxTents = 4 AutoReadOnly
Int Property kMaxBedrolls = 16 AutoReadOnly
Int Property kRingBedrolls = 4 AutoReadOnly        ; without tents
Int Property kSmall = 0 AutoReadOnly
Int Property kLarge = 1 AutoReadOnly
Int Property kColors = 5 AutoReadOnly

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
		If !AllowPackUpFromPower
			If TentCount == 0
				Debug.Notification("You already have a camp set up. Sneak and use a bedroll to pack it up.")
			Else
				Debug.Notification("You already have a camp set up. Sneak and use a tent to pack it up.")
			EndIf
		ElseIf MaxPackUpDistance > 0.0 && PlayerRef.GetDistance(placedRefs[kCampfire]) > MaxPackUpDistance
			Debug.Notification("You are too far from your camp to pack it up.")
		ElseIf !busy
			busy = True
			PackUpCamp()
			busy = False
		EndIf
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

; Called by CSK_BedrollScript. In a camp without tents, the bedrolls'
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
	Bool native = UseNativePlacement && CSK_Native.GetVersion() > 0
	Int bedrollsCarried = PlayerRef.GetItemCount(CSK_BedrollKit)

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

	; Bedrolls: one in each tent, then the rest in turn up to each tent's
	; capacity. Without tents, up to kRingBedrolls ring the fire.
	Int[] bedsInTent = new Int[4]
	Int bedrolls = 0
	Int t = 0
	If tents > 0
		While t < tents
			bedsInTent[t] = 1
			t += 1
		EndWhile
		bedrolls = tents
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

	If native && !IsSiteClear(heading, tents, sizes, tentFwd, tentSide, tentTurn)
		Return
	EndIf

	; Start the follower quest: its aliases find the followers who are with
	; the player, and each one gets a seat by the fire.
	CSK_CampFollowersScript followers = None
	If CSK_CampFollowersQuest
		If CSK_CampFollowersQuest.IsRunning()
			CSK_CampFollowersQuest.Stop()
		EndIf
		If CSK_CampFollowersQuest.Start()
			followers = CSK_CampFollowersQuest as CSK_CampFollowersScript
		EndIf
	EndIf

	; Slot layout (see placedRefs). Index 0, the campfire, is the camp's anchor.
	Form[] forms = new Form[27]
	Float[] fwd = new Float[27]
	Float[] side = new Float[27]
	Float[] lift = new Float[27]
	Float[] turn = new Float[27]

	forms[kCampfire] = CSK_CampfirePlaced
	fwd[kCampfire] = FireForward()
	forms[kLight] = CSK_CampfireLight
	fwd[kLight] = FireForward()
	lift[kLight] = LightHeight
	forms[kCooking] = CSK_CookingStation
	fwd[kCooking] = FireForward()

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
			t += 1
		EndWhile

		; Bedroll slots: first each tent's first bedroll (the player's is in the
		; first tent, followers' first beds in the others), then the rest.
		Int nextSlot = kFirstBedroll
		t = 0
		While t < tents
			SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], 0, bedsInTent[t])
			nextSlot += 1
			t += 1
		EndWhile
		t = 0
		While t < tents
			Int b = 1
			While b < bedsInTent[t]
				SetTentBedroll(forms, fwd, side, turn, nextSlot, tentFwd[t], tentSide[t], tentTurn[t], b, bedsInTent[t])
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
			r += 1
		EndWhile
	EndIf

	; Phase 1: work out every position before placing anything, so a
	; bedroll's ground check can't hit the tent standing over it.
	Float[] posX = new Float[27]
	Float[] posY = new Float[27]
	Float[] posZ = new Float[27]
	Float[] angX = new Float[27]
	Float[] angY = new Float[27]
	Float[] angZ = new Float[27]

	Int i = 0
	While i < kSlotCount
		If forms[i]
			posX[i] = OffsetX(heading, fwd[i], side[i])
			posY[i] = OffsetY(heading, fwd[i], side[i])
			posZ[i] = PlayerRef.GetPositionZ() + lift[i]
			angZ[i] = heading + turn[i]

			If native
				Float[] p = CSK_Native.FindPlacement(PlayerRef, posX[i], posY[i], angZ[i], AlignToSlope, MaxTilt, SearchUp, SearchDown)
				If p.Length == 5
					posZ[i] = p[0] + lift[i]
					angX[i] = p[1]
					angY[i] = p[2]
					angZ[i] = p[3]
				EndIf
			EndIf
		EndIf
		i += 1
	EndWhile

	; Phase 2: place.
	placedRefs = new ObjectReference[27]
	i = 0
	While i < kSlotCount
		placedRefs[i] = PlaceAt(forms[i], posX[i], posY[i], posZ[i], angX[i], angY[i], angZ[i])
		; Without tents, the bedrolls' scripts handle activation first, so
		; sneaking and activating one can pack up the camp.
		If tents == 0 && i >= kFirstBedroll && placedRefs[i]
			placedRefs[i].BlockActivation(True)
		EndIf
		i += 1
	EndWhile

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
		ObjectReference[] beds = new ObjectReference[3]
		ObjectReference[] seats = new ObjectReference[3]
		i = 0
		While i < kMaxFollowers
			beds[i] = placedRefs[kFirstBedroll + 1 + i]
			seats[i] = placedRefs[kFirstFollowerSeat + i]
			i += 1
		EndWhile
		followers.Assign(placedRefs[kCampfire], placedRefs[kPlayerSeat], beds, seats)
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

	Int i = 0
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

; Checks the ground under the campfire and each tent with the SKSE plugin.
Bool Function IsSiteClear(Float heading, Int tents, Int[] sizes, Float[] tentFwd, Float[] tentSide, Float[] tentTurn)
	Float fireX = OffsetX(heading, FireForward(), 0.0)
	Float fireY = OffsetY(heading, FireForward(), 0.0)
	Int result = CSK_Native.IsAreaClear(PlayerRef, fireX, fireY, heading, CampfireHalfSize, CampfireHalfSize, MaxStep, MaxSlope, SearchUp, SearchDown)

	Int t = 0
	While result == 0 && t < tents
		Float tentX = OffsetX(heading, tentFwd[t], tentSide[t])
		Float tentY = OffsetY(heading, tentFwd[t], tentSide[t])
		result = CSK_Native.IsAreaClear(PlayerRef, tentX, tentY, heading + tentTurn[t], TentHalfWidth(sizes[t]), TentHalfDepth(sizes[t]), MaxStep, MaxSlope, SearchUp, SearchDown)
		t += 1
	EndWhile

	If result == 1
		Debug.Notification("There's no solid ground here to camp on.")
	ElseIf result == 2
		Debug.Notification("The ground here is too steep to camp on.")
	ElseIf result == 3
		Debug.Notification("There isn't enough flat, open ground here.")
	ElseIf result == 4
		Debug.Notification("You can't set up camp in the water.")
	EndIf
	Return result == 0
EndFunction

; Skyrim heading: 0 = north (+Y), increasing clockwise.
; forward = (sin a, cos a), right = (cos a, -sin a)
Float Function OffsetX(Float heading, Float fwd, Float right)
	Return PlayerRef.GetPositionX() + fwd * Math.Sin(heading) + right * Math.Cos(heading)
EndFunction

Float Function OffsetY(Float heading, Float fwd, Float right)
	Return PlayerRef.GetPositionY() + fwd * Math.Cos(heading) - right * Math.Sin(heading)
EndFunction

ObjectReference Function PlaceAt(Form akForm, Float x, Float y, Float z, Float ax, Float ay, Float az)
	If !akForm
		Return None
	EndIf

	ObjectReference ref = PlayerRef.PlaceAtMe(akForm, 1, False, True)
	ref.SetPosition(x, y, z)
	ref.SetAngle(ax, ay, az)
	ref.Enable()
	Return ref
EndFunction
