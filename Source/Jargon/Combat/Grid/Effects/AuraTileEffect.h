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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura")
	EJargonTileEffectOperation AuraOperation = EJargonTileEffectOperation::ApplyShield;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura")
	EJargonTileEffectTargetFilter TargetFilter = EJargonTileEffectTargetFilter::FriendlyToSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura", meta = (ClampMin = "0"))
	int32 DefaultEffectValue = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aura")
	bool bApplyOnPlayerTurnStart = true;
};