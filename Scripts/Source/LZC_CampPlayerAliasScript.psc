Scriptname LZC_CampPlayerAliasScript extends ReferenceAlias
{On the Player alias of LZC_CampFollowersQuest. Tells the quest when the
player sits down at, or gets up from, their seat by the fire.}

Event OnSit(ObjectReference akFurniture)
	(GetOwningQuest() as LZC_CampFollowersScript).OnPlayerSat(akFurniture)
EndEvent

Event OnGetUp(ObjectReference akFurniture)
	(GetOwningQuest() as LZC_CampFollowersScript).OnPlayerGotUp(akFurniture)
EndEvent
