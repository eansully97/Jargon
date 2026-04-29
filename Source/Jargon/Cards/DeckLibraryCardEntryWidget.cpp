#include "DeckLibraryCardEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Cards/CardDisplayWidget.h"

void UDeckLibraryCardEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CardButton)
	{
		CardButton->OnClicked.RemoveAll(this);
		CardButton->OnClicked.AddDynamic(this, &UDeckLibraryCardEntryWidget::HandleCardButtonClicked);
	}
}

void UDeckLibraryCardEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshVisuals();
}

void UDeckLibraryCardEntryWidget::InitializeFromCardLibraryEntry(
	UCardDefinition* InCardDefinition,
	int32 InOwnedCount,
	int32 InDeckCount,
	int32 InAvailableCount,
	bool bInCanAddToDeck,
	bool bInCanRemoveFromDeck)
{
	CardDefinition = InCardDefinition;
	OwnedCount = FMath::Max(0, InOwnedCount);
	DeckCount = FMath::Max(0, InDeckCount);
	AvailableCount = FMath::Max(0, InAvailableCount);
	bCanAddToDeck = bInCanAddToDeck;
	bCanRemoveFromDeck = bInCanRemoveFromDeck;

	RefreshVisuals();
}

bool UDeckLibraryCardEntryWidget::CanAddDisplayedCardToDeck() const
{
	return CardDefinition != nullptr && bCanAddToDeck;
}

void UDeckLibraryCardEntryWidget::RefreshVisuals()
{
	const bool bCanAddDisplayedCard = CanAddDisplayedCardToDeck();

	if (CardDisplay)
	{
		CardDisplay->SetCardDefinition(CardDefinition);
	}

	if (OwnedCountText)
	{
		OwnedCountText->SetText(FText::Format(NSLOCTEXT("DeckLibrary", "OwnedCountFormat", "Owned: {0}"), FText::AsNumber(OwnedCount)));
	}

	if (DeckCountText)
	{
		DeckCountText->SetText(FText::Format(NSLOCTEXT("DeckLibrary", "DeckCountFormat", "Deck: {0}"), FText::AsNumber(DeckCount)));
	}

	if (AvailableCountText)
	{
		AvailableCountText->SetText(FText::Format(NSLOCTEXT("DeckLibrary", "AvailableCountFormat", "Available: {0}"), FText::AsNumber(AvailableCount)));
	}

	if (UnavailableOverlay)
	{
		UnavailableOverlay->SetVisibility(bCanAddDisplayedCard ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (CardButton)
	{
		CardButton->SetIsEnabled(bCanAddDisplayedCard);
	}

	BP_OnLibraryCardEntryRefreshed();
}

void UDeckLibraryCardEntryWidget::HandleCardButtonClicked()
{
	if (!CanAddDisplayedCardToDeck())
	{
		return;
	}

	OnLibraryCardClicked.Broadcast(CardDefinition);
	BP_OnLibraryCardClicked();
}
