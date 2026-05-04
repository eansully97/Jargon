#include "Combat/Widgets/ElementalBonusChoiceWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Combat/JargonCombatPlayerController.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"

void UElementalBonusChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &UElementalBonusChoiceWidget::HandleConfirmButtonClicked);
		ConfirmButton->OnClicked.AddDynamic(this, &UElementalBonusChoiceWidget::HandleConfirmButtonClicked);
	}

	if (SpendButton)
	{
		SpendButton->OnClicked.RemoveDynamic(this, &UElementalBonusChoiceWidget::HandleSpendButtonClicked);
		SpendButton->OnClicked.AddDynamic(this, &UElementalBonusChoiceWidget::HandleSpendButtonClicked);
	}

	if (SkipButton)
	{
		SkipButton->OnClicked.RemoveDynamic(this, &UElementalBonusChoiceWidget::HandleSkipButtonClicked);
		SkipButton->OnClicked.AddDynamic(this, &UElementalBonusChoiceWidget::HandleSkipButtonClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &UElementalBonusChoiceWidget::HandleCancelButtonClicked);
		CancelButton->OnClicked.AddDynamic(this, &UElementalBonusChoiceWidget::HandleCancelButtonClicked);
	}

	RefreshChoiceVisibility();
}

void UElementalBonusChoiceWidget::SetChoiceRequest(const FJargonElementalBonusChoiceRequest& InRequest)
{
	CurrentChoiceRequest = InRequest;
	ClearSelectedBonusIndices();
	RefreshChoiceVisibility();
	BP_OnChoiceRequestChanged(CurrentChoiceRequest);
	PositionAtMouseCursor();
}

void UElementalBonusChoiceWidget::ClearChoiceRequest()
{
	const bool bHadChoiceRequest = CurrentChoiceRequest.bHasUsableOptions;
	CurrentChoiceRequest = FJargonElementalBonusChoiceRequest();
	ClearSelectedBonusIndices();
	RefreshChoiceVisibility();

	if (bHadChoiceRequest)
	{
		BP_OnChoiceRequestCleared();
	}
}

bool UElementalBonusChoiceWidget::ConfirmSelectedBonusIndices(const TArray<int32>& InSelectedBonusIndices)
{
	AJargonCombatPlayerController* CombatPlayerController = GetOwningPlayer<AJargonCombatPlayerController>();
	return CombatPlayerController
		? CombatPlayerController->ConfirmPendingElementalBonusChoices(InSelectedBonusIndices)
		: false;
}

bool UElementalBonusChoiceWidget::ConfirmSingleBonusIndex(int32 BonusIndex)
{
	if (BonusIndex == INDEX_NONE)
	{
		return false;
	}

	TArray<int32> SingleSelectedBonusIndex;
	SingleSelectedBonusIndex.Add(BonusIndex);
	return ConfirmSelectedBonusIndices(SingleSelectedBonusIndex);
}

bool UElementalBonusChoiceWidget::ConfirmFirstOfferedBonus()
{
	AJargonCombatPlayerController* CombatPlayerController = GetOwningPlayer<AJargonCombatPlayerController>();
	return CombatPlayerController
		? CombatPlayerController->ConfirmFirstPendingElementalBonusChoice()
		: false;
}

bool UElementalBonusChoiceWidget::ConfirmCurrentSelection()
{
	if (SelectedBonusIndices.Num() <= 0 && CurrentChoiceRequest.Options.Num() == 1)
	{
		return ConfirmSingleBonusIndex(CurrentChoiceRequest.Options[0].BonusIndex);
	}

	return ConfirmSelectedBonusIndices(SelectedBonusIndices);
}

bool UElementalBonusChoiceWidget::SkipBonuses()
{
	AJargonCombatPlayerController* CombatPlayerController = GetOwningPlayer<AJargonCombatPlayerController>();
	return CombatPlayerController
		? CombatPlayerController->SkipPendingElementalBonusChoices()
		: false;
}

void UElementalBonusChoiceWidget::CancelChoice()
{
	AJargonCombatPlayerController* CombatPlayerController = GetOwningPlayer<AJargonCombatPlayerController>();
	if (CombatPlayerController)
	{
		CombatPlayerController->CancelPendingElementalBonusChoice();
		return;
	}

	ClearChoiceRequest();
}

void UElementalBonusChoiceWidget::SetBonusSelected(int32 BonusIndex, bool bSelected)
{
	if (BonusIndex == INDEX_NONE)
	{
		return;
	}

	if (bSelected)
	{
		SelectedBonusIndices.AddUnique(BonusIndex);
		return;
	}

	SelectedBonusIndices.Remove(BonusIndex);
}

void UElementalBonusChoiceWidget::ToggleBonusSelected(int32 BonusIndex)
{
	if (BonusIndex == INDEX_NONE)
	{
		return;
	}

	if (SelectedBonusIndices.Contains(BonusIndex))
	{
		SelectedBonusIndices.Remove(BonusIndex);
		return;
	}

	SelectedBonusIndices.Add(BonusIndex);
}

bool UElementalBonusChoiceWidget::IsBonusSelected(int32 BonusIndex) const
{
	return SelectedBonusIndices.Contains(BonusIndex);
}

int32 UElementalBonusChoiceWidget::GetFirstOfferedBonusIndex() const
{
	return CurrentChoiceRequest.Options.Num() > 0
		? CurrentChoiceRequest.Options[0].BonusIndex
		: INDEX_NONE;
}

void UElementalBonusChoiceWidget::ClearSelectedBonusIndices()
{
	SelectedBonusIndices.Reset();
}

void UElementalBonusChoiceWidget::PositionAtMouseCursor()
{
	if (!bPositionAtMouseOnOpen || !CurrentChoiceRequest.bHasUsableOptions)
	{
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!OwningPlayer->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return;
	}

	ForceLayoutPrepass();

	const float ViewportScale = FMath::Max(UE_KINDA_SMALL_NUMBER, UWidgetLayoutLibrary::GetViewportScale(this));
	const FVector2D DesiredWidgetSize = GetDesiredSize() * ViewportScale;
	const FVector2D MousePosition(MouseX, MouseY);
	FVector2D TargetPosition = MousePosition + CursorOffset;

	if (TargetPosition.X + DesiredWidgetSize.X + ViewportPadding.X > ViewportSize.X)
	{
		TargetPosition.X = MousePosition.X - CursorOffset.X - DesiredWidgetSize.X;
	}

	if (TargetPosition.Y + DesiredWidgetSize.Y + ViewportPadding.Y > ViewportSize.Y)
	{
		TargetPosition.Y = MousePosition.Y - CursorOffset.Y - DesiredWidgetSize.Y;
	}

	TargetPosition.X = FMath::Clamp(
		TargetPosition.X,
		ViewportPadding.X,
		FMath::Max(ViewportPadding.X, ViewportSize.X - DesiredWidgetSize.X - ViewportPadding.X));
	TargetPosition.Y = FMath::Clamp(
		TargetPosition.Y,
		ViewportPadding.Y,
		FMath::Max(ViewportPadding.Y, ViewportSize.Y - DesiredWidgetSize.Y - ViewportPadding.Y));

	SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
	SetPositionInViewport(TargetPosition / ViewportScale, false);
}

void UElementalBonusChoiceWidget::RefreshChoiceVisibility()
{
	SetVisibility(CurrentChoiceRequest.bHasUsableOptions
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed);
}

void UElementalBonusChoiceWidget::HandleConfirmButtonClicked()
{
	ConfirmCurrentSelection();
}

void UElementalBonusChoiceWidget::HandleSpendButtonClicked()
{
	if (SelectedBonusIndices.Num() > 0)
	{
		ConfirmCurrentSelection();
		return;
	}

	ConfirmFirstOfferedBonus();
}

void UElementalBonusChoiceWidget::HandleSkipButtonClicked()
{
	SkipBonuses();
}

void UElementalBonusChoiceWidget::HandleCancelButtonClicked()
{
	CancelChoice();
}
