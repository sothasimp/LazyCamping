Scriptname CSK_Native Hidden
{Native functions provided by CampsiteKit.dll (SKSE). If the DLL is not
loaded these are unbound: GetVersion() returns 0 and the quest script falls
back to plain Papyrus placement.}

; Returns the plugin version, or 0 if CampsiteKit.dll is not loaded.
Int Function GetVersion() Global Native

; Finds the ground below (x, y), searching from origin Z + searchUp down to
; origin Z - searchDown.
; Returns [groundZ, angleX, angleY, angleZ, slopeDegrees], or an empty array
; if no ground was found. With alignToSlope the angles stand the object on the
; surface (tilt limited to maxTilt degrees); otherwise they are (0, 0, yaw).
Float[] Function FindPlacement(ObjectReference origin, Float x, Float y, Float yaw, Bool alignToSlope, Float maxTilt, Float searchUp, Float searchDown) Global Native

; Checks a rectangular footprint centred on (x, y) and facing yaw.
; Returns 0 = clear, 1 = no ground, 2 = too steep, 3 = uneven or obstructed,
; 4 = underwater.
Int Function IsAreaClear(ObjectReference origin, Float x, Float y, Float yaw, Float halfWidth, Float halfDepth, Float maxStep, Float maxSlope, Float searchUp, Float searchDown) Global Native
