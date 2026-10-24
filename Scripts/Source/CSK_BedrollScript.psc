Scriptname CSK_BedrollScript extends ObjectReference
{Attached to the camp bedroll furniture. In a camp without a tent, sneaking
and activating a bedroll packs up the camp.}

CSK_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	CampQuest.OnBedrollActivated(Self, akActionRef)
EndEvent
