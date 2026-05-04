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
};
