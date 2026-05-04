#include "CardEntryWidget.h"

#include "Cards/CardDisplayWidget.h"
#include "Components/Button.h"

void UCardEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CardButton)
	{
		CardButton->OnClicked.RemoveAll(this);
		CardButton->OnClicked.AddDynamic(this, &UCardEntryWidget::HandleCardButtonClicked);
	}
}

void UCardEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (CardDisplay)
	{
		CardDisplay->SetCardDefinition(CardDefinition);
	}
}

void UCardEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	OnCardEntryHovered.Broadcast(this);
	BP_OnCardHovered();
}

void UCardEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	OnCardEntryUnhovered.Broadcast(this);
	BP_OnCardUnhovered();
}

void UCardEntryWidget::InitializeFromCard(UCardDefinition* InCard)
{
	CardDefinition = InCard;

	if (CardDisplay)
	{
		CardDisplay->SetCardDefinition(CardDefinition);
	}

	BP_OnCardInitialized();
}

void UCardEntryWidget::HandleCardButtonClicked()
{
	if (!CardDefinition)
	{
		return;
	}

	OnCardEntryClicked.Broadcast(CardDefinition);
	BP_OnCardClickFeedback();
}
