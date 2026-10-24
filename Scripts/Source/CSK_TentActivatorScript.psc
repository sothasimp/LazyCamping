Scriptname CSK_TentActivatorScript extends ObjectReference
{Attached to every tent Activator (both sizes, all colours). Sneaking and
activating the tent from outside packs up the camp.}

CSK_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	If akActionRef == CampQuest.PlayerRef
		CampQuest.OnTentActivated(Self)
	EndIf
EndEvent
