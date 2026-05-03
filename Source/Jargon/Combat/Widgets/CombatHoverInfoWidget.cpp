#include "Combat/Widgets/CombatHoverInfoWidget.h"

#include "Components/TextBlock.h"

void UCombatHoverInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshDescriptionText();
}

void UCombatHoverInfoWidget::SetHoverInfo(const FJargonCombatHoverInfo& InHoverInfo)
{
	CurrentHoverInfo = InHoverInfo;
	RefreshDescriptionText();
	BP_OnHoverInfoChanged(CurrentHoverInfo);
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
