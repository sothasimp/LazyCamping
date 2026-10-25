Scriptname LZC_CampFollowersScript extends Quest
{Runs while the camp is set up. The Followers aliases fill with nearby
followers when the quest starts. Each follower has two alias packages:
- sit at their Seats alias while LZC_CampActivity == 1 (the player is
  sitting at the fire)
- sleep in their Bedrolls alias while LZC_CampActivity == 2 (the player is
  sleeping at camp). The bedrolls are handed out when the player goes to
  sleep, skipping whichever camp bedroll the player is lying in. A spouse or
  lover gets the free bedroll next to the player's, and each follower couple
  two free bedrolls side by side.
The rest of the time they follow the player as normal. A follower without a
seat or bedroll has an empty alias, so that package simply doesn't run.}

Actor Property PlayerRef Auto
ReferenceAlias[] Property Followers Auto
{Follower1-3, filled by alias conditions when the quest starts.}
ReferenceAlias[] Property Bedrolls Auto
{Bedroll1-3, filled when the player goes to sleep.}
ReferenceAlias[] Property Seats Auto
{Seat1-3, filled by Assign.}
GlobalVariable Property LZC_CampActivity Auto
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

; Every camp bedroll, with where it is: its group (the tent it's in, or 0
; without tents) and its place in the row (neighbours differ by 1). Partners and
; couples are matched from these, without measuring distances.
ObjectReference[] campBedrolls
Int[] bedGroup
Int[] bedOrder

Bool Function HasFollower(Int index)
	Return index < Followers.Length && Followers[index].GetReference() != None
EndFunction

; Called by the camp script once everything is placed. allBeds holds every
; camp bedroll (groups/orders say where each one is); followers get theirs
; when the player goes to sleep.
Function Assign(ObjectReference akCampfire, ObjectReference akPlayerSeat, ObjectReference[] allBeds, Int[] groups, Int[] orders, ObjectReference[] seats)
	CampAnchor = akCampfire
	PlayerSeat = akPlayerSeat
	campBedrolls = allBeds
	bedGroup = groups
	bedOrder = orders
	ClearAll(Bedrolls)
	ForceAll(Seats, seats)
	RegisterForSleep()
	SetActivity(kNormal)
EndFunction

; Called by the camp script when the camp is packed up.
Function Shutdown()
	UnregisterForSleep()
	LZC_CampActivity.SetValueInt(kNormal)

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
	LZC_CampActivity.SetValueInt(activity)
	Int i = 0
	While i < Followers.Length
		Actor follower = Followers[i].GetActorRef()
		If follower
			follower.EvaluatePackage()
		EndIf
		i += 1
	EndWhile
EndFunction

; Called by LZC_CampPlayerAliasScript.
Function OnPlayerSat(ObjectReference akFurniture)
	If akFurniture && akFurniture == PlayerSeat && LZC_CampActivity.GetValueInt() != kSleeping
		SetActivity(kSitting)
	EndIf
EndFunction

Function OnPlayerGotUp(ObjectReference akFurniture)
	If akFurniture && akFurniture == PlayerSeat && LZC_CampActivity.GetValueInt() == kSitting
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
; choose first, taking the free bedrolls next to the player's; then each
; follower couple takes two free bedrolls side by side; everyone else takes
; the rest in camp order, so followers spread across the tents.
Function AssignBedrolls()
	Bool[] taken = new Bool[12]
	Int i = 0
	While i < taken.Length
		taken[i] = i >= campBedrolls.Length || !campBedrolls[i]
		i += 1
	EndWhile

	Int playerBed = BedrollUnderPlayer()
	If playerBed >= 0
		taken[playerBed] = True
	EndIf

	Int[] chosen = new Int[3]
	i = 0
	While i < chosen.Length
		chosen[i] = -1
		i += 1
	EndWhile

	; The player's partners take the free bedrolls next to the player's.
	If playerBed >= 0
		i = 0
		While i < Followers.Length && i < chosen.Length
			Actor follower = Followers[i].GetActorRef()
			If follower && IsPartner(follower)
				chosen[i] = NearestFree(taken, playerBed)
				If chosen[i] >= 0
					taken[chosen[i]] = True
				EndIf
			EndIf
			i += 1
		EndWhile
	EndIf

	; Each follower couple takes two free bedrolls side by side, normally in
	; the tent set aside for them.
	Int[] mates = Mates()
	i = 0
	While i < mates.Length
		Int j = mates[i]
		If j > i
			Int first = FreePairStart(taken)
			If first >= 0
				chosen[i] = first
				taken[first] = True
				chosen[j] = NearestFree(taken, first)
				If chosen[j] >= 0
					taken[chosen[j]] = True
				EndIf
			EndIf
		EndIf
		i += 1
	EndWhile

	; Everyone else takes the rest in camp order.
	Int next = 0
	i = 0
	While i < Followers.Length && i < chosen.Length
		If chosen[i] < 0 && Followers[i].GetReference()
			While next < taken.Length && taken[next]
				next += 1
			EndWhile
			If next < taken.Length
				chosen[i] = next
				taken[next] = True
			EndIf
		EndIf
		i += 1
	EndWhile

	i = 0
	While i < Bedrolls.Length
		If i < chosen.Length && chosen[i] >= 0
			Bedrolls[i].ForceRefTo(campBedrolls[chosen[i]])
		Else
			Bedrolls[i].Clear()
		EndIf
		i += 1
	EndWhile
EndFunction

; Index of the camp bedroll the player is lying in, or -1.
Int Function BedrollUnderPlayer()
	Int playerBed = -1
	Float nearest = kInBedDistance
	Int i = 0
	While i < campBedrolls.Length
		If campBedrolls[i]
			Float distance = PlayerRef.GetDistance(campBedrolls[i])
			If distance <= nearest
				nearest = distance
				playerBed = i
			EndIf
		EndIf
		i += 1
	EndWhile
	Return playerBed
EndFunction

; Index of the free bedroll closest to bedroll `target` by position: in the
; same group, with the nearest order. Falls back to any free bedroll, or -1.
Int Function NearestFree(Bool[] taken, Int target)
	Int best = -1
	Int bestGap = 0
	Int i = 0
	While i < taken.Length
		If !taken[i]
			Int gap = 1000 + i   ; another group: only if nothing in this one
			If bedGroup[i] == bedGroup[target]
				gap = OrderGap(bedOrder[i], bedOrder[target])
			EndIf
			If best < 0 || gap < bestGap
				best = i
				bestGap = gap
			EndIf
		EndIf
		i += 1
	EndWhile
	Return best
EndFunction

; Index of a free bedroll that has a free neighbour in its group, or of any
; free bedroll if none does, or -1.
Int Function FreePairStart(Bool[] taken)
	Int anyFree = -1
	Int i = 0
	While i < taken.Length
		If !taken[i]
			If anyFree < 0
				anyFree = i
			EndIf
			Int j = 0
			While j < taken.Length
				If j != i && !taken[j] && bedGroup[j] == bedGroup[i] && OrderGap(bedOrder[i], bedOrder[j]) == 1
					Return i
				EndIf
				j += 1
			EndWhile
		EndIf
		i += 1
	EndWhile
	Return anyFree
EndFunction

; How many places apart two bedrolls are within a group.
Int Function OrderGap(Int a, Int b)
	If a > b
		Return a - b
	EndIf
	Return b - a
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
