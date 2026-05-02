#include "Exploration/Encounters/EncounterDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UEncounterDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (CombatMapName.IsNone())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("CombatMapName is None."));
	}

	if (EnemySpawns.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("EnemySpawns is empty."));
	}

	for (int32 SpawnIndex = 0; SpawnIndex < EnemySpawns.Num(); ++SpawnIndex)
	{
		const FEncounterEnemySpawn& Spawn = EnemySpawns[SpawnIndex];
		if (!Spawn.UnitClass)
		{
			JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("EnemySpawns entry %d has no UnitClass."), SpawnIndex));
		}
	}

	if (VictoryCurrencyReward.Gold < 0 || VictoryCurrencyReward.Silver < 0 || VictoryCurrencyReward.Copper < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("VictoryCurrencyReward contains a negative denomination. Gold=%d Silver=%d Copper=%d."), VictoryCurrencyReward.Gold, VictoryCurrencyReward.Silver, VictoryCurrencyReward.Copper));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
