#include "DeckLibraryCardEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Cards/CardDisplayWidget.h"
#include "Town/JargonTownPlayerController.h"
#include "Town/Widgets/DeckEditWidget.h"

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

void UDeckLibraryCardEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	HandleCardHovered();
}

void UDeckLibraryCardEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	HandleCardUnhovered();
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

UDeckEditWidget* UDeckLibraryCardEntryWidget::ResolveOwningDeckEditWidget() const
{
	if (UDeckEditWidget* DeckEditWidget = GetTypedOuter<UDeckEditWidget>())
	{
		return DeckEditWidget;
	}

	if (AJargonTownPlayerController* TownController = GetOwningPlayer<AJargonTownPlayerController>())
	{
		return TownController->GetDeckEditWidget();
	}

	return nullptr;
}

bool UDeckLibraryCardEntryWidget::CanAddDisplayedCardToDeck() const
{
	return CardDefinition != nullptr && bCanAddToDeck;
}

bool UDeckLibraryCardEntryWidget::CanExecuteLibraryCardClick() const
{
	return !bIsClickBlocked && CanAddDisplayedCardToDeck();
}

void UDeckLibraryCardEntryWidget::RefreshVisuals()
{
	const bool bCanAddDisplayedCard = CanAddDisplayedCardToDeck();
	bIsClickBlocked = !bCanAddDisplayedCard;

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

	BP_OnLibraryCardEntryRefreshed();
}

void UDeckLibraryCardEntryWidget::HandleCardButtonClicked()
{
	if (!CanExecuteLibraryCardClick())
	{
		BP_OnLibraryCardClickBlocked();
		return;
	}

	OnLibraryCardClicked.Broadcast(CardDefinition);
	BP_OnLibraryCardClicked();
}

void UDeckLibraryCardEntryWidget::HandleCardHovered()
{
	if (UDeckEditWidget* DeckEditWidget = ResolveOwningDeckEditWidget())
	{
		DeckEditWidget->ShowLibraryCardHoverInfo(CardDefinition, this);
	}

	BP_OnLibraryCardHovered();
}

void UDeckLibraryCardEntryWidget::HandleCardUnhovered()
{
	if (UDeckEditWidget* DeckEditWidget = ResolveOwningDeckEditWidget())
	{
		DeckEditWidget->ClearLibraryCardHoverInfo(this);
	}

	BP_OnLibraryCardUnhovered();
}
