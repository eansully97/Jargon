#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/Interactions/JargonInteractionTypes.h"
#include "JargonInteractionPromptWidget.generated.h"

class UTextBlock;

/**
 * Shared prompt widget for Town and Exploration interactables.
 *
 * Blueprint widget setup can be minimal:
 * - Optional TextBlock named PromptText
 * - Optional TextBlock named VerbText
 *
 * The player controller owns this widget and updates it from
 * FJargonInteractionPromptData, so individual actors do not need their own UI.
 */
UCLASS(BlueprintType, Blueprintable)
class JARGON_API UJargonInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction|Prompt")
	void ApplyPromptData(const FJargonInteractionPromptData& InPromptData);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Prompt")
	void ClearPromptData();

	UFUNCTION(BlueprintPure, Category = "Interaction|Prompt")
	const FJargonInteractionPromptData& GetPromptData() const
	{
		return PromptData;
	}

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VerbText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Prompt")
	FJargonInteractionPromptData PromptData;

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Prompt")
	void BP_OnPromptDataApplied(const FJargonInteractionPromptData& InPromptData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Prompt")
	void BP_OnPromptDataCleared();
};
