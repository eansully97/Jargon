#include "Town/JargonTownGameMode.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Town/JargonTownPlayerController.h"

AJargonTownGameMode::AJargonTownGameMode()
{
	PlayerControllerClass = AJargonTownPlayerController::StaticClass();

	// DefaultPawnClass is intentionally left for BP_TownGameMode to configure.
	// This avoids hard references to template pawn assets while we transition
	// toward a Jargon-owned exploration/town character.
}

void AJargonTownGameMode::BeginPlay()
{
	Super::BeginPlay();

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	if (!JargonGI)
	{
		return;
	}

	JargonGI->SetTownMapName(TownMapName);

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
