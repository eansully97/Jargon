#include "World/Interactions/CardLibraryInteractableActor.h"

#include "Exploration/JargonExplorationPlayerController.h"
#include "Jargon.h"
#include "Town/JargonTownPlayerController.h"

ACardLibraryInteractableActor::ACardLibraryInteractableActor()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Open Card Library"));
	InteractionVerbText = FText::FromString(TEXT("Open Card Library"));
}

void ACardLibraryInteractableActor::Interact_Implementation(AJargonExplorationPlayerController* InteractingController)
{
	AJargonTownPlayerController* TownController = Cast<AJargonTownPlayerController>(InteractingController);
	if (!TownController)
	{
		UE_LOG(LogJargon, Warning, TEXT("Card Library interactable '%s' requires AJargonTownPlayerController."), *GetNameSafe(this));
		return;
	}

	TownController->OpenDeckEdit();
}
