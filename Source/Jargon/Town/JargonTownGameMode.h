#pragma once

#include "CoreMinimal.h"
#include "Data/CardDefinition.h"
#include "GameFramework/GameModeBase.h"
#include "JargonTownGameMode.generated.h"

UCLASS()
class JARGON_API AJargonTownGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	TArray<TObjectPtr<UCardDefinition>> StarterDeckDefinitions;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingGold = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingSilver = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingCopper = 0;

protected:
	virtual void BeginPlay() override;
};