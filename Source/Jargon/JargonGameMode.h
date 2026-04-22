// JargonGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JargonGameMode.generated.h"

UCLASS(minimalapi)
class AJargonGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJargonGameMode();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};