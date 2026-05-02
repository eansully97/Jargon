#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JargonTownGameMode.generated.h"

class UCardPackDefinition;
class UJargonDeckDefinition;
class UJargonHeroDefinition;

UCLASS()
class JARGON_API AJargonTownGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJargonTownGameMode();

	const TArray<TObjectPtr<UCardPackDefinition>>& GetTownShopPackOffers() const
	{
		return TownShopPackOffers;
	}

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup", meta = (ToolTip = "Preferred starter deck Data Asset used when beginning a new run in town."))
	TObjectPtr<UJargonDeckDefinition> StarterDeckDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingGold = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingSilver = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	int32 StartingCopper = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Run Setup")
	TObjectPtr<UJargonHeroDefinition> DefaultHeroDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Town")
	FName TownMapName = TEXT("L_TownMap");

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TArray<TObjectPtr<UCardPackDefinition>> TownShopPackOffers;

protected:
	virtual void BeginPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
