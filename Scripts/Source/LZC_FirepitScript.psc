Scriptname LZC_FirepitScript extends ObjectReference
{On the unlit firepit the camp places when the firewood mechanic is on. Hands
activation to the quest script, which lights the fire if the player is
sneaking and has firewood.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	CampQuest.OnFirepitActivated(Self, akActionRef)
EndEvent
