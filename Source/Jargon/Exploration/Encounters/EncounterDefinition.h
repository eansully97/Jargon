// EncounterDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Exploration/Encounters/EncounterTypes.h"
#include "Engine/DataAsset.h"
#include "EncounterDefinition.generated.h"

UCLASS(BlueprintType)
class JARGON_API UEncounterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter", meta = (ToolTip = "Combat map package/name used for this encounter. Leave empty only for placeholder assets."))
	FName CombatMapName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter", meta = (TitleProperty = "UnitClass", ToolTip = "Enemy spawn definitions for this encounter."))
	TArray<FEncounterEnemySpawn> EnemySpawns;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter|Rewards", meta = (ToolTip = "Currency granted when this encounter is won."))
	FJargonCurrencyAmount VictoryCurrencyReward;

	bool IsValidDefinition() const
	{
		return !CombatMapName.IsNone() && EnemySpawns.Num() > 0;
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
