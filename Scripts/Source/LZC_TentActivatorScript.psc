Scriptname LZC_TentActivatorScript extends ObjectReference
{Attached to every tent Activator (both sizes, all colours). Sneaking and
activating the tent from outside offers to pack up the camp.}

LZC_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	If akActionRef == CampQuest.PlayerRef
		CampQuest.OnTentActivated(Self)
	EndIf
EndEvent
