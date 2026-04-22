// JargonGameMode.cpp

#include "JargonGameMode.h"

#include "Core/JargonGameInstance.h"
#include "GameFramework/Pawn.h"
#include "JargonCharacter.h"
#include "JargonPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AJargonGameMode::AJargonGameMode()
{
	// Use the top down character Blueprint as the default pawn.
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PlayerControllerClass = AJargonPlayerController::StaticClass();
}

void AJargonGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		return;
	}

	if (!GameInstance->IsReturningFromCombat())
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