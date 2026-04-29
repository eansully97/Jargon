#include "Combat/Presentation/JargonCombatPresentationSettings.h"

#include "Combat/Presentation/JargonFloatingCombatTextWidget.h"

UJargonCombatPresentationSettings::UJargonCombatPresentationSettings()
{
	FloatingTextWidgetClass = UJargonFloatingCombatTextWidget::StaticClass();
}

FLinearColor UJargonCombatPresentationSettings::GetColorForCue(EJargonCombatCueType CueType) const
{
	switch (CueType)
	{
	case EJargonCombatCueType::Damage:
	case EJargonCombatCueType::PushCollision:
		return DamageColor;

	case EJargonCombatCueType::Heal:
		return HealColor;

	case EJargonCombatCueType::ShieldGained:
	case EJargonCombatCueType::ShieldBroken:
		return ShieldColor;

	case EJargonCombatCueType::StunApplied:
	case EJargonCombatCueType::StunConsumed:
		return StunColor;

	case EJargonCombatCueType::RelicTriggered:
		return RelicColor;

	default:
		return DefaultColor;
	}
}
