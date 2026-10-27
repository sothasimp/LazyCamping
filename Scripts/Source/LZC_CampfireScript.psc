Scriptname LZC_CampfireScript extends ObjectReference
{On the camp's campfire (LZC_Campfire). Hands activation to the quest script,
which opens the Cook menu.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	CampQuest.OnCampfireActivated(Self, akActionRef)
EndEvent
