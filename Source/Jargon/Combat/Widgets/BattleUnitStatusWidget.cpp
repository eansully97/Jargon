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
		if (StunText)
		{
			StunText->SetText(FText::GetEmpty());
		}
		if (FreezeText)
		{
			FreezeText->SetText(FText::GetEmpty());
		}
		if (BurnText)
		{
			BurnText->SetText(FText::GetEmpty());
		}
		if (RootText)
		{
			RootText->SetText(FText::GetEmpty());
		}
		if (VulnerableText)
		{
			VulnerableText->SetText(FText::GetEmpty());
		}
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

	if (StunText)
	{
		const int32 StunTurns = ObservedUnit->GetStunTurnsRemaining();
		StunText->SetText(StunTurns > 0
			? FText::FromString(TEXT("Stunned"))
			: FText::GetEmpty());
	}

	if (FreezeText)
	{
		const int32 FreezeTurns = ObservedUnit->GetFreezeTurnsRemaining();
		FreezeText->SetText(FreezeTurns > 0
			? FText::FromString(TEXT("Frozen"))
			: FText::GetEmpty());
	}

	if (BurnText)
	{
		const int32 BurnStacks = ObservedUnit->GetBurnStacks();
		BurnText->SetText(BurnStacks > 0
			? FText::Format(FText::FromString(TEXT("Burn {0}")), FText::AsNumber(BurnStacks))
			: FText::GetEmpty());
	}

	if (RootText)
	{
		const int32 RootTurns = ObservedUnit->GetRootTurnsRemaining();
		RootText->SetText(RootTurns > 0
			? FText::FromString(TEXT("Rooted"))
			: FText::GetEmpty());
	}

	if (VulnerableText)
	{
		const int32 VulnerableBonus = ObservedUnit->GetVulnerableDamageBonus();
		VulnerableText->SetText(VulnerableBonus > 0
			? FText::Format(FText::FromString(TEXT("Vuln +{0}")), FText::AsNumber(VulnerableBonus))
			: FText::GetEmpty());
	}
}
