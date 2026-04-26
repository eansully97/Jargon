#include "Exploration/JargonExplorationGameMode.h"

#include "Core/JargonGameInstance.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "GameFramework/Pawn.h"
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

AJargonExplorationGameMode::AJargonExplorationGameMode()
{
	PlayerControllerClass = AJargonExplorationPlayerController::StaticClass();

	if (UClass* PreferredWorldPawnClass = ResolvePreferredWorldPawnClass())
	{
		DefaultPawnClass = PreferredWorldPawnClass;
	}
}

void AJargonExplorationGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
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
