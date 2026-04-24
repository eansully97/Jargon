// AuraShieldTileEffect.h

#pragma once

#include "CoreMinimal.h"
#include "Grid/BattleTileEffect.h"
#include "AuraShieldTileEffect.generated.h"

UCLASS(Blueprintable)
class JARGON_API AAuraShieldTileEffect : public ABattleTileEffect
{
	GENERATED_BODY()

public:
	virtual void HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Aura", meta = (ClampMin = "0"))
	int32 DefaultShieldAmount = 1;
};
