// EncounterDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Exploration/Encounters/EncounterTypes.h"
#include "Engine/DataAsset.h"
#include "EncounterDefinition.generated.h"

UCLASS(BlueprintType)
class JARGON_API UEncounterDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter")
	FName CombatMapName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter")
	TArray<FEncounterEnemySpawn> EnemySpawns;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter|Rewards")
	FJargonCurrencyAmount VictoryCurrencyReward;

	bool IsValidDefinition() const
	{
		return !CombatMapName.IsNone() && EnemySpawns.Num() > 0;
	}
};
