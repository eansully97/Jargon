#pragma once

#include "CoreMinimal.h"
#include "World/Interactions/JargonInteractableActor.h"
#include "ExplorationRewardInteractable.generated.h"

class AJargonExplorationPlayerController;

/**
 * One-shot exploration reward interactable.
 *
 * Completion persists for the current run through UJargonGameInstance, similar
 * to cleared exploration encounters. Set a stable CompletionId for authored
 * map rewards so they stay gone after leaving and returning to Exploration.
 */
UCLASS(Abstract, Blueprintable)
class JARGON_API AExplorationRewardInteractable : public AJargonInteractableActor
{
	GENERATED_BODY()

public:
	AExplorationRewardInteractable();

	virtual void Interact_Implementation(AJargonExplorationPlayerController* InteractingController) override;

	UFUNCTION(BlueprintPure, Category = "Exploration Reward")
	FName GetResolvedCompletionId() const;

	UFUNCTION(BlueprintPure, Category = "Exploration Reward")
	bool IsRewardCompleted() const;

protected:
	virtual void BeginPlay() override;
	virtual void HandlePlayerEnteredRange(AJargonExplorationPlayerController* InteractingController) override;

	virtual bool GrantReward(AJargonExplorationPlayerController* InteractingController);
	bool TryClaimReward(AJargonExplorationPlayerController* InteractingController);

	void MarkCompletedAndDisable(AJargonExplorationPlayerController* InteractingController);
	void ApplyCompletedState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Exploration Reward")
	void BP_OnRewardClaimed(AJargonExplorationPlayerController* InteractingController);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward", meta = (ToolTip = "Stable ID used to persist this reward as completed for the current run. If left empty, the actor name is used as a fallback."))
	FName CompletionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward")
	bool bDestroyWhenCompleted = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward", meta = (ToolTip = "If true, walking into range immediately claims the reward. If false, the player can still press the normal interact input."))
	bool bClaimOnOverlap = true;
};
