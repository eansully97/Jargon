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
	 * Deprecated legacy payload. New traps resolve effects from UJargonTileEffectDefinition.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Trap", meta = (AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Effects.", ToolTip = "Deprecated legacy trap effects. New traps resolve the placed tile-effect definition."))
	TArray<FJargonEffectSpec> TriggeredEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Trap", meta = (AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.bDestroyAfterUnitEnter.", ToolTip = "Deprecated legacy destroy flag. New traps use the placed tile-effect definition."))
	bool bTriggerOnce = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Deprecated|Trap", meta = (ClampMin = "0", AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Effects.", ToolTip = "Deprecated legacy fallback damage. New traps use the placed tile-effect definition."))
	int32 DefaultTrapDamage = 1;
};
