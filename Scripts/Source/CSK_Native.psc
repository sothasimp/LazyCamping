Scriptname CSK_Native Hidden
{Native functions provided by CampsiteKit.dll (SKSE), which the mod requires.
If the DLL is not loaded these are unbound and GetVersion() returns 0.

Camp layouts are offsets from an origin (the player): fwd units in front
along heading and side units to the right, with turn added to heading for
each object's facing. Angles are in degrees.}

; Returns the plugin version, or 0 if CampsiteKit.dll is not loaded.
Int Function GetVersion() Global Native

; Checks the ground under the first count footprints (rectangles with the
; given half-sizes along their right and forward axes).
; Returns 0 = clear, 1 = no ground, 2 = too steep, 3 = uneven or obstructed,
; 4 = underwater, 5 = bad arguments.
Int Function CheckCampSite(ObjectReference origin, Float heading, Int count, Float[] fwd, Float[] side, Float[] turn, Float[] halfWidth, Float[] halfDepth, Float maxStep, Float maxSlope) Global Native

; Places every form in the layout on the ground (raised by lift), tilted to
; the slope if alignToSlope (by at most maxTilt degrees), all in one frame.
; Returns the new references in layout order (None where the form is None),
; or an empty array if the arrays don't line up.
ObjectReference[] Function PlaceCamp(ObjectReference origin, Float heading, Form[] forms, Float[] fwd, Float[] side, Float[] lift, Float[] turn, Bool alignToSlope, Float maxTilt) Global Native
