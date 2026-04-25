#pragma once

#include "CoreMinimal.h"
#include "Data/CardDefinition.h"
#include "GameFramework/GameModeBase.h"
#include "JargonTownGameMode.generated.h"

class UCardPackDefinition;

UCLASS()
class JARGON_API AJargonTownGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJargonTownGameMode();

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	TArray<TObjectPtr<UCardDefinition>> StarterDeckDefinitions;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingGold = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingSilver = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingCopper = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Town")
	FName TownMapName = TEXT("L_TownMap");

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TArray<TObjectPtr<UCardPackDefinition>> TownShopPackOffers;

protected:
	virtual void BeginPlay() override;
};
