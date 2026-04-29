#pragma once

#include "CoreMinimal.h"
#include "Exploration/Interactables/ExplorationRewardInteractable.h"
#include "ExplorationRelicRewardInteractable.generated.h"

class AJargonExplorationPlayerController;
class UJargonRelicDefinition;

/** Simple one-shot exploration reward that adds a relic to the current run. */
UCLASS(Blueprintable)
class JARGON_API AExplorationRelicRewardInteractable : public AExplorationRewardInteractable
{
	GENERATED_BODY()

public:
	AExplorationRelicRewardInteractable();

protected:
	virtual bool GrantReward(AJargonExplorationPlayerController* InteractingController) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Exploration Reward|Relic")
	void BP_OnRelicGranted(AJargonExplorationPlayerController* InteractingController, UJargonRelicDefinition* GrantedRelic);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward|Relic", meta = (ToolTip = "Relic added to the current run when this one-shot reward is claimed."))
	TObjectPtr<UJargonRelicDefinition> RelicDefinition = nullptr;
};
