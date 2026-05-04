#pragma once

#include "CoreMinimal.h"
#include "Exploration/Interactables/ExplorationRewardInteractable.h"
#include "ExplorationArtifactRewardInteractable.generated.h"

class AJargonExplorationPlayerController;
class UJargonArtifactDefinition;

/** Simple one-shot exploration reward that adds an Artifact to the current run. */
UCLASS(Blueprintable)
class JARGON_API AExplorationArtifactRewardInteractable : public AExplorationRewardInteractable
{
	GENERATED_BODY()

public:
	AExplorationArtifactRewardInteractable();

protected:
	virtual bool GrantReward(AJargonExplorationPlayerController* InteractingController) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Exploration Reward|Artifact")
	void BP_OnArtifactGranted(AJargonExplorationPlayerController* InteractingController, UJargonArtifactDefinition* GrantedArtifact);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward|Artifact", meta = (DisplayName = "Artifact Definition", ToolTip = "Artifact added to the current run when this one-shot reward is claimed."))
	TObjectPtr<UJargonArtifactDefinition> ArtifactDefinition = nullptr;
};
