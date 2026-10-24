Scriptname CSK_TentActivatorScript extends ObjectReference
{Attached to the placed tent Activator. Activating the tent offers to pack up.}

CSK_CampsiteQuestScript Property CampQuest Auto

Event OnActivate(ObjectReference akActionRef)
	If akActionRef == CampQuest.PlayerRef
		CampQuest.PromptPackUp(False)
	EndIf
EndEvent
