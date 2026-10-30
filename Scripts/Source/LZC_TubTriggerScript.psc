Scriptname LZC_TubTriggerScript extends ObjectReference
{On the tub's trigger box (LZC_TubTrigger). Tells the camp quest when a
follower steps into or out of the tub, so they undress and dress again.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnTriggerEnter(ObjectReference akActionRef)
	CampQuest.OnCampTrigger(Self, akActionRef, True)
EndEvent

Event OnTriggerLeave(ObjectReference akActionRef)
	CampQuest.OnCampTrigger(Self, akActionRef, False)
EndEvent
