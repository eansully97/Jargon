#pragma once

#include "CoreMinimal.h"
#include "World/Interactions/JargonInteractableActor.h"
#include "GatewayInteractableActor.generated.h"

UCLASS(Blueprintable)
class JARGON_API AGatewayInteractableActor : public AJargonInteractableActor
{
	GENERATED_BODY()

public:
	AGatewayInteractableActor();

	virtual void Interact_Implementation(AJargonExplorationPlayerController* InteractingController) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gateway")
	FName MapName = NAME_None;
};
