#include "Data/JargonRelicDefinition.h"

#include "Data/JargonHeroDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

bool UJargonRelicDefinition::HasAnyEffects() const
{
	return OnCombatStartEffects.Num() > 0
		|| OnPlayerTurnStartEffects.Num() > 0
		|| OnEnemyDeathEffects.Num() > 0;
}

bool UJargonRelicDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty() && HasAnyEffects();
}

bool UJargonRelicDefinition::IsEligibleForHeroDefinition(const UJargonHeroDefinition* HeroDefinition) const
{
	const bool bRequiresHeroClass = EligibleHeroClasses.Num() > 0;
	const bool bRequiresHeroAspect = EligibleHeroAspects.Num() > 0;
	if (!bRequiresHeroClass && !bRequiresHeroAspect)
	{
		return true;
	}

	if (!HeroDefinition)
	{
		return false;
	}

	if (bRequiresHeroClass && !EligibleHeroClasses.Contains(HeroDefinition->HeroClass))
	{
		return false;
	}

	if (bRequiresHeroAspect)
	{
		for (const EJargonHeroAspect EligibleAspect : EligibleHeroAspects)
		{
			if (EligibleAspect != EJargonHeroAspect::None && HeroDefinition->FindAspectDefinitionByAspect(EligibleAspect))
			{
				return true;
			}
		}

		return false;
	}

	return true;
}

#if WITH_EDITOR
EDataValidationResult UJargonRelicDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (!HasAnyEffects())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("No boon effects authored. At least one combat-start, turn-start, or enemy-death effect is required."));
	}

	if (EligibleHeroClasses.Contains(EJargonHeroClass::None))
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("EligibleHeroClasses contains None; remove it or leave the array empty to allow any class."));
	}

	if (EligibleHeroAspects.Contains(EJargonHeroAspect::None))
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("EligibleHeroAspects contains None; remove it or leave the array empty to allow any aspect kit."));
	}

	for (int32 EffectIndex = 0; EffectIndex < OnCombatStartEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			OnCombatStartEffects[EffectIndex],
			FString::Printf(TEXT("OnCombatStartEffects effect %d"), EffectIndex),
			EJargonEffectTrigger::OnCombatStart,
			Context);
	}

	for (int32 EffectIndex = 0; EffectIndex < OnPlayerTurnStartEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			OnPlayerTurnStartEffects[EffectIndex],
			FString::Printf(TEXT("OnPlayerTurnStartEffects effect %d"), EffectIndex),
			EJargonEffectTrigger::OnTurnStart,
			Context);
	}

	for (int32 EffectIndex = 0; EffectIndex < OnEnemyDeathEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			OnEnemyDeathEffects[EffectIndex],
			FString::Printf(TEXT("OnEnemyDeathEffects effect %d"), EffectIndex),
			EJargonEffectTrigger::OnEnemyDeath,
			Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
