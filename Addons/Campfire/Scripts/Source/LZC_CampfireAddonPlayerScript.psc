Scriptname LZC_CampfireAddonPlayerScript extends ReferenceAlias
{Wayfarer - Campfire Addon. On the player alias of the addon's start-game-enabled
quest: sneak-activating Wayfarer's camp fire opens Campfire's skill trees
there, through an invisible stand-in (LZC_CampfireSkillsProxy).}

Activator Property LZC_CampfireSkillsProxyActivator Auto
{The addon's invisible activator carrying LZC_CampfireSkillsProxy.}

LZC_CampfireSkillsProxy proxy

Event OnInit()
	Register()
EndEvent

Event OnPlayerLoadGame()
	Register()
EndEvent

Function Register()
	RegisterForModEvent("Wayfarer_FireSneakActivated", "OnFireSneakActivated")
	RegisterForModEvent("Wayfarer_CampPackedUp", "OnCampPackedUp")
	; Sneak-activating the fire opens the trees rather than cooking.
	LZC_CampsiteQuestScript camp = LZC_CampsiteQuestScript.Camp()
	If camp
		camp.FireSneakHandled = True
	EndIf
EndFunction

Event OnFireSneakActivated(Form akCampCentre)
	ObjectReference fire = akCampCentre as ObjectReference
	If !fire || !LZC_CampfireSkillsProxyActivator
		Return
	EndIf
	RemoveProxy()
	proxy = fire.PlaceAtMe(LZC_CampfireSkillsProxyActivator) as LZC_CampfireSkillsProxy
	If proxy
		proxy.OpenTrees()
	EndIf
EndEvent

Event OnCampPackedUp(Form akCampCentre, Int aiFollowers)
	RemoveProxy()
EndEvent

Function RemoveProxy()
	If proxy
		proxy.TakeDown()
		proxy = None
	EndIf
EndFunction
