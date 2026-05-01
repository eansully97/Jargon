// CombatHUDWidget.cpp

#include "CombatHUDWidget.h"


#include "Data/CardDefinition.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Combat/Widgets/CardEntryWidget.h"
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
	RefreshElementCharges(0, 0, 0, 0, 0, 0);
	RefreshHeroIdentity(FJargonHeroClassInfo(), FJargonHeroAspectInfo());
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

void UCombatHUDWidget::SetElementChargeValues(const TMap<EJargonElementType, int32>& NewElementCharges)
{
	const auto GetChargeAmount = [&NewElementCharges](EJargonElementType ElementType)
	{
		const int32* FoundAmount = NewElementCharges.Find(ElementType);
		return FoundAmount ? FMath::Max(0, *FoundAmount) : 0;
	};

	RefreshElementCharges(
		GetChargeAmount(EJargonElementType::Fire),
		GetChargeAmount(EJargonElementType::Frost),
		GetChargeAmount(EJargonElementType::Storm),
		GetChargeAmount(EJargonElementType::Nature),
		GetChargeAmount(EJargonElementType::Radiance),
		GetChargeAmount(EJargonElementType::Quietus));
}

void UCombatHUDWidget::RefreshHeroIdentity(
	const FJargonHeroClassInfo& HeroClassInfo,
	const FJargonHeroAspectInfo& HeroAspectInfo)
{
	const bool bHasClass = HeroClassInfo.HeroClass != EJargonHeroClass::None && !HeroClassInfo.DisplayName.IsEmpty();
	const bool bHasElement = HeroAspectInfo.ElementType != EJargonElementType::None || HeroAspectInfo.RequiredElement != EJargonElementType::None;
	const bool bHasActiveAspect = HeroAspectInfo.bIsActive && HeroAspectInfo.Aspect != EJargonHeroAspect::None;

	const EJargonElementType DisplayElement = HeroAspectInfo.ElementType != EJargonElementType::None
		? HeroAspectInfo.ElementType
		: HeroAspectInfo.RequiredElement;
	const int32 CurrentCharges = HeroAspectInfo.CurrentElementCharges > 0
		? HeroAspectInfo.CurrentElementCharges
		: HeroAspectInfo.CurrentCharges;
	const int32 RequiredCharges = HeroAspectInfo.RequiredElementCharges > 0
		? HeroAspectInfo.RequiredElementCharges
		: HeroAspectInfo.RequiredCharges;

	const FText ClassName = bHasClass ? HeroClassInfo.DisplayName : FText::FromString(TEXT("Unknown"));
	const FText ElementName = bHasElement ? GetElementDisplayText(DisplayElement) : FText::FromString(TEXT("None"));
	const FText AspectName = !HeroAspectInfo.DisplayName.IsEmpty()
		? HeroAspectInfo.DisplayName
		: FText::FromString(TEXT("None"));

	if (HeroClassText)
	{
		HeroClassText->SetText(FText::Format(FText::FromString(TEXT("Class: {0}")), ClassName));
	}

	if (HeroClassDescriptionText)
	{
		HeroClassDescriptionText->SetText(bHasClass ? HeroClassInfo.Description : FText::GetEmpty());
	}

	if (DominantElementText)
	{
		DominantElementText->SetText(FText::Format(FText::FromString(TEXT("Dominant: {0}")), ElementName));
	}

	if (ActiveAspectText)
	{
		if (bHasActiveAspect)
		{
			ActiveAspectText->SetText(FText::Format(FText::FromString(TEXT("Aspect: {0}")), AspectName));
		}
		else if (bHasElement)
		{
			ActiveAspectText->SetText(FText::Format(FText::FromString(TEXT("Building: {0}")), AspectName));
		}
		else
		{
			ActiveAspectText->SetText(FText::FromString(TEXT("Aspect: None")));
		}
	}

	if (ActiveAspectDescriptionText)
	{
		ActiveAspectDescriptionText->SetText(bHasActiveAspect ? HeroAspectInfo.Description : FText::GetEmpty());
	}

	if (ActiveAspectPassiveText)
	{
		if (bHasActiveAspect && !HeroAspectInfo.PassiveName.IsEmpty())
		{
			ActiveAspectPassiveText->SetText(FText::Format(FText::FromString(TEXT("Passive: {0}")), HeroAspectInfo.PassiveName));
		}
		else
		{
			ActiveAspectPassiveText->SetText(FText::GetEmpty());
		}
	}

	if (AspectProgressText)
	{
		if (bHasElement && RequiredCharges > 0)
		{
			AspectProgressText->SetText(FText::Format(
				FText::FromString(TEXT("{0} {1}/{2}")),
				ElementName,
				FText::AsNumber(CurrentCharges),
				FText::AsNumber(RequiredCharges)));
		}
		else
		{
			AspectProgressText->SetText(FText::GetEmpty());
		}
	}

	if (HeroIdentitySummaryText)
	{
		if (bHasActiveAspect && RequiredCharges > 0)
		{
			HeroIdentitySummaryText->SetText(FText::Format(
				FText::FromString(TEXT("{0} / {1} ({2} {3}/{4})")),
				ClassName,
				AspectName,
				ElementName,
				FText::AsNumber(CurrentCharges),
				FText::AsNumber(RequiredCharges)));
		}
		else if (bHasElement && RequiredCharges > 0)
		{
			HeroIdentitySummaryText->SetText(FText::Format(
				FText::FromString(TEXT("{0} / {1} {2}/{3}")),
				ClassName,
				ElementName,
				FText::AsNumber(CurrentCharges),
				FText::AsNumber(RequiredCharges)));
		}
		else
		{
			HeroIdentitySummaryText->SetText(FText::Format(
				FText::FromString(TEXT("{0} / No element influence")),
				ClassName));
		}
	}

	BP_OnHeroIdentityRefreshed(HeroClassInfo, HeroAspectInfo);
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

void UCombatHUDWidget::RefreshElementCharges(
	int32 Fire,
	int32 Frost,
	int32 Storm,
	int32 Nature,
	int32 Radiance,
	int32 Quietus)
{
	if (FireChargeText)
	{
		FireChargeText->SetText(FText::AsNumber(Fire));
	}

	if (FrostChargeText)
	{
		FrostChargeText->SetText(FText::AsNumber(Frost));
	}

	if (StormChargeText)
	{
		StormChargeText->SetText(FText::AsNumber(Storm));
	}

	if (NatureChargeText)
	{
		NatureChargeText->SetText(FText::AsNumber(Nature));
	}

	if (RadianceChargeText)
	{
		RadianceChargeText->SetText(FText::AsNumber(Radiance));
	}

	if (QuietusChargeText)
	{
		QuietusChargeText->SetText(FText::AsNumber(Quietus));
	}
}

FText UCombatHUDWidget::GetElementDisplayText(EJargonElementType ElementType) const
{
	const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
	return ElementEnum
		? ElementEnum->GetDisplayNameTextByValue(static_cast<int64>(ElementType))
		: FText::FromString(TEXT("Element"));
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
