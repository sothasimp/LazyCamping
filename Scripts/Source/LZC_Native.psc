Scriptname LZC_Native Hidden
{Native functions provided by LazyCamping.dll (SKSE), which the mod requires.
If the DLL is not loaded these are unbound and GetVersion() returns 0.

Camp layouts are offsets from an origin (the player): fwd units in front
along heading and side units to the right, with turn added to heading for
each object's facing. Angles are in degrees.}

; Returns the plugin version, or 0 if LazyCamping.dll is not loaded.
Int Function GetVersion() Global Native

; Checks the ground under the first count footprints (rectangles with the
; given half-sizes along their right and forward axes).
; Returns 0 = clear, 1 = no ground, 2 = too steep, 3 = uneven or obstructed,
; 4 = underwater, 5 = bad arguments.
Int Function CheckCampSite(ObjectReference origin, Float heading, Int count, Float[] fwd, Float[] side, Float[] turn, Float[] halfWidth, Float[] halfDepth, Float maxStep, Float maxSlope) Global Native

; Places every form in the layout on the ground (raised by lift), tilted to
; the slope if alignToSlope (by at most maxTilt degrees), all in one frame.
;
; Dressing: a layout object with dressSet[i] = s + 1 also gets a copy of
; dressing set s around it. Set s is setAnchors[s] (a marker standing in for
; the camp object) plus the setItems whose itemSet entry is s + 1 (0 =
; unused); each item's offset and rotation relative to the anchor carry over,
; and items sit on the ground at their own spot, raised by their height above
; the anchor. The first item found that is one of storageMarkers isn't copied
; as clutter: a copy of it marks where the player's storage goes.
;
; Returns the layout's references (None where the form is None), then the
; storage spot (or None), then the dressing; or an empty array if the arrays
; don't line up.
ObjectReference[] Function PlaceCamp(ObjectReference origin, Float heading, Form[] forms, Float[] fwd, Float[] side, Float[] lift, Float[] turn, Bool alignToSlope, Float maxTilt, Int[] dressSet, ObjectReference[] setAnchors, ObjectReference[] setItems, Int[] itemSet, ObjectReference[] storageMarkers) Global Native

; Hides the grass in a circle just big enough to hold all the references (by
; their object bounds), replacing any earlier camp's. Grass that
; loads later around them (coming back to the camp, loading a save) is hidden
; too, until ShowGrass. Works with No Grass In Objects' grass cache.
Function HideGrass(ObjectReference[] refs) Global Native

; Brings back the grass hidden by HideGrass.
Function ShowGrass() Global Native

; Renames a form (a spell, say), or gives it its own name back if name is
; empty. Unlike SKSE's Form.SetName, the name is kept in the save.
Function RenameForm(Form akForm, String name) Global Native
