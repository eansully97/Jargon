#include "Exploration/Interactables/ExplorationRelicRewardInteractable.h"

#include "Core/JargonGameInstance.h"
#include "Data/JargonRelicDefinition.h"
#include "Exploration/JargonExplorationPlayerController.h"

AExplorationRelicRewardInteractable::AExplorationRelicRewardInteractable()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Claim Relic"));
	InteractionVerbText = FText::FromString(TEXT("Claim Relic"));
}

bool AExplorationRelicRewardInteractable::GrantReward(AJargonExplorationPlayerController* InteractingController)
{
	if (!RelicDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' has no RelicDefinition configured."), *GetName());
		return false;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' could not find UJargonGameInstance."), *GetName());
		return false;
	}

	if (GameInstance->HasRunRelic(RelicDefinition.Get()))
	{
		UE_LOG(LogTemp, Log, TEXT("ExplorationRelicRewardInteractable '%s' found relic '%s' already in the run; completing reward anyway."),
			*GetName(),
			*GetNameSafe(RelicDefinition.Get()));
		BP_OnRelicGranted(InteractingController, RelicDefinition.Get());
		return true;
	}

	if (!GameInstance->AddRunRelic(RelicDefinition.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' failed to add relic '%s'."),
			*GetName(),
			*GetNameSafe(RelicDefinition.Get()));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ExplorationRelicRewardInteractable '%s' granted relic '%s'."),
		*GetName(),
		*GetNameSafe(RelicDefinition.Get()));

	BP_OnRelicGranted(InteractingController, RelicDefinition.Get());
	return true;
}
