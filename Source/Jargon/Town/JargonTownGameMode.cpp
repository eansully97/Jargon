#include "Town/JargonTownGameMode.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Data/JargonDeckDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "JargonCharacter.h"
#include "Town/JargonTownPlayerController.h"

AJargonTownGameMode::AJargonTownGameMode()
{
	PlayerControllerClass = AJargonTownPlayerController::StaticClass();
	DefaultPawnClass = AJargonCharacter::StaticClass();
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
	JargonGI->EnsureActiveHeroDefinition(DefaultHeroDefinition);

	if (!JargonGI->HasActiveRun())
	{
		TArray<UCardDefinition*> InitialDeck;
		if (!StarterDeckDefinition)
		{
			UE_LOG(LogTemp, Error, TEXT("TownGameMode cannot start a new run because StarterDeckDefinition is not assigned."));
			return;
		}

		InitialDeck = StarterDeckDefinition->GetResolvedCards();
		if (!StarterDeckDefinition->IsValidDefinition())
		{
			UE_LOG(LogTemp, Warning, TEXT("Town starter deck definition '%s' failed validation. Runtime will preserve resolved non-null cards."),
				*GetNameSafe(StarterDeckDefinition));
		}

		if (InitialDeck.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("TownGameMode cannot start a new run because StarterDeckDefinition '%s' resolved no cards."),
				*GetNameSafe(StarterDeckDefinition));
			return;
		}

		FJargonCurrencyAmount StartingCurrency;
		StartingCurrency.Gold = StartingGold;
		StartingCurrency.Silver = StartingSilver;
		StartingCurrency.Copper = StartingCopper;

		JargonGI->StartNewRun(InitialDeck, StartingCurrency);
	}

	JargonGI->CompletePostCombatReturn();
}

void AJargonTownGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	UJargonHeroDefinition* HeroDefinition = JargonGI
		? JargonGI->EnsureActiveHeroDefinition(DefaultHeroDefinition)
		: DefaultHeroDefinition.Get();

	AJargonCharacter* JargonCharacter = Cast<AJargonCharacter>(NewPlayer->GetPawn());
	if (JargonCharacter && HeroDefinition)
	{
		JargonCharacter->InitializeFromHeroDefinition(HeroDefinition);
	}
}
