#include "Exploration/Interactables/ExplorationArtifactRewardInteractable.h"

#include "Core/JargonGameInstance.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonArtifactDefinition.h"
#include "Exploration/JargonExplorationPlayerController.h"

AExplorationArtifactRewardInteractable::AExplorationArtifactRewardInteractable()
{
	InteractionPromptText = FText::FromString(TEXT("Press E to Claim Artifact"));
	InteractionVerbText = FText::FromString(TEXT("Claim Artifact"));
}

bool AExplorationArtifactRewardInteractable::GrantReward(AJargonExplorationPlayerController* InteractingController)
{
	if (!ArtifactDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationArtifactRewardInteractable '%s' has no Artifact definition configured."), *GetName());
		return false;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationArtifactRewardInteractable '%s' could not find UJargonGameInstance."), *GetName());
		return false;
	}

	if (GameInstance->HasRunArtifact(ArtifactDefinition.Get()))
	{
		UE_LOG(LogTemp, Log, TEXT("ExplorationArtifactRewardInteractable '%s' found Artifact '%s' already in the run; completing reward anyway."),
			*GetName(),
			*GetNameSafe(ArtifactDefinition.Get()));
		BP_OnArtifactGranted(InteractingController, ArtifactDefinition.Get());
		return true;
	}

	UJargonHeroDefinition* ActiveHeroDefinition = GameInstance->GetActiveHeroDefinition();
	if (!ArtifactDefinition->IsEligibleForHeroDefinition(ActiveHeroDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationArtifactRewardInteractable '%s' rejected Artifact '%s' for active hero '%s'. Check EligibleHeroClasses on the Artifact definition."),
			*GetName(),
			*GetNameSafe(ArtifactDefinition.Get()),
			*GetNameSafe(ActiveHeroDefinition));
		return false;
	}

	if (!GameInstance->AddRunArtifact(ArtifactDefinition.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationArtifactRewardInteractable '%s' failed to add Artifact '%s'."),
			*GetName(),
			*GetNameSafe(ArtifactDefinition.Get()));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ExplorationArtifactRewardInteractable '%s' granted Artifact '%s'."),
		*GetName(),
		*GetNameSafe(ArtifactDefinition.Get()));

	BP_OnArtifactGranted(InteractingController, ArtifactDefinition.Get());
	return true;
}
