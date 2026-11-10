Scriptname LZC_CampfireAddonPlayerScript extends ReferenceAlias
{Wayfarer - Campfire Addon. On the player alias of the addon's start-game-enabled
quest: sneak-activating Wayfarer's camp fire opens Campfire's skill trees
there, through an invisible stand-in (LZC_CampfireSkillsProxy).}

Activator Property LZC_CampfireSkillsProxyActivator Auto
{The addon's invisible activator carrying LZC_CampfireSkillsProxy.}

MiscObject Property _Camp_DeadwoodLog Auto
{Campfire's dead wood, which counts as firewood for lighting the camp fire
(Auto-Fill).}

Form[] Property OtherFirewood Auto
{Optional: more items that count as firewood for the camp fire, one for one
(Campfire's _Camp_DeadwoodBranch, other mods' wood).}

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
		; Campfire's dead wood (and any others listed) lights the fire too.
		camp.AddFirewood(_Camp_DeadwoodLog)
		Int i = 0
		While OtherFirewood && i < OtherFirewood.Length
			camp.AddFirewood(OtherFirewood[i])
			i += 1
		EndWhile
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
