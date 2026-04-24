// CombatHUDWidget.cpp

#include "Widgets/CombatHUDWidget.h"

#include "Data/CardDefinition.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/CardEntryWidget.h"
#include "Components/Button.h"
#include "Core/JargonTypes.h"

void UCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EndTurnButton)
	{
		EndTurnButton->OnClicked.RemoveDynamic(this, &UCombatHUDWidget::HandleEndTurnButtonClicked);
		EndTurnButton->OnClicked.AddDynamic(this, &UCombatHUDWidget::HandleEndTurnButtonClicked);
	}

	RefreshSelectedCardText(nullptr);
	RefreshPhaseText(ECombatPhase::BattleStart);
	RefreshEnergyText(0, 0);
	RefreshActionAvailabilityText(ECombatPhase::BattleStart, false, false);
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

void UCombatHUDWidget::HandleEndTurnButtonClicked()
{
	EndTurnClickedDelegate.Broadcast();
}

void UCombatHUDWidget::RefreshPhaseText(ECombatPhase NewPhase)
{
	if (!PhaseText)
	{
		return;
	}

	FText PhaseLabel = FText::FromString(TEXT("Unknown"));

	switch (NewPhase)
	{
	case ECombatPhase::BattleStart:
		PhaseLabel = FText::FromString(TEXT("Battle Start"));
		break;

	case ECombatPhase::PlayerTurn:
		PhaseLabel = FText::FromString(TEXT("Player Turn"));
		break;

	case ECombatPhase::EnemyTurn:
		PhaseLabel = FText::FromString(TEXT("Enemy Turn"));
		break;

	case ECombatPhase::Resolving:
		PhaseLabel = FText::FromString(TEXT("Resolving"));
		break;

	case ECombatPhase::Victory:
		PhaseLabel = FText::FromString(TEXT("Victory"));
		break;

	case ECombatPhase::Defeat:
		PhaseLabel = FText::FromString(TEXT("Defeat"));
		break;

	default:
		break;
	}

	PhaseText->SetText(PhaseLabel);
}

void UCombatHUDWidget::SetSelectedCard(UCardDefinition* SelectedCard)
{
	RefreshSelectedCardText(SelectedCard);
}

void UCombatHUDWidget::SetPhaseText(ECombatPhase NewPhase)
{
	RefreshPhaseText(NewPhase);
}

void UCombatHUDWidget::SetEnergyText(int32 NewEnergy)
{
	RefreshEnergyText(NewEnergy, INDEX_NONE);
}

void UCombatHUDWidget::SetEnergyValues(int32 NewEnergy, int32 NewMaxEnergy)
{
	RefreshEnergyText(NewEnergy, NewMaxEnergy);
}

void UCombatHUDWidget::SetActionAvailability(ECombatPhase CurrentPhase, bool bCanMove, bool bCanAttack)
{
	RefreshActionAvailabilityText(CurrentPhase, bCanMove, bCanAttack);
}

void UCombatHUDWidget::RefreshEnergyText(int32 NewEnergy, int32 NewMaxEnergy)
{
	if (!EnergyText)
	{
		return;
	}

	if (NewMaxEnergy >= 0)
	{
		EnergyText->SetText(FText::Format(
			FText::FromString(TEXT("{0} / {1}")),
			FText::AsNumber(NewEnergy),
			FText::AsNumber(NewMaxEnergy)
		));
		return;
	}

	EnergyText->SetText(FText::AsNumber(NewEnergy));
}

void UCombatHUDWidget::RefreshActionAvailabilityText(ECombatPhase CurrentPhase, bool bCanMove, bool bCanAttack)
{
	if (!ActionAvailabilityText)
	{
		return;
	}

	const bool bIsPlayerTurn = CurrentPhase == ECombatPhase::PlayerTurn;
	const FText MoveLabel = bIsPlayerTurn
		? (bCanMove ? FText::FromString(TEXT("Ready")) : FText::FromString(TEXT("Used")))
		: FText::FromString(TEXT("-"));
	const FText AttackLabel = bIsPlayerTurn
		? (bCanAttack ? FText::FromString(TEXT("Ready")) : FText::FromString(TEXT("Used")))
		: FText::FromString(TEXT("-"));

	ActionAvailabilityText->SetText(FText::Format(
		FText::FromString(TEXT("Move: {0}\nAttack: {1}")),
		MoveLabel,
		AttackLabel
	));
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
