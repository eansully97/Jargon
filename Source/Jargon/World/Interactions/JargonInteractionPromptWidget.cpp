#include "World/Interactions/JargonInteractionPromptWidget.h"

#include "Components/TextBlock.h"

void UJargonInteractionPromptWidget::ApplyPromptData(const FJargonInteractionPromptData& InPromptData)
{
	PromptData = InPromptData;

	if (PromptText)
	{
		PromptText->SetText(PromptData.PromptText);
	}

	if (VerbText)
	{
		VerbText->SetText(PromptData.VerbText);
	}

	SetVisibility(PromptData.bShouldShowPrompt ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	BP_OnPromptDataApplied(PromptData);
}

void UJargonInteractionPromptWidget::ClearPromptData()
{
	PromptData.Reset();

	if (PromptText)
	{
		PromptText->SetText(FText::GetEmpty());
	}

	if (VerbText)
	{
		VerbText->SetText(FText::GetEmpty());
	}

	SetVisibility(ESlateVisibility::Collapsed);
	BP_OnPromptDataCleared();
}
