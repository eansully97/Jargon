#include "BattleUnitStatusWidget.h"

#include "Components/TextBlock.h"
#include "Combat/Units/BattleUnit.h"

void UBattleUnitStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromObservedUnit();
}

void UBattleUnitStatusWidget::SetObservedUnit(ABattleUnit* InUnit)
{
	ObservedUnit = InUnit;
	RefreshFromObservedUnit();
}

void UBattleUnitStatusWidget::RefreshFromObservedUnit()
{
	if (!HPText || !AttackText || !ShieldText)
	{
		return;
	}

	if (!ObservedUnit)
	{
		HPText->SetText(FText::GetEmpty());
		AttackText->SetText(FText::GetEmpty());
		AttackText->SetColorAndOpacity(AttackReadyColor);
		ShieldText->SetText(FText::GetEmpty());
		return;
	}

	HPText->SetText(FText::Format(
		FText::FromString(TEXT("HP {0}/{1}")),
		FText::AsNumber(ObservedUnit->GetCurrentHP()),
		FText::AsNumber(ObservedUnit->GetMaxHP())
	));

	AttackText->SetText(FText::Format(
		FText::FromString(TEXT("ATK {0}")),
		FText::AsNumber(ObservedUnit->GetAttackDamage())
	));
	AttackText->SetColorAndOpacity(
		ObservedUnit->HasAttackActionRemaining() ? AttackReadyColor : AttackUnavailableColor
	);

	const int32 ShieldValue = ObservedUnit->GetTemporaryShield();
	if (ShieldValue > 0)
	{
		ShieldText->SetText(FText::Format(
			FText::FromString(TEXT("SHD {0}")),
			FText::AsNumber(ShieldValue)
		));
	}
	else
	{
		ShieldText->SetText(FText::GetEmpty());
	}
}
