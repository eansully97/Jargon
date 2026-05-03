#include "Combat/Widgets/CombatHoverInfoWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

void UCombatHoverInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshDescriptionText();
}

void UCombatHoverInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateHoverPosition();
}

void UCombatHoverInfoWidget::SetHoverInfo(const FJargonCombatHoverInfo& InHoverInfo)
{
	CurrentHoverInfo = InHoverInfo;
	RefreshDescriptionText();
	UpdateHoverPosition();
	BP_OnHoverInfoChanged(CurrentHoverInfo);
}

void UCombatHoverInfoWidget::UpdateHoverPosition()
{
	if (!bFollowMouseCursor || !CurrentHoverInfo.bHasInfo)
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

	const float ViewportScale = FMath::Max(UE_KINDA_SMALL_NUMBER, UWidgetLayoutLibrary::GetViewportScale(this));
	const FVector2D DesiredWidgetSize = GetDesiredSize() * ViewportScale;
	const FVector2D MousePosition(MouseX, MouseY);
	FVector2D TargetPosition = MousePosition + CursorOffset;

	if (bFlipToStayOnScreen)
	{
		if (TargetPosition.X + DesiredWidgetSize.X + ViewportPadding.X > ViewportSize.X)
		{
			TargetPosition.X = MousePosition.X - CursorOffset.X - DesiredWidgetSize.X;
		}

		if (TargetPosition.Y + DesiredWidgetSize.Y + ViewportPadding.Y > ViewportSize.Y)
		{
			TargetPosition.Y = MousePosition.Y - CursorOffset.Y - DesiredWidgetSize.Y;
		}
	}

	if (bClampToViewport)
	{
		TargetPosition.X = FMath::Clamp(TargetPosition.X, ViewportPadding.X, FMath::Max(ViewportPadding.X, ViewportSize.X - DesiredWidgetSize.X - ViewportPadding.X));
		TargetPosition.Y = FMath::Clamp(TargetPosition.Y, ViewportPadding.Y, FMath::Max(ViewportPadding.Y, ViewportSize.Y - DesiredWidgetSize.Y - ViewportPadding.Y));
	}

	SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
	SetPositionInViewport(TargetPosition / ViewportScale, false);
}

void UCombatHoverInfoWidget::RefreshDescriptionText()
{
	SetVisibility(CurrentHoverInfo.bHasInfo ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (!DescriptionText)
	{
		return;
	}

	DescriptionText->SetText(CurrentHoverInfo.bHasInfo ? CurrentHoverInfo.DescriptionText : FText::GetEmpty());
}
