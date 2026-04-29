// SpikeTrapTileEffect.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "TrapTileEffect.generated.h"

UCLASS(Blueprintable)
class JARGON_API ATrapTileEffect : public ABattleTileEffect
{
	GENERATED_BODY()

public:
	virtual void HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit) override;

protected:
	/**
	 * Optional shared-effect trap payloads. If this array is empty, the legacy DefaultTrapDamage behavior is used.
	 * For a spike trap, use DealDamage + ExplicitUnit + EnemyToSource.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effects", meta = (ToolTip = "Optional shared effects resolved when a unit enters this trap. Empty keeps the legacy trap damage fallback."))
	TArray<FJargonEffectSpec> TriggeredEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effects")
	bool bTriggerOnce = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0"))
	int32 DefaultTrapDamage = 1;
};
