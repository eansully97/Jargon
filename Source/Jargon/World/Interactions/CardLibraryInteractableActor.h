#pragma once

#include "CoreMinimal.h"
#include "World/Interactions/JargonInteractableActor.h"
#include "CardLibraryInteractableActor.generated.h"

UCLASS(Blueprintable)
class JARGON_API ACardLibraryInteractableActor : public AJargonInteractableActor
{
	GENERATED_BODY()

public:
	ACardLibraryInteractableActor();

	virtual void Interact_Implementation(AJargonExplorationPlayerController* InteractingController) override;
};
