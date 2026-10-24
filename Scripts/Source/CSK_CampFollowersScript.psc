Scriptname CSK_CampFollowersScript extends Quest
{Runs while the player sleeps at camp. The Followers aliases fill with nearby
followers when the quest starts. Each one's sleep package targets the
matching Bedrolls alias, so followers left without a bedroll carry on as
normal.}

ReferenceAlias[] Property Followers Auto
{Follower1-3, filled by alias conditions when the quest starts.}
ReferenceAlias[] Property Bedrolls Auto
{Bedroll1-3, filled by AssignBedrolls.}

Function AssignBedrolls(ObjectReference[] beds)
	Int i = 0
	While i < Bedrolls.Length
		If i < beds.Length && beds[i]
			Bedrolls[i].ForceRefTo(beds[i])
		Else
			Bedrolls[i].Clear()
		EndIf
		i += 1
	EndWhile

	; Pick up the sleep packages now that their bedrolls exist.
	i = 0
	While i < Followers.Length
		Actor follower = Followers[i].GetActorRef()
		If follower
			follower.EvaluatePackage()
		EndIf
		i += 1
	EndWhile
EndFunction
