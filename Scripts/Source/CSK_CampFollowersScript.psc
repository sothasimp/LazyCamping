Scriptname CSK_CampFollowersScript extends Quest
{Runs while the camp is set up. The Followers aliases fill with nearby
followers when the quest starts. Each follower has two alias packages:
- sit at their Seats alias while CSK_CampActivity == 1 (the player is
  sitting at the fire)
- sleep in their Bedrolls alias while CSK_CampActivity == 2 (the player is
  sleeping at camp). The bedrolls are handed out when the player goes to
  sleep, skipping whichever camp bedroll the player is lying in.
The rest of the time they follow the player as normal. A follower without a
seat or bedroll has an empty alias, so that package simply doesn't run.}

Actor Property PlayerRef Auto
ReferenceAlias[] Property Followers Auto
{Follower1-3, filled by alias conditions when the quest starts.}
ReferenceAlias[] Property Bedrolls Auto
{Bedroll1-3, filled when the player goes to sleep.}
ReferenceAlias[] Property Seats Auto
{Seat1-3, filled by Assign.}
GlobalVariable Property CSK_CampActivity Auto
{0 = normal, 1 = player sitting at the fire, 2 = player sleeping at camp.}
Float Property CampRadius = 1024.0 Auto
{Sleeping within this distance of the campfire counts as sleeping at camp.}

ObjectReference Property CampAnchor Auto Hidden
ObjectReference Property PlayerSeat Auto Hidden

Float Property kInBedDistance = 128.0 AutoReadOnly
{The player counts as lying in a camp bedroll within this distance of it.}

Int Property kNormal = 0 AutoReadOnly
Int Property kSitting = 1 AutoReadOnly
Int Property kSleeping = 2 AutoReadOnly

ObjectReference[] campBedrolls

Bool Function HasFollower(Int index)
	Return index < Followers.Length && Followers[index].GetReference() != None
EndFunction

; Called by the camp script once everything is placed. allBeds holds every
; camp bedroll; followers get theirs when the player goes to sleep.
Function Assign(ObjectReference akCampfire, ObjectReference akPlayerSeat, ObjectReference[] allBeds, ObjectReference[] seats)
	CampAnchor = akCampfire
	PlayerSeat = akPlayerSeat
	campBedrolls = allBeds
	ClearAll(Bedrolls)
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

; Called by CSK_CampPlayerAliasScript.
Function OnPlayerSat(ObjectReference akFurniture)
	If akFurniture && akFurniture == PlayerSeat && CSK_CampActivity.GetValueInt() != kSleeping
		SetActivity(kSitting)
	EndIf
EndFunction

Function OnPlayerGotUp(ObjectReference akFurniture)
	If akFurniture && akFurniture == PlayerSeat && CSK_CampActivity.GetValueInt() == kSitting
		SetActivity(kNormal)
	EndIf
EndFunction

Event OnSleepStart(Float afSleepStartTime, Float afDesiredSleepEndTime)
	If CampAnchor && PlayerRef.GetDistance(CampAnchor) <= CampRadius
		ForceAll(Bedrolls, FreeBedrolls())
		SetActivity(kSleeping)
	EndIf
EndEvent

; The camp's bedrolls in camp order, minus the one the player is lying in.
ObjectReference[] Function FreeBedrolls()
	ObjectReference playerBed = None
	Float nearest = kInBedDistance
	Int i = 0
	While i < campBedrolls.Length
		If campBedrolls[i] && PlayerRef.GetDistance(campBedrolls[i]) <= nearest
			nearest = PlayerRef.GetDistance(campBedrolls[i])
			playerBed = campBedrolls[i]
		EndIf
		i += 1
	EndWhile

	ObjectReference[] free = new ObjectReference[3]
	Int count = 0
	i = 0
	While i < campBedrolls.Length && count < free.Length
		If campBedrolls[i] && campBedrolls[i] != playerBed
			free[count] = campBedrolls[i]
			count += 1
		EndIf
		i += 1
	EndWhile
	Return free
EndFunction

Event OnSleepStop(Bool abInterrupted)
	If CSK_CampActivity.GetValueInt() == kSleeping
		SetActivity(kNormal)
	EndIf
EndEvent

Function ClearAll(ReferenceAlias[] aliases)
	Int i = 0
	While i < aliases.Length
		aliases[i].Clear()
		i += 1
	EndWhile
EndFunction

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
