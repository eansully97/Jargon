// SpikeTrapTileEffect.h

#pragma once

#include "CoreMinimal.h"
#include "Grid/BattleTileEffect.h"
#include "SpikeTrapTileEffect.generated.h"

UCLASS(Blueprintable)
class JARGON_API ASpikeTrapTileEffect : public ABattleTileEffect
{
	GENERATED_BODY()

public:
	virtual void HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Trap", meta = (ClampMin = "0"))
	int32 DefaultTrapDamage = 1;
};
