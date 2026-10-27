Scriptname LZC_ClothesBasketScript extends ObjectReference
{On LZC_ClothesBasket, for the tub's clutter set. Activating it takes off
everything the player is wearing, to bathe.}

Actor Property PlayerRef Auto

Event OnActivate(ObjectReference akActionRef)
	If akActionRef == PlayerRef
		PlayerRef.UnequipAll()
	EndIf
EndEvent
