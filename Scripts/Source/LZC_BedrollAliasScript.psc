Scriptname LZC_BedrollAliasScript extends ReferenceAlias
{On the RingBedroll1-4 aliases of LZC_CampsiteQuest. In a camp without tents,
sneaking and activating a bedroll offers to pack up the camp. Being on an alias rather
than the bedroll's base object lets the camp use the vanilla bedroll, so Base
Object Swapper variants still apply.}

Event OnActivate(ObjectReference akActionRef)
	(GetOwningQuest() as LZC_CampsiteQuestScript).OnBedrollActivated(GetReference(), akActionRef)
EndEvent
