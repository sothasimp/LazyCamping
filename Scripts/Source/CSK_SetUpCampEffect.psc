Scriptname CSK_SetUpCampEffect extends ActiveMagicEffect
{Attached to the Set Up Camp magic effect. Hands off to the quest script,
which owns all campsite state.}

CSK_CampsiteQuestScript Property CampQuest Auto

Event OnEffectStart(Actor akTarget, Actor akCaster)
	If akCaster == CampQuest.PlayerRef
		CampQuest.HandlePowerCast()
	EndIf
EndEvent
