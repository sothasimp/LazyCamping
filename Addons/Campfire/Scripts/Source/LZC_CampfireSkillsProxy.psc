Scriptname LZC_CampfireSkillsProxy extends CampCampfire
{Wayfarer - Campfire Addon. An invisible stand-in for a Campfire campfire, placed at
Wayfarer's camp fire so Campfire's skill trees can be shown there: Campfire's
trees only work around a CampCampfire. It places none of a real campfire's
pieces (no flames, fuel, seats or light) and counts as lit.

On the addon's activator: fill the properties with the Creation Kit's
Auto-Fill All (they're Campfire's own forms).}

; None of a real campfire's behaviour: no setup, burning, sitting or menus.
Event OnInit()
EndEvent

Event OnUpdate()
EndEvent

Event OnActivate(ObjectReference akActionRef)
EndEvent

Event OnCellAttach()
EndEvent

Event OnCellDetach()
EndEvent

Event OnHit(ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
EndEvent

Event OnMagicEffectApply(ObjectReference akCaster, MagicEffect akEffect)
EndEvent

Event OnUpdateGameTime()
EndEvent

; Shows the skill tree last looked at, around this stand-in.
Function OpenTrees()
	campfire_stage = 2
	EnterTreeSystem(_Camp_LastSelectedSkill.GetValueInt())
EndFunction

; Closes the trees and removes the stand-in.
Function TakeDown()
	If myPerkNodeController
		(myPerkNodeController as CampPerkNodeController).TakeDown()
	EndIf
	If myPerkNavController
		(myPerkNavController as _Camp_PerkNavController).TakeDown()
	EndIf
	myPerkNodeController = None
	myPerkNavController = None
	Disable()
	Delete()
EndFunction
