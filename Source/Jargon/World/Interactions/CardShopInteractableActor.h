#pragma once

#include "CoreMinimal.h"
#include "World/Interactions/JargonInteractableActor.h"
#include "CardShopInteractableActor.generated.h"

UCLASS(Blueprintable)
class JARGON_API ACardShopInteractableActor : public AJargonInteractableActor
{
	GENERATED_BODY()

public:
	ACardShopInteractableActor();

	virtual void Interact_Implementation(AJargonExplorationPlayerController* InteractingController) override;
};
