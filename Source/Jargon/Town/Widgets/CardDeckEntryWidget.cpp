#include "CardDeckEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Data/CardDefinition.h"

void UCardDeckEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CardButton)
	{
		CardButton->OnClicked.RemoveAll(this);
		CardButton->OnClicked.AddDynamic(this, &UCardDeckEntryWidget::HandleCardButtonClicked);
	}
}

void UCardDeckEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshVisuals();
}

void UCardDeckEntryWidget::InitializeFromStackedDeckEntry(UCardDefinition* InCardDefinition, int32 InDeckCount)
{
	CardDefinition = InCardDefinition;
	DeckCount = FMath::Max(0, InDeckCount);

	RefreshVisuals();
}

void UCardDeckEntryWidget::RefreshVisuals()
{
	if (CardNameText)
	{
		CardNameText->SetText(CardDefinition ? CardDefinition->DisplayName : FText::FromString(TEXT("Card")));
	}

	if (DeckCountText)
	{
		DeckCountText->SetText(FText::Format(NSLOCTEXT("DeckEntry", "DeckCountFormat", "x{0}"), FText::AsNumber(DeckCount)));
	}

	if (CardButton)
	{
		CardButton->SetIsEnabled(CardDefinition != nullptr && DeckCount > 0);
	}

	BP_OnDeckEntryRefreshed();
}

void UCardDeckEntryWidget::HandleCardButtonClicked()
{
	if (!CardDefinition)
	{
		return;
	}

	OnDeckEntryClicked.Broadcast(CardDefinition);
	BP_OnDeckEntryClicked();
}