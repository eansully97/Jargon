#pragma once

#include "CoreMinimal.h"
#include "JargonInteractionTypes.generated.h"

class AJargonInteractableActor;

USTRUCT(BlueprintType)
struct JARGON_API FJargonInteractionPromptData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AJargonInteractableActor> Interactable = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText PromptText;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText VerbText;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bCanInteract = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bShouldShowPrompt = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bHasWorldLocation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FVector WorldLocation = FVector::ZeroVector;

	void Reset()
	{
		Interactable = nullptr;
		PromptText = FText::GetEmpty();
		VerbText = FText::GetEmpty();
		bCanInteract = false;
		bShouldShowPrompt = false;
		bHasWorldLocation = false;
		WorldLocation = FVector::ZeroVector;
	}
};
