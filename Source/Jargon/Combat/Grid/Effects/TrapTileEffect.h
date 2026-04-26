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
	UPROPERTY(EditDefaultsOnly, Category = "Trap", meta = (ClampMin = "0"))
	int32 DefaultTrapDamage = 1;
};
