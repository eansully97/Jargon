#include "Exploration/Interactables/ExplorationCurrencyCacheInteractable.h"

#include "Core/JargonGameInstance.h"
#include "Exploration/JargonExplorationPlayerController.h"

AExplorationCurrencyCacheInteractable::AExplorationCurrencyCacheInteractable()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Open Cache"));
	InteractionVerbText = FText::FromString(TEXT("Open Cache"));
	Reward.Copper = 5;
}

bool AExplorationCurrencyCacheInteractable::GrantReward(AJargonExplorationPlayerController* InteractingController)
{
	if (Reward.IsZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationCurrencyCacheInteractable '%s' has no currency reward configured."), *GetName());
		return false;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationCurrencyCacheInteractable '%s' could not find UJargonGameInstance."), *GetName());
		return false;
	}

	GameInstance->AddCurrency(Reward);

	UE_LOG(LogTemp, Log, TEXT("ExplorationCurrencyCacheInteractable '%s' granted %d copper total."),
		*GetName(),
		Reward.GetTotalCopperValue());

	return true;
}
