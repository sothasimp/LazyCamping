Scriptname CSK_CampFollowersScript extends Quest
{Runs while the camp is set up. The Followers aliases fill with nearby
followers when the quest starts. Each follower has two alias packages:
- sit at their Seats alias while CSK_CampActivity == 1 (the player is
  sitting in their own seat by the fire)
- sleep in their Bedrolls alias while CSK_CampActivity == 2 (the player is
  in their own bedroll)
The rest of the time they follow the player as normal. A follower without a
seat or bedroll has an empty alias, so that package simply doesn't run.}

Actor Property PlayerRef Auto
ReferenceAlias[] Property Followers Auto
{Follower1-3, filled by alias conditions when the quest starts.}
ReferenceAlias[] Property Bedrolls Auto
{Bedroll1-3, filled by Assign.}
ReferenceAlias[] Property Seats Auto
{Seat1-3, filled by Assign.}
GlobalVariable Property CSK_CampActivity Auto
{0 = normal, 1 = player in their seat, 2 = player in their bedroll.}

ObjectReference Property PlayerSeat Auto Hidden
ObjectReference Property PlayerBedroll Auto Hidden

Float Property kInBedDistance = 128.0 AutoReadOnly

Int Property kNormal = 0 AutoReadOnly
Int Property kSitting = 1 AutoReadOnly
Int Property kSleeping = 2 AutoReadOnly

Bool Function HasFollower(Int index)
	Return index < Followers.Length && Followers[index].GetReference() != None
EndFunction

; Called by the camp script once everything is placed.
Function Assign(ObjectReference akPlayerSeat, ObjectReference akPlayerBedroll, ObjectReference[] beds, ObjectReference[] seats)
	PlayerSeat = akPlayerSeat
	PlayerBedroll = akPlayerBedroll
	ForceAll(Bedrolls, beds)
	ForceAll(Seats, seats)
	RegisterForSleep()
	SetActivity(kNormal)
EndFunction

; Called by the camp script when the camp is packed up.
Function Shutdown()
	UnregisterForSleep()
	CSK_CampActivity.SetValueInt(kNormal)

	Actor[] actors = new Actor[3]
	Int i = 0
	While i < Followers.Length && i < actors.Length
		actors[i] = Followers[i].GetActorRef()
		i += 1
	EndWhile

	Stop()

	; With the aliases cleared, send everyone back to following.
	i = 0
	While i < actors.Length
		If actors[i]
			actors[i].EvaluatePackage()
		EndIf
		i += 1
	EndWhile
EndFunction

Function SetActivity(Int activity)
	CSK_CampActivity.SetValueInt(activity)
	Int i = 0
	While i < Followers.Length
		Actor follower = Followers[i].GetActorRef()
		If follower
			follower.EvaluatePackage()
		EndIf
		i += 1
	EndWhile
EndFunction

; Called by CSK_CampPlayerAliasScript. Followers sit when the player sits in
; their own seat, and go to bed when the player gets into their own bedroll.
Function OnPlayerSat(ObjectReference akFurniture)
	If !akFurniture
		Return
	EndIf
	If akFurniture == PlayerBedroll
		SetActivity(kSleeping)
	ElseIf akFurniture == PlayerSeat && CSK_CampActivity.GetValueInt() != kSleeping
		SetActivity(kSitting)
	EndIf
EndFunction

Function OnPlayerGotUp(ObjectReference akFurniture)
	If !akFurniture
		Return
	EndIf
	Int activity = CSK_CampActivity.GetValueInt()
	If (akFurniture == PlayerSeat && activity == kSitting) || (akFurniture == PlayerBedroll && activity == kSleeping)
		SetActivity(kNormal)
	EndIf
EndFunction

; Backup in case getting into a bed doesn't send OnSit: sleeping counts if
; the player is lying in their own bedroll.
Event OnSleepStart(Float afSleepStartTime, Float afDesiredSleepEndTime)
	If PlayerBedroll && PlayerRef.GetDistance(PlayerBedroll) <= kInBedDistance
		SetActivity(kSleeping)
	EndIf
EndEvent

Event OnSleepStop(Bool abInterrupted)
	If CSK_CampActivity.GetValueInt() == kSleeping
		SetActivity(kNormal)
	EndIf
EndEvent

Function ForceAll(ReferenceAlias[] aliases, ObjectReference[] refs)
	Int i = 0
	While i < aliases.Length
		If i < refs.Length && refs[i]
			aliases[i].ForceRefTo(refs[i])
		Else
			aliases[i].Clear()
		EndIf
		i += 1
	EndWhile
EndFunction
