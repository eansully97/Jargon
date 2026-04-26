#include "World/Interactions/GatewayInteractableActor.h"

#include "Exploration/JargonExplorationPlayerController.h"
#include "Jargon.h"
#include "Kismet/GameplayStatics.h"

AGatewayInteractableActor::AGatewayInteractableActor()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Travel"));
	InteractionVerbText = FText::FromString(TEXT("Travel"));
}

void AGatewayInteractableActor::Interact_Implementation(AJargonExplorationPlayerController* InteractingController)
{
	if (MapName.IsNone())
	{
		UE_LOG(LogJargon, Warning, TEXT("Gateway interactable '%s' has no MapName assigned."), *GetNameSafe(this));
		return;
	}

	UGameplayStatics::OpenLevel(this, MapName);
}
