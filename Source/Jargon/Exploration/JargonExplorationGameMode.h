#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JargonExplorationGameMode.generated.h"

UCLASS()
class JARGON_API AJargonExplorationGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJargonExplorationGameMode();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
