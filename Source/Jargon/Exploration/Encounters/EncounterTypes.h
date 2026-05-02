// EncounterTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Units/BattleUnit.h"
#include "Core/JargonTypes.h"
#include "Core/JargonRunStateTypes.h"
#include "EncounterTypes.generated.h"

USTRUCT(BlueprintType)
struct JARGON_API FEncounterEnemySpawn
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter", meta = (ToolTip = "Enemy unit Blueprint class spawned for this encounter entry. Data-driven enemy definitions should replace direct classes in a later migration."))
	TSubclassOf<ABattleUnit> UnitClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter", meta = (ToolTip = "Combat board coordinate where this enemy should spawn."))
	FHexCoord SpawnCoord;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Rewards")
	FJargonCurrencyAmount VictoryCurrencyReward;

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
		VictoryCurrencyReward = FJargonCurrencyAmount();
	}
};
