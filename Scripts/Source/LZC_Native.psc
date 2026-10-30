Scriptname LZC_Native Hidden
{Native functions provided by LazyCamping.dll (SKSE), which the mod requires.
If the DLL is not loaded these are unbound and GetVersion() returns 0.}

; Returns the plugin version, or 0 if LazyCamping.dll is not loaded.
Int Function GetVersion() Global Native

; Sets up a camp in front of the player, all in one go: works out which tents
; and bedrolls go up and where everything stands from campQuest's
; LZC_CampsiteQuestScript properties (partners and couples are the follower
; counts, for sharing out the bedrolls), checks the ground, places it all with
; its clutter, and moves the cook box, storage backpack and map marker to the
; camp, sets up the bedroll seats and hides the grass.
;
; Returns the placed references in the camp's slots (see the script's
; placedRefs), then the clutter; or an empty array if the camp couldn't go up,
; with GetBuildStatus saying why: 1 = no ground, 2 = too steep, 3 = uneven or
; obstructed, 4 = underwater, 5 = something else.
ObjectReference[] Function BuildCamp(Quest campQuest, Int partners, Int couples) Global Native
Int Function GetBuildStatus() Global Native

; What the last BuildCamp laid out, for the follower quest: the fire seats
; (the player's first) and their places in the fan, the bedrolls and the seat
; on each, each bedroll's group (its tent) and order in the row, the seats in
; the tub, and the tents' sizes (0 small, 1 large) and count.
ObjectReference[] Function GetFireSeats() Global Native
Int[] Function GetFirePlaces() Global Native
ObjectReference[] Function GetCampBeds() Global Native
ObjectReference[] Function GetBedSeats() Global Native
Int[] Function GetBedGroups() Global Native
Int[] Function GetBedOrders() Global Native
ObjectReference[] Function GetTubSeats() Global Native
Int[] Function GetTentSizes() Global Native
Int Function GetTentCount() Global Native

; Brings back the grass hidden when the camp went up.
Function ShowGrass() Global Native

; Forgets the camp's bedroll seats (sneak-activating a camp bedroll sits the
; player on its seat until then).
Function ClearBedrollSeats() Global Native

; Renames a form (a spell, say), or gives it its own name back if name is
; empty. Unlike SKSE's Form.SetName, the name is kept in the save.
Function RenameForm(Form akForm, String name) Global Native
