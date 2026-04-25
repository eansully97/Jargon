#include "Town/JargonTownGameMode.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"

void AJargonTownGameMode::BeginPlay()
{
	Super::BeginPlay();

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	if (!JargonGI)
	{
		return;
	}

	JargonGI->SetTownMapName(TownMapName);

	TArray<UCardPackDefinition*> PackOffers;
	for (UCardPackDefinition* PackOffer : TownShopPackOffers)
	{
		if (PackOffer)
		{
			PackOffers.Add(PackOffer);
		}
	}

	JargonGI->SetAvailableCardPackOffers(PackOffers);

	if (!JargonGI->HasActiveRun())
	{
		TArray<UCardDefinition*> InitialDeck;
		for (UCardDefinition* Card : StarterDeckDefinitions)
		{
			if (Card)
			{
				InitialDeck.Add(Card);
			}
		}

		FJargonCurrencyAmount StartingCurrency;
		StartingCurrency.Gold = StartingGold;
		StartingCurrency.Silver = StartingSilver;
		StartingCurrency.Copper = StartingCopper;

		JargonGI->StartNewRun(InitialDeck, StartingCurrency);
	}

	JargonGI->CompletePostCombatReturn();
}