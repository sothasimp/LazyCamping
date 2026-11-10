Scriptname CampCampfire extends ObjectReference
{COMPILE-TIME STUB ONLY - never compile this file or ship it. It stands in for
Campfire's real CampCampfire so LZC_CampfireSkillsProxy compiles without
Campfire's whole chain of dependencies. It declares only the members the addon
uses, with the same names and types; in game, Campfire's real script runs.}

GlobalVariable Property _Camp_LastSelectedSkill Auto
Int Property campfire_stage = 0 Auto Hidden
ObjectReference Property myPerkNodeController Auto Hidden
ObjectReference Property myPerkNavController Auto Hidden

Function EnterTreeSystem(Int idx)
EndFunction

Function TakeDown()
EndFunction
