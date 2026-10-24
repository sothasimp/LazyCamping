Scriptname CSK_CampFollowersScript extends Quest
{Runs while the camp is set up. The Followers aliases fill with nearby
followers when the quest starts. Each follower has two alias packages:
- sit at their Seats alias while CSK_CampActivity == 1 (the player is
  sitting at the fire)
- sleep in their Bedrolls alias while CSK_CampActivity == 2 (the player is
  sleeping at camp). The bedrolls are handed out when the player goes to
  sleep, skipping whichever camp bedroll the player is lying in. A spouse or
  lover gets the free bedroll nearest the player's, and each follower couple
  the two free bedrolls closest together.
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
Faction Property PartnerFaction Auto
{Optional, such as PlayerMarriedFaction. Followers in it count as the player's
partner, as well as any follower whose relationship rank with the player is
Lover (4), which covers vanilla spouses and most romance mods.}

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
		AssignBedrolls()
		SetActivity(kSleeping)
	EndIf
EndEvent

; Gives each follower a camp bedroll the player isn't lying in. Partners
; choose first, taking the free bedrolls nearest the player's; everyone
; else takes the rest in camp order, so followers spread across the tents.
Function AssignBedrolls()
	ObjectReference playerBed = BedrollUnderPlayer()

	ObjectReference[] free = new ObjectReference[16]
	Int i = 0
	While i < campBedrolls.Length && i < free.Length
		If campBedrolls[i] != playerBed
			free[i] = campBedrolls[i]
		EndIf
		i += 1
	EndWhile

	ObjectReference[] chosen = new ObjectReference[3]

	; The player's partners take the free bedrolls nearest the player's.
	If playerBed
		i = 0
		While i < Followers.Length && i < chosen.Length
			Actor follower = Followers[i].GetActorRef()
			If follower && IsPartner(follower)
				Int k = NearestFree(free, playerBed)
				If k >= 0
					chosen[i] = free[k]
					free[k] = None
				EndIf
			EndIf
			i += 1
		EndWhile
	EndIf

	; Each follower couple takes the two free bedrolls closest together:
	; normally side by side in the tent set aside for them.
	Int[] mates = Mates()
	i = 0
	While i < mates.Length
		Int j = mates[i]
		If j > i && !chosen[i] && !chosen[j]
			Int first = -1
			Int second = -1
			Float closest = 0.0
			Int a = 0
			While a < free.Length
				If free[a]
					Int b = a + 1
					While b < free.Length
						If free[b]
							Float gap = free[a].GetDistance(free[b])
							If first < 0 || gap < closest
								first = a
								second = b
								closest = gap
							EndIf
						EndIf
						b += 1
					EndWhile
				EndIf
				a += 1
			EndWhile
			If first >= 0
				chosen[i] = free[first]
				chosen[j] = free[second]
				free[first] = None
				free[second] = None
			EndIf
		EndIf
		i += 1
	EndWhile

	; Everyone else takes the rest in camp order.
	Int next = 0
	i = 0
	While i < Followers.Length && i < chosen.Length
		If !chosen[i] && Followers[i].GetReference()
			While next < free.Length && !free[next]
				next += 1
			EndWhile
			If next < free.Length
				chosen[i] = free[next]
				next += 1
			EndIf
		EndIf
		i += 1
	EndWhile

	ForceAll(Bedrolls, chosen)
EndFunction

; The camp bedroll the player is lying in, or None.
ObjectReference Function BedrollUnderPlayer()
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
	Return playerBed
EndFunction

; Index of the free bedroll nearest akRef, or -1 if none are free.
Int Function NearestFree(ObjectReference[] free, ObjectReference akRef)
	Int best = -1
	Float bestDistance = 0.0
	Int i = 0
	While i < free.Length
		If free[i]
			Float distance = free[i].GetDistance(akRef)
			If best < 0 || distance < bestDistance
				best = i
				bestDistance = distance
			EndIf
		EndIf
		i += 1
	EndWhile
	Return best
EndFunction

Int Function PartnerCount()
	Int count = 0
	Int i = 0
	While i < Followers.Length
		Actor follower = Followers[i].GetActorRef()
		If follower && IsPartner(follower)
			count += 1
		EndIf
		i += 1
	EndWhile
	Return count
EndFunction

Int Function CoupleCount()
	Int[] mates = Mates()
	Int count = 0
	Int i = 0
	While i < mates.Length
		If mates[i] > i
			count += 1
		EndIf
		i += 1
	EndWhile
	Return count
EndFunction

; For each Followers index, the index of the follower they're a couple with
; (relationship rank Lover with each other), or -1. The player's partners
; are paired with the player instead, and each follower with at most one.
Int[] Function Mates()
	Int[] mates = new Int[3]
	Int i = 0
	While i < mates.Length
		mates[i] = -1
		i += 1
	EndWhile

	i = 0
	While i < Followers.Length && i < mates.Length
		Actor first = Followers[i].GetActorRef()
		If first && mates[i] < 0 && !IsPartner(first)
			Int j = i + 1
			While j < Followers.Length && j < mates.Length && mates[i] < 0
				Actor second = Followers[j].GetActorRef()
				If second && mates[j] < 0 && !IsPartner(second) && first.GetRelationshipRank(second) >= 4
					mates[i] = j
					mates[j] = i
				EndIf
				j += 1
			EndWhile
		EndIf
		i += 1
	EndWhile
	Return mates
EndFunction

Bool Function IsPartner(Actor akActor)
	If PartnerFaction && akActor.IsInFaction(PartnerFaction)
		Return True
	EndIf
	Return akActor.GetRelationshipRank(PlayerRef) >= 4
EndFunction

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
