#include "Combat/Presentation/JargonFloatingCombatTextWidget.h"

#include "Components/TextBlock.h"
#include "TimerManager.h"

void UJargonFloatingCombatTextWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoRemoveTimerHandle);
	}

	Super::NativeDestruct();
}

void UJargonFloatingCombatTextWidget::InitializeFromCue(const FJargonCombatCueEvent& InCue, const FLinearColor& InColor)
{
	Cue = InCue;
	DisplayText = InCue.TextOverride.IsEmpty() ? BuildDefaultTextForCue(InCue) : InCue.TextOverride;
	DisplayColor = InCue.bHasColorOverride ? InCue.ColorOverride : InColor;

	ApplyTextToBoundWidgets();
	BP_OnInitializedFromCue(Cue);

	if (bAutoRemoveAfterDelay && AutoRemoveDelay > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoRemoveTimerHandle);
			World->GetTimerManager().SetTimer(
				AutoRemoveTimerHandle,
				this,
				&UJargonFloatingCombatTextWidget::HandleAutoRemoveElapsed,
				AutoRemoveDelay,
				false);
		}
	}
}

void UJargonFloatingCombatTextWidget::HandleAutoRemoveElapsed()
{
	RemoveFromParent();
}

void UJargonFloatingCombatTextWidget::ApplyTextToBoundWidgets()
{
	if (FloatingTextText)
	{
		FloatingTextText->SetText(DisplayText);
		FloatingTextText->SetColorAndOpacity(FSlateColor(DisplayColor));
	}

	if (ValueText && ValueText != FloatingTextText)
	{
		ValueText->SetText(DisplayText);
		ValueText->SetColorAndOpacity(FSlateColor(DisplayColor));
	}
}

FText UJargonFloatingCombatTextWidget::BuildDefaultTextForCue(const FJargonCombatCueEvent& InCue)
{
	switch (InCue.CueType)
	{
	case EJargonCombatCueType::Damage:
	case EJargonCombatCueType::PushCollision:
		return FText::FromString(FString::Printf(TEXT("-%d"), FMath::Max(0, InCue.Value)));

	case EJargonCombatCueType::Heal:
		return FText::FromString(FString::Printf(TEXT("+%d"), FMath::Max(0, InCue.Value)));

	case EJargonCombatCueType::ShieldGained:
		return FText::FromString(FString::Printf(TEXT("+%d Shield"), FMath::Max(0, InCue.Value)));

	case EJargonCombatCueType::ShieldBroken:
		return FText::FromString(TEXT("Shield Break"));

	case EJargonCombatCueType::StunApplied:
		return FText::FromString(TEXT("Stun"));

	case EJargonCombatCueType::StunConsumed:
		return FText::FromString(TEXT("Stunned"));

	case EJargonCombatCueType::RelicTriggered:
		return FText::FromString(TEXT("Relic"));

	default:
		return FText::GetEmpty();
	}
}
