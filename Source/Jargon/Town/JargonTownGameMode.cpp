#include "Town/JargonTownGameMode.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "GameFramework/Pawn.h"
#include "Town/JargonTownPlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UClass* ResolvePreferredWorldPawnClass()
{
	// Intentional prototype bridge: Jargon-owned modes/controllers drive flow,
	// while the working template pawn keeps movement/camera/animation stable.
	static ConstructorHelpers::FClassFinder<APawn> ThirdPersonPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (ThirdPersonPawnBPClass.Class)
	{
		return ThirdPersonPawnBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APawn> TopDownPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	return TopDownPawnBPClass.Class;
}
}

AJargonTownGameMode::AJargonTownGameMode()
{
	PlayerControllerClass = AJargonTownPlayerController::StaticClass();

	if (UClass* PreferredWorldPawnClass = ResolvePreferredWorldPawnClass())
	{
		DefaultPawnClass = PreferredWorldPawnClass;
	}
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
