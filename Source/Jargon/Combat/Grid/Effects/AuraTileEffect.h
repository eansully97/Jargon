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
	 * Optional shared-effect aura payloads. If this array is empty, the legacy AuraOperation/TargetFilter behavior is used.
	 * For a shield aura, use ApplyShield + UnitsInRadius + FriendlyToSource and set Radius on the effect spec.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Effects", meta = (ToolTip = "Optional shared effects resolved on player turn start. Empty keeps the legacy aura operation fallback."))
	TArray<FJargonEffectSpec> AuraEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Legacy Fallback", meta = (ToolTip = "Legacy fallback operation used only when AuraEffects is empty. Prefer AuraEffects for new aura authoring."))
	EJargonTileEffectOperation AuraOperation = EJargonTileEffectOperation::ApplyShield;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Legacy Fallback", meta = (ToolTip = "Legacy fallback target filter used only when AuraEffects is empty. Prefer AuraEffects for new aura authoring."))
	EJargonTileEffectTargetFilter TargetFilter = EJargonTileEffectTargetFilter::FriendlyToSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura|Legacy Fallback", meta = (ClampMin = "0", ToolTip = "Legacy fallback amount used only when AuraEffects is empty. Prefer AuraEffects for new aura authoring."))
	int32 DefaultEffectValue = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura", meta = (ToolTip = "If true, this aura resolves AuraEffects or legacy fallback behavior on player turn start."))
	bool bApplyOnPlayerTurnStart = true;
};
