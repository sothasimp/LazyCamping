Scriptname CSK_CampPlayerAliasScript extends ReferenceAlias
{On the Player alias of CSK_CampFollowersQuest. Tells the quest when the
player sits in, or gets up from, any furniture; the quest reacts to their own
seat and bedroll.}

Event OnSit(ObjectReference akFurniture)
	(GetOwningQuest() as CSK_CampFollowersScript).OnPlayerSat(akFurniture)
EndEvent

Event OnGetUp(ObjectReference akFurniture)
	(GetOwningQuest() as CSK_CampFollowersScript).OnPlayerGotUp(akFurniture)
EndEvent
