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
	case EJargonCombatCueType::RegenApplied:
	case EJargonCombatCueType::RegenTick:
	case EJargonCombatCueType::StatusCleansed:
	case EJargonCombatCueType::Lifesteal:
		return HealColor;

	case EJargonCombatCueType::ShieldGained:
	case EJargonCombatCueType::ShieldBroken:
		return ShieldColor;

	case EJargonCombatCueType::StunApplied:
	case EJargonCombatCueType::StunConsumed:
	case EJargonCombatCueType::WeakApplied:
	case EJargonCombatCueType::WeakConsumed:
		return StunColor;

	case EJargonCombatCueType::FreezeApplied:
	case EJargonCombatCueType::FreezeConsumed:
		return FreezeColor;

	case EJargonCombatCueType::ArtifactTriggered:
		return ArtifactColor;

	case EJargonCombatCueType::ElementalBonusTriggered:
		return ElementalBonusColor;

	default:
		return DefaultColor;
	}
}
