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
MiscObject Property CSK_TentKit Auto
MiscObject Property CSK_BedrollKit Auto
MiscObject Property CSK_CampfireKit Auto

MiscObject Property Firewood01 Auto
{Optional. Vanilla firewood, consumed (not refunded) each time a fire is built.}
Int Property FirewoodRequired = 0 Auto
{How much firewood building the fire costs. 0 disables the requirement.}

; ---------------------------------------------------------------------------
; World objects placed when the camp is set up
; ---------------------------------------------------------------------------
Form Property CSK_TentPlaced Auto
{Activator (recommended, so the tent can be used to pack up) or Static.}
Form Property CSK_BedrollPlaced Auto
{Furniture with a sleep marker, so the player can sleep / wait in it.}
Form Property CSK_CampfirePlaced Auto
{Static or Activator: the burning campfire.}
Light Property CSK_CampfireLight Auto
{Optional. Flickering firelight placed above the campfire.}
Form Property CSK_CookingStation Auto
{Optional. Cooking pot / spit furniture placed over the fire.}

; ---------------------------------------------------------------------------
; Layout, in game units relative to the player (fwd = in front, right = to the
; player's right). ~70 units is roughly one metre. Tweak to fit your meshes.
; ---------------------------------------------------------------------------
Float Property CampfireForward = 160.0 Auto
Float Property TentForward = 420.0 Auto
Float Property TentRight = 0.0 Auto
Float Property BedrollForward = 420.0 Auto
Float Property BedrollRight = 0.0 Auto
Float Property LightHeight = 40.0 Auto

; ---------------------------------------------------------------------------
; Followers. Extra bedrolls the player carries are laid out beside theirs.
; ---------------------------------------------------------------------------
Int Property MaxExtraBedrolls = 2 Auto
{Extra bedrolls placed for followers when the player carries more than one
(0-3).}
Float Property BedrollSpacing = 75.0 Auto
{Side-to-side distance between bedrolls in the tent.}
Quest Property CSK_CampFollowersQuest Auto
{Optional. Sends followers to the extra bedrolls while the player sleeps at
camp. Leave empty to only place the bedrolls.}
Float Property CampRadius = 1024.0 Auto
{The player counts as sleeping "at camp" within this distance of their bedroll.}

; ---------------------------------------------------------------------------
; Native placement (CampsiteKit.dll). Ignored if the SKSE plugin isn't loaded.
; ---------------------------------------------------------------------------
Bool Property UseNativePlacement = True Auto
{Use the SKSE plugin to snap the camp to the ground when it is available.}
Bool Property AlignToSlope = True Auto
{Tilt the tent, bedroll and fire to match the ground.}
Float Property MaxTilt = 20.0 Auto
{Objects never tilt more than this many degrees, even on steeper ground.}
Float Property MaxSlope = 25.0 Auto
{Refuse to camp where the ground is steeper than this (degrees).}
Float Property MaxStep = 48.0 Auto
{Refuse to camp if the ground under the tent varies by more than this
(catches rocks, logs and ledges).}
Float Property TentHalfWidth = 110.0 Auto
Float Property TentHalfDepth = 140.0 Auto
{Half the tent's footprint, used for the flat-ground check.}
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
Float Property MaxPackUpDistance = 0.0 Auto
{If > 0, the player must be this close to the camp to pack it up remotely
with the power. 0 = pack up from anywhere (kits are never lost).}

; ---------------------------------------------------------------------------
; Messages
; ---------------------------------------------------------------------------
Message Property CSK_PackUpMsg Auto
{Message box. Button 0 = "Pack up camp", Button 1 = "Leave it".}

; ---------------------------------------------------------------------------
; State
; ---------------------------------------------------------------------------
Bool Property CampDeployed = False Auto Hidden
Int Property ExtraBedrollsPlaced = 0 Auto Hidden

; Slots in placedRefs: 0 campfire, 1 light, 2 cooking station, 3 tent,
; 4 player's bedroll, 5-7 follower bedrolls.
Int Property kFirstExtraBedroll = 5 AutoReadOnly
Int Property kMaxExtraBedrolls = 3 AutoReadOnly

ObjectReference[] placedRefs
Bool busy = False

Event OnInit()
	PlayerRef.AddSpell(CSK_SetUpCampPower, False)
EndEvent

; Called by the power's magic effect.
Function HandlePowerCast()
	If busy
		Return
	EndIf
	busy = True

	If CampDeployed
		PromptPackUp(True)
	ElseIf CanCampHere() && HasAllKits()
		SetUpCamp()
	EndIf

	busy = False
EndFunction

; Called by the power (remote = True) or by activating the tent (remote = False).
Function PromptPackUp(Bool remote = False)
	If !CampDeployed
		Return
	EndIf

	If remote && MaxPackUpDistance > 0.0 && placedRefs[0]
		If PlayerRef.GetDistance(placedRefs[0]) > MaxPackUpDistance
			Debug.Notification("You are too far from your camp to pack it up.")
			Return
		EndIf
	EndIf

	If CSK_PackUpMsg.Show() == 0
		PackUpCamp()
	EndIf
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
	If PlayerRef.GetItemCount(CSK_TentKit) < 1
		missing += CSK_TentKit.GetName() + ", "
	EndIf
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
	; Slot layout (see placedRefs). Index 0, the campfire, is the camp's anchor.
	Form[] forms = new Form[8]
	Float[] fwd = new Float[8]
	Float[] side = new Float[8]
	Float[] lift = new Float[8]
	Float[] turn = new Float[8]

	forms[0] = CSK_CampfirePlaced
	fwd[0] = CampfireForward
	forms[1] = CSK_CampfireLight
	fwd[1] = CampfireForward
	lift[1] = LightHeight
	forms[2] = CSK_CookingStation
	fwd[2] = CampfireForward
	; Tent and bedroll face back toward the fire.
	forms[3] = CSK_TentPlaced
	fwd[3] = TentForward
	side[3] = TentRight
	turn[3] = 180.0

	; Bedrolls sit side by side, centred in the tent. The player's is slot 4;
	; any extra bedrolls the player carries go to followers.
	Int extras = PlayerRef.GetItemCount(CSK_BedrollKit) - 1
	Int maxExtras = MaxExtraBedrolls
	If maxExtras > kMaxExtraBedrolls
		maxExtras = kMaxExtraBedrolls
	EndIf
	If extras > maxExtras
		extras = maxExtras
	EndIf
	If extras < 0
		extras = 0
	EndIf

	Float firstOffset = BedrollRight - extras * BedrollSpacing / 2.0
	Int b = 0
	While b <= extras
		Int slot = 4
		If b > 0
			slot = kFirstExtraBedroll + b - 1
		EndIf
		forms[slot] = CSK_BedrollPlaced
		fwd[slot] = BedrollForward
		side[slot] = firstOffset + b * BedrollSpacing
		turn[slot] = 180.0
		b += 1
	EndWhile

	Float heading = PlayerRef.GetAngleZ()
	Bool native = UseNativePlacement && CSK_Native.GetVersion() > 0

	If native && !IsSiteClear(heading)
		Return
	EndIf

	; Phase 1: work out every position before placing anything, so the
	; bedroll's ground check can't hit the tent standing over it.
	Float[] posX = new Float[8]
	Float[] posY = new Float[8]
	Float[] posZ = new Float[8]
	Float[] angX = new Float[8]
	Float[] angY = new Float[8]
	Float[] angZ = new Float[8]

	Int i = 0
	While i < forms.Length
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
	placedRefs = new ObjectReference[8]
	i = 0
	While i < forms.Length
		placedRefs[i] = PlaceAt(forms[i], posX[i], posY[i], posZ[i], angX[i], angY[i], angZ[i])
		i += 1
	EndWhile

	PlayerRef.RemoveItem(CSK_TentKit, 1, True)
	PlayerRef.RemoveItem(CSK_BedrollKit, 1 + extras, True)
	PlayerRef.RemoveItem(CSK_CampfireKit, 1, True)
	If FirewoodRequired > 0 && Firewood01
		PlayerRef.RemoveItem(Firewood01, FirewoodRequired, True)
	EndIf

	ExtraBedrollsPlaced = extras
	CampDeployed = True
	If extras > 0 && CSK_CampFollowersQuest
		RegisterForSleep()
	EndIf
	Debug.Notification("You set up camp.")
EndFunction

Function PackUpCamp()
	If PlayerRef.GetSitState() != 0
		Debug.Notification("You must get up before packing up camp.")
		Return
	EndIf

	UnregisterForSleep()
	If CSK_CampFollowersQuest && CSK_CampFollowersQuest.IsRunning()
		CSK_CampFollowersQuest.Stop()
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

	PlayerRef.AddItem(CSK_TentKit, 1, True)
	PlayerRef.AddItem(CSK_BedrollKit, 1 + ExtraBedrollsPlaced, True)
	PlayerRef.AddItem(CSK_CampfireKit, 1, True)

	ExtraBedrollsPlaced = 0
	CampDeployed = False
	Debug.Notification("You pack up your camp.")
EndFunction

; Followers bed down in the extra bedrolls while the player sleeps near camp.
Event OnSleepStart(Float afSleepStartTime, Float afDesiredSleepEndTime)
	If !CampDeployed || ExtraBedrollsPlaced < 1 || !CSK_CampFollowersQuest
		Return
	EndIf
	If !placedRefs[4] || PlayerRef.GetDistance(placedRefs[4]) > CampRadius
		Return
	EndIf

	ObjectReference[] beds = new ObjectReference[3]
	Int i = 0
	While i < ExtraBedrollsPlaced
		beds[i] = placedRefs[kFirstExtraBedroll + i]
		i += 1
	EndWhile

	If CSK_CampFollowersQuest.IsRunning() || CSK_CampFollowersQuest.Start()
		(CSK_CampFollowersQuest as CSK_CampFollowersScript).AssignBedrolls(beds)
	EndIf
EndEvent

Event OnSleepStop(Bool abInterrupted)
	If CSK_CampFollowersQuest && CSK_CampFollowersQuest.IsRunning()
		CSK_CampFollowersQuest.Stop()
	EndIf
EndEvent

; Checks the ground under the campfire and tent with the SKSE plugin.
Bool Function IsSiteClear(Float heading)
	Float fireX = OffsetX(heading, CampfireForward, 0.0)
	Float fireY = OffsetY(heading, CampfireForward, 0.0)
	Int result = CSK_Native.IsAreaClear(PlayerRef, fireX, fireY, heading, CampfireHalfSize, CampfireHalfSize, MaxStep, MaxSlope, SearchUp, SearchDown)

	If result == 0 && CSK_TentPlaced
		Float tentX = OffsetX(heading, TentForward, TentRight)
		Float tentY = OffsetY(heading, TentForward, TentRight)
		result = CSK_Native.IsAreaClear(PlayerRef, tentX, tentY, heading + 180.0, TentHalfWidth, TentHalfDepth, MaxStep, MaxSlope, SearchUp, SearchDown)
	EndIf

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
