Scriptname LZC_CampObjectScript extends ObjectReference
{On every camp object the player can use: the tents, the unlit firepit, the
cook box over the fire, the clothes basket and the tub trigger. Hands
activation and the trigger's events to the quest script, which works out which
object it is.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	CampQuest.OnCampObjectActivated(Self, akActionRef)
EndEvent

Event OnTriggerEnter(ObjectReference akActionRef)
	CampQuest.OnCampTrigger(Self, akActionRef, True)
EndEvent

Event OnTriggerLeave(ObjectReference akActionRef)
	CampQuest.OnCampTrigger(Self, akActionRef, False)
EndEvent
