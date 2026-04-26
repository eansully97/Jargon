// EnemyBattleUnit.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonRunStateTypes.h"
#include "Units/BattleUnit.h"
#include "EnemyBattleUnit.generated.h"

UCLASS(Blueprintable)
class JARGON_API AEnemyBattleUnit : public ABattleUnit
{
	GENERATED_BODY()

public:
	AEnemyBattleUnit();

	UFUNCTION(BlueprintPure, Category = "Enemy|Rewards")
	FJargonCurrencyAmount GetKillCurrencyReward() const
	{
		return KillCurrencyReward;
	}

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rewards")
	FJargonCurrencyAmount KillCurrencyReward;
};
