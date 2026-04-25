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

	JargonGI->SetTownMapName(TEXT("L_TownMap"));

	if (JargonGI->HasActiveRun())
	{
		return;
	}

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