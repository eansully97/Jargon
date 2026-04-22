// CombatHUDWidget.cpp

#include "Widgets/CombatHUDWidget.h"

#include "Data/CardDefinition.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/CardEntryWidget.h"

void UCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshSelectedCardText(nullptr);
}

void UCombatHUDWidget::RefreshHand(const TArray<TObjectPtr<UCardDefinition>>& HandCards)
{
	SpawnedCardWidgets.Reset();

	if (!HandContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatHUDWidget '%s' has no HandContainer bound."), *GetName());
		return;
	}

	HandContainer->ClearChildren();

	if (!CardEntryWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatHUDWidget '%s' has no CardEntryWidgetClass assigned."), *GetName());
		return;
	}

	APlayerController* OwningPlayerController = GetOwningPlayer();

	for (UCardDefinition* Card : HandCards)
	{
		if (!Card)
		{
			continue;
		}

		UCardEntryWidget* CardWidget = nullptr;

		if (OwningPlayerController)
		{
			CardWidget = CreateWidget<UCardEntryWidget>(OwningPlayerController, CardEntryWidgetClass);
		}
		else
		{
			CardWidget = CreateWidget<UCardEntryWidget>(this, CardEntryWidgetClass);
		}

		if (!CardWidget)
		{
			continue;
		}

		CardWidget->InitializeFromCard(Card);
		CardWidget->OnCardClicked().AddUObject(this, &UCombatHUDWidget::HandleCardEntryClicked);

		SpawnedCardWidgets.Add(CardWidget);
		HandContainer->AddChild(CardWidget);
	}
}

void UCombatHUDWidget::SetSelectedCard(UCardDefinition* SelectedCard)
{
	RefreshSelectedCardText(SelectedCard);
}

void UCombatHUDWidget::RefreshSelectedCardText(UCardDefinition* SelectedCard)
{
	if (!SelectedCardText)
	{
		return;
	}

	if (SelectedCard)
	{
		SelectedCardText->SetText(FText::Format(
			FText::FromString(TEXT("Selected: {0}")),
			SelectedCard->DisplayName
		));
	}
	else
	{
		SelectedCardText->SetText(FText::FromString(TEXT("Selected: None")));
	}
}

void UCombatHUDWidget::HandleCardEntryClicked(UCardDefinition* ClickedCard)
{
	HandCardClickedDelegate.Broadcast(ClickedCard);
}