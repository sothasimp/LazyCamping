Scriptname LZC_CampfireScript extends ObjectReference
{On LZC_CookActivator, the base of the invisible activator box that sits over
the camp's fire. Hands activation to the quest script, which opens the Cook
menu.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	CampQuest.OnCampfireActivated(Self, akActionRef)
EndEvent
