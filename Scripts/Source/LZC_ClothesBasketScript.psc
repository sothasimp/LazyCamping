Scriptname LZC_ClothesBasketScript extends ObjectReference
{On LZC_ClothesBasket, for the tub's clutter set. Activating it takes off
everything the player is wearing, to bathe; activating it again puts the same
things back on.}

Actor Property PlayerRef Auto

Form[] taken      ; what the basket took off, until it's put back on
Bool undressed = False

Event OnActivate(ObjectReference akActionRef)
	If akActionRef != PlayerRef
		Return
	EndIf
	If undressed
		Dress()
	Else
		Undress()
	EndIf
EndEvent

; Takes off whatever is worn in every armour slot (30-61), remembering each
; piece once even if it covers several slots.
Function Undress()
	taken = new Form[32]
	Int count = 0
	Int slot = 0
	While slot < 32
		Form item = PlayerRef.GetWornForm(Math.LeftShift(1, slot))
		If item && taken.Find(item) < 0
			taken[count] = item
			count += 1
		EndIf
		slot += 1
	EndWhile
	Int i = 0
	While i < count
		PlayerRef.UnequipItem(taken[i], False, True)
		i += 1
	EndWhile
	undressed = True
EndFunction

; Puts back on what Undress took off, if the player still has it.
Function Dress()
	Int i = 0
	While i < taken.Length
		If taken[i] && PlayerRef.GetItemCount(taken[i]) > 0
			PlayerRef.EquipItem(taken[i], False, True)
		EndIf
		i += 1
	EndWhile
	undressed = False
EndFunction
