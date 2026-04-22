// EncounterTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Units/BattleUnit.h"
#include "EncounterTypes.generated.h"

USTRUCT(BlueprintType)
struct JARGON_API FEncounterEnemySpawn
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter")
	TSubclassOf<ABattleUnit> UnitClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter")
	FIntPoint SpawnCoord = FIntPoint::ZeroValue;

	bool IsValid() const
	{
		return UnitClass != nullptr;
	}
};

USTRUCT(BlueprintType)
struct JARGON_API FPendingEncounterRuntimeData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName EncounterId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName CombatMapName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<FEncounterEnemySpawn> EnemySpawns;

	bool HasAnyEncounter() const
	{
		return !EncounterId.IsNone();
	}

	bool HasConfiguredCombatEncounter() const
	{
		return !EncounterId.IsNone() && !CombatMapName.IsNone() && EnemySpawns.Num() > 0;
	}

	void Reset()
	{
		EncounterId = NAME_None;
		CombatMapName = NAME_None;
		EnemySpawns.Reset();
	}
};