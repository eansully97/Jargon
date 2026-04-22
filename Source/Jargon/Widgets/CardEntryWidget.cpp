// CardEntryWidget.cpp

#include "Widgets/CardEntryWidget.h"

#include "Data/CardDefinition.h"
#include "Combat/JargonCombatPlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCardEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CardButton)
	{
		CardButton->OnClicked.RemoveDynamic(this, &UCardEntryWidget::HandleCardButtonClicked);
		CardButton->OnClicked.AddDynamic(this, &UCardEntryWidget::HandleCardButtonClicked);
	}

	RefreshDisplay();
}

void UCardEntryWidget::InitializeFromCard(UCardDefinition* InCard)
{
	CardDefinition = InCard;
	RefreshDisplay();
}

void UCardEntryWidget::HandleCardButtonClicked()
{
	if (!CardDefinition)
	{
		return;
	}

	if (AJargonCombatPlayerController* CombatPC = GetOwningPlayer<AJargonCombatPlayerController>())
	{
		CombatPC->SelectCard(CardDefinition);
		return;
	}

	CardClickedDelegate.Broadcast(CardDefinition);
}

void UCardEntryWidget::RefreshDisplay()
{
	if (CardNameText)
	{
		CardNameText->SetText(CardDefinition ? CardDefinition->DisplayName : FText::GetEmpty());
	}

	if (CostText)
	{
		const FText CostValueText = CardDefinition
			? FText::AsNumber(CardDefinition->Cost)
			: FText::GetEmpty();

		CostText->SetText(CostValueText);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(CardDefinition ? CardDefinition->Description : FText::GetEmpty());
	}
}