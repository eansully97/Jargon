#include "Data/JargonRelicDefinition.h"

#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonHeroDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

bool UJargonRelicDefinition::HasAnyEffects() const
{
	return OnCombatStartEffects.Num() > 0
		|| OnCombatStartAbility != nullptr
		|| OnPlayerTurnStartEffects.Num() > 0
		|| OnPlayerTurnStartAbility != nullptr
		|| OnEnemyDeathEffects.Num() > 0
		|| OnEnemyDeathAbility != nullptr;
}

bool UJargonRelicDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& HasAnyEffects()
		&& (!OnCombatStartAbility || OnCombatStartAbility->IsValidDefinition())
		&& (!OnPlayerTurnStartAbility || OnPlayerTurnStartAbility->IsValidDefinition())
		&& (!OnEnemyDeathAbility || OnEnemyDeathAbility->IsValidDefinition());
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

	if (OnCombatStartAbility)
	{
		if (!OnCombatStartAbility->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("OnCombatStartAbility is assigned but is not a valid ability definition."));
		}

		if (OnCombatStartEffects.Num() > 0)
		{
			JargonDataAssetValidation::AddWarning(Context, this, TEXT("OnCombatStartAbility and raw OnCombatStartEffects are both authored. Runtime will prefer the ability definition."));
		}
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

	if (OnPlayerTurnStartAbility)
	{
		if (!OnPlayerTurnStartAbility->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("OnPlayerTurnStartAbility is assigned but is not a valid ability definition."));
		}

		if (OnPlayerTurnStartEffects.Num() > 0)
		{
			JargonDataAssetValidation::AddWarning(Context, this, TEXT("OnPlayerTurnStartAbility and raw OnPlayerTurnStartEffects are both authored. Runtime will prefer the ability definition."));
		}
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

	if (OnEnemyDeathAbility)
	{
		if (!OnEnemyDeathAbility->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("OnEnemyDeathAbility is assigned but is not a valid ability definition."));
		}

		if (OnEnemyDeathEffects.Num() > 0)
		{
			JargonDataAssetValidation::AddWarning(Context, this, TEXT("OnEnemyDeathAbility and raw OnEnemyDeathEffects are both authored. Runtime will prefer the ability definition."));
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
