// JargonGameInstance.cpp

#include "Core/JargonGameInstance.h"

UJargonGameInstance::UJargonGameInstance()
{
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;
	PendingEncounterData.Reset();
	bReturningFromCombat = false;
}

void UJargonGameInstance::StartEncounter(
	const FName& InEncounterId,
	const FName& InSourceMapName,
	const FTransform& InSourceTransform
)
{
	// Legacy milestone-1 path.
	// We keep this so the old trigger-based loop still works until the real
	// exploration enemy actor replaces it.
	PendingEncounterData.Reset();
	PendingEncounterData.EncounterId = InEncounterId;

	ReturnMapName = InSourceMapName;
	ReturnTransform = InSourceTransform;
	bReturningFromCombat = false;
}

void UJargonGameInstance::StartEncounterWithRuntimeData(
	const FPendingEncounterRuntimeData& InPendingEncounter,
	const FName& InSourceMapName,
	const FTransform& InSourceTransform
)
{
	PendingEncounterData.Reset();
	PendingEncounterData.EncounterId = InPendingEncounter.EncounterId;
	PendingEncounterData.CombatMapName = InPendingEncounter.CombatMapName;

	for (const FEncounterEnemySpawn& SpawnEntry : InPendingEncounter.EnemySpawns)
	{
		if (SpawnEntry.IsValid())
		{
			PendingEncounterData.EnemySpawns.Add(SpawnEntry);
		}
	}

	ReturnMapName = InSourceMapName;
	ReturnTransform = InSourceTransform;
	bReturningFromCombat = false;
}

void UJargonGameInstance::MarkEncounterCleared(const FName& EncounterId)
{
	if (!EncounterId.IsNone())
	{
		ClearedEncounterIds.Add(EncounterId);
	}
}

bool UJargonGameInstance::IsEncounterCleared(const FName& EncounterId) const
{
	if (EncounterId.IsNone())
	{
		return false;
	}

	return ClearedEncounterIds.Contains(EncounterId);
}

void UJargonGameInstance::PrepareReturnToExploration()
{
	bReturningFromCombat = true;
}

void UJargonGameInstance::CompleteReturnToExploration()
{
	bReturningFromCombat = false;
	ClearPendingEncounter();
}

void UJargonGameInstance::ClearPendingEncounter()
{
	PendingEncounterData.Reset();
}