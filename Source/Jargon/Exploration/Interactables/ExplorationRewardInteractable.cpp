#include "Exploration/Interactables/ExplorationRewardInteractable.h"

#include "Components/BoxComponent.h"
#include "Core/JargonGameInstance.h"
#include "Exploration/JargonExplorationPlayerController.h"

AExplorationRewardInteractable::AExplorationRewardInteractable()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Open"));
	InteractionVerbText = FText::FromString(TEXT("Open"));
	bDestroyWhenCompleted = true;
	bClaimOnOverlap = true;
}

void AExplorationRewardInteractable::BeginPlay()
{
	Super::BeginPlay();

	if (CompletionId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRewardInteractable '%s' has no CompletionId. Falling back to actor name for current-run persistence."), *GetName());
	}

	if (IsRewardCompleted())
	{
		ApplyCompletedState();
	}
}

void AExplorationRewardInteractable::Interact_Implementation(AJargonExplorationPlayerController* InteractingController)
{
	TryClaimReward(InteractingController);
}

void AExplorationRewardInteractable::HandlePlayerEnteredRange(AJargonExplorationPlayerController* InteractingController)
{
	Super::HandlePlayerEnteredRange(InteractingController);

	if (bClaimOnOverlap)
	{
		TryClaimReward(InteractingController);
	}
}

bool AExplorationRewardInteractable::TryClaimReward(AJargonExplorationPlayerController* InteractingController)
{
	if (IsRewardCompleted())
	{
		ApplyCompletedState();
		return false;
	}

	if (!GrantReward(InteractingController))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRewardInteractable '%s' did not grant a reward and will remain available."), *GetName());
		return false;
	}

	BP_OnRewardClaimed(InteractingController);
	MarkCompletedAndDisable(InteractingController);
	return true;
}

FName AExplorationRewardInteractable::GetResolvedCompletionId() const
{
	return CompletionId.IsNone() ? GetFName() : CompletionId;
}

bool AExplorationRewardInteractable::IsRewardCompleted() const
{
	const UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	return GameInstance && GameInstance->IsExplorationInteractionCompleted(GetResolvedCompletionId());
}

bool AExplorationRewardInteractable::GrantReward(AJargonExplorationPlayerController* InteractingController)
{
	return false;
}

void AExplorationRewardInteractable::MarkCompletedAndDisable(AJargonExplorationPlayerController* InteractingController)
{
	if (UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>())
	{
		GameInstance->MarkExplorationInteractionCompleted(GetResolvedCompletionId());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRewardInteractable '%s' could not persist completion because UJargonGameInstance was unavailable."), *GetName());
	}

	if (InteractingController)
	{
		NotifyInteractionPromptHidden(InteractingController);
	}

	ApplyCompletedState();
}

void AExplorationRewardInteractable::ApplyCompletedState()
{
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	if (InteractionBox)
	{
		InteractionBox->SetGenerateOverlapEvents(false);
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (bDestroyWhenCompleted)
	{
		Destroy();
	}
}
