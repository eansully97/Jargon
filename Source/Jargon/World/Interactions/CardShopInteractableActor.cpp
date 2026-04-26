#include "World/Interactions/CardShopInteractableActor.h"

#include "Exploration/JargonExplorationPlayerController.h"
#include "Jargon.h"
#include "Town/JargonTownPlayerController.h"

ACardShopInteractableActor::ACardShopInteractableActor()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Open Card Shop"));
	InteractionVerbText = FText::FromString(TEXT("Open Card Shop"));
}

void ACardShopInteractableActor::Interact_Implementation(AJargonExplorationPlayerController* InteractingController)
{
	AJargonTownPlayerController* TownController = Cast<AJargonTownPlayerController>(InteractingController);
	if (!TownController)
	{
		UE_LOG(LogJargon, Warning, TEXT("Card Shop interactable '%s' requires AJargonTownPlayerController."), *GetNameSafe(this));
		return;
	}

	TownController->OpenCardShop();
}
