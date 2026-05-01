#include "Exploration/JargonExplorationGameMode.h"

#include "Core/JargonGameInstance.h"
#include "Data/JargonHeroDefinition.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "JargonCharacter.h"

AJargonExplorationGameMode::AJargonExplorationGameMode()
{
	PlayerControllerClass = AJargonExplorationPlayerController::StaticClass();
	DefaultPawnClass = AJargonCharacter::StaticClass();
}

void AJargonExplorationGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	UJargonHeroDefinition* HeroDefinition = GameInstance ? GameInstance->GetActiveHeroDefinition() : nullptr;

	AJargonCharacter* JargonCharacter = Cast<AJargonCharacter>(NewPlayer->GetPawn());
	if (JargonCharacter && HeroDefinition)
	{
		JargonCharacter->InitializeFromHeroDefinition(HeroDefinition);
	}

	if (!GameInstance || !GameInstance->IsReturningFromCombat())
	{
		return;
	}

	APawn* PlayerPawn = NewPlayer->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	PlayerPawn->SetActorTransform(GameInstance->GetReturnTransform());
	GameInstance->CompleteReturnToExploration();
}
