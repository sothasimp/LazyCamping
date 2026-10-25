Scriptname LZC_CampPlayerAliasScript extends ReferenceAlias
{On the Player alias of LZC_CampFollowersQuest. Tells the quest when the
player sits down, so followers can take seats too. Followers stay seated when
the player gets up.}

Event OnSit(ObjectReference akFurniture)
	(GetOwningQuest() as LZC_CampFollowersScript).OnPlayerSat(akFurniture)
EndEvent
