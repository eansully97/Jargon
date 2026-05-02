#include "Exploration/Interactables/ExplorationRelicRewardInteractable.h"

#include "Core/JargonGameInstance.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonRelicDefinition.h"
#include "Exploration/JargonExplorationPlayerController.h"

AExplorationRelicRewardInteractable::AExplorationRelicRewardInteractable()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Claim Hero Boon"));
	InteractionVerbText = FText::FromString(TEXT("Claim Hero Boon"));
}

bool AExplorationRelicRewardInteractable::GrantReward(AJargonExplorationPlayerController* InteractingController)
{
	if (!RelicDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' has no Hero Boon definition configured."), *GetName());
		return false;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' could not find UJargonGameInstance."), *GetName());
		return false;
	}

	if (GameInstance->HasRunBoon(RelicDefinition.Get()))
	{
		UE_LOG(LogTemp, Log, TEXT("ExplorationRelicRewardInteractable '%s' found Hero Boon '%s' already in the run; completing reward anyway."),
			*GetName(),
			*GetNameSafe(RelicDefinition.Get()));
		BP_OnHeroBoonGranted(InteractingController, RelicDefinition.Get());
		BP_OnRelicGranted(InteractingController, RelicDefinition.Get());
		return true;
	}

	UJargonHeroDefinition* ActiveHeroDefinition = GameInstance->GetActiveHeroDefinition();
	if (!RelicDefinition->IsEligibleForHeroDefinition(ActiveHeroDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' rejected Hero Boon '%s' for active hero '%s'. Check EligibleHeroClasses and EligibleHeroAspects on the boon definition."),
			*GetName(),
			*GetNameSafe(RelicDefinition.Get()),
			*GetNameSafe(ActiveHeroDefinition));
		return false;
	}

	if (!GameInstance->AddRunBoon(RelicDefinition.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationRelicRewardInteractable '%s' failed to add Hero Boon '%s'."),
			*GetName(),
			*GetNameSafe(RelicDefinition.Get()));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ExplorationRelicRewardInteractable '%s' granted Hero Boon '%s'."),
		*GetName(),
		*GetNameSafe(RelicDefinition.Get()));

	BP_OnHeroBoonGranted(InteractingController, RelicDefinition.Get());
	BP_OnRelicGranted(InteractingController, RelicDefinition.Get());
	return true;
}
