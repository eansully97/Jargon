#pragma once

#include "CoreMinimal.h"
#include "BattleTileEffect.h"
#include "AuraTileEffect.generated.h"

UCLASS(Blueprintable)
class JARGON_API AAuraTileEffect : public ABattleTileEffect
{
	GENERATED_BODY()

public:
	virtual void HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode) override;

protected:
	/**
	 * Deprecated legacy payload. New auras resolve effects from UJargonTileEffectDefinition.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Aura", meta = (AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Effects.", ToolTip = "Deprecated legacy aura effects. New auras resolve the placed tile-effect definition."))
	TArray<FJargonEffectSpec> AuraEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Aura", meta = (AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Effects.", ToolTip = "Deprecated legacy fallback operation. New auras use the placed tile-effect definition."))
	EJargonTileEffectOperation AuraOperation = EJargonTileEffectOperation::ApplyShield;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Aura", meta = (AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Effects target filters.", ToolTip = "Deprecated legacy fallback target filter. New auras use effect target filters."))
	EJargonTileEffectTargetFilter TargetFilter = EJargonTileEffectTargetFilter::FriendlyToSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Aura", meta = (ClampMin = "0", AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Effects.", ToolTip = "Deprecated legacy fallback amount. New auras use the placed tile-effect definition."))
	int32 DefaultEffectValue = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deprecated|Aura", meta = (AdvancedDisplay, DeprecatedProperty, DeprecationMessage = "Use UJargonTileEffectDefinition.Trigger.", ToolTip = "Deprecated legacy enable flag. New auras use the placed tile-effect definition trigger."))
	bool bApplyOnPlayerTurnStart = true;
};
