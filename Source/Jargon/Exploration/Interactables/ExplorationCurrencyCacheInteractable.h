#pragma once

#include "CoreMinimal.h"
#include "Core/JargonRunStateTypes.h"
#include "Exploration/Interactables/ExplorationRewardInteractable.h"
#include "ExplorationCurrencyCacheInteractable.generated.h"

class AJargonExplorationPlayerController;

/** Simple one-shot exploration cache that grants run currency. */
UCLASS(Blueprintable)
class JARGON_API AExplorationCurrencyCacheInteractable : public AExplorationRewardInteractable
{
	GENERATED_BODY()

public:
	AExplorationCurrencyCacheInteractable();

protected:
	virtual bool GrantReward(AJargonExplorationPlayerController* InteractingController) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration Reward|Currency")
	FJargonCurrencyAmount Reward;
};
