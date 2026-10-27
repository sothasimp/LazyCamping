Scriptname LZC_CampObjectScript extends ObjectReference
{On every camp object the player can use: the tents, the unlit firepit, the
cook box over the fire and the clothes basket. Hands activation to the quest
script, which works out which object it is.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	CampQuest.OnCampObjectActivated(Self, akActionRef)
EndEvent
