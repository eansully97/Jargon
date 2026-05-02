#pragma once

#include "CoreMinimal.h"
#include "Exploration/Interactables/ExplorationRewardInteractable.h"
#include "ExplorationRelicRewardInteractable.generated.h"

class AJargonExplorationPlayerController;
class UJargonRelicDefinition;

/** Simple one-shot exploration reward that adds a Hero Boon to the current run. */
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

	UFUNCTION(BlueprintImplementableEvent, Category = "Exploration Reward|Hero Boon")
	void BP_OnHeroBoonGranted(AJargonExplorationPlayerController* InteractingController, UJargonRelicDefinition* GrantedBoon);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward|Hero Boon", meta = (DisplayName = "Hero Boon Definition", ToolTip = "Hero Boon added to the current run when this one-shot reward is claimed. The property name is retained for existing relic reward assets."))
	TObjectPtr<UJargonRelicDefinition> RelicDefinition = nullptr;
};
