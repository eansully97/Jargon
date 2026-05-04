#include "Data/JargonArtifactDefinition.h"

#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonHeroDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
namespace
{
void ValidateArtifactAbilityForHook(
	const UJargonArtifactDefinition* Owner,
	const UJargonAbilityDefinition* Ability,
	const TCHAR* Label,
	EJargonEffectTrigger ExpectedTrigger,
	EJargonAbilityHookContextType ExpectedHookContext,
	FDataValidationContext& Context)
{
	if (!Ability)
	{
		return;
	}

	if (!Ability->IsValidDefinition())
	{
		JargonDataAssetValidation::AddError(Context, Owner, FString::Printf(TEXT("%s is assigned but is not a valid ability definition."), Label));
	}

	if (Ability->ExpectedTrigger != ExpectedTrigger)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(
				TEXT("%s expects trigger %s but this hook requires %s."),
				Label,
				*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(Ability->ExpectedTrigger)),
				*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(ExpectedTrigger))));
	}

	if (Ability->ExpectedHookContext == EJargonAbilityHookContextType::None)
	{
		JargonDataAssetValidation::AddWarning(Context, Owner, FString::Printf(TEXT("%s has ExpectedHookContext=None; set it to %s."), Label, *JargonEffectContracts::GetHookContextName(ExpectedHookContext)));
	}
	else if (Ability->ExpectedHookContext != ExpectedHookContext)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(
				TEXT("%s expects hook context %s but this hook requires %s."),
				Label,
				*JargonEffectContracts::GetHookContextName(Ability->ExpectedHookContext),
				*JargonEffectContracts::GetHookContextName(ExpectedHookContext)));
	}
}
}
#endif

bool UJargonArtifactDefinition::HasAnyEffects() const
{
	return OnCombatStartAbility != nullptr
		|| OnPlayerTurnStartAbility != nullptr
		|| OnEnemyDeathAbility != nullptr;
}

bool UJargonArtifactDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& HasAnyEffects()
		&& (!OnCombatStartAbility || OnCombatStartAbility->IsValidDefinition())
		&& (!OnPlayerTurnStartAbility || OnPlayerTurnStartAbility->IsValidDefinition())
		&& (!OnEnemyDeathAbility || OnEnemyDeathAbility->IsValidDefinition());
}

bool UJargonArtifactDefinition::IsEligibleForHeroDefinition(const UJargonHeroDefinition* HeroDefinition) const
{
	const bool bRequiresHeroClass = EligibleHeroClasses.Num() > 0;
	if (!bRequiresHeroClass)
	{
		return true;
	}

	if (!HeroDefinition)
	{
		return false;
	}

	if (!EligibleHeroClasses.Contains(HeroDefinition->HeroClass))
	{
		return false;
	}

	return true;
}

#if WITH_EDITOR
EDataValidationResult UJargonArtifactDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (!HasAnyEffects())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("No artifact effects authored. At least one combat-start, turn-start, or enemy-death effect is required."));
	}

	if (EligibleHeroClasses.Contains(EJargonHeroClass::None))
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("EligibleHeroClasses contains None; remove it or leave the array empty to allow any class."));
	}

	if (ArtifactRole == EJargonArtifactRole::ClassDefault && EligibleHeroClasses.Num() == 0)
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("Class Default artifacts should normally restrict EligibleHeroClasses to the owning hero class."));
	}

	if (OnCombatStartAbility)
	{
		ValidateArtifactAbilityForHook(this, OnCombatStartAbility, TEXT("OnCombatStartAbility"), EJargonEffectTrigger::OnCombatStart, EJargonAbilityHookContextType::ArtifactCombatStart, Context);
	}

	if (OnPlayerTurnStartAbility)
	{
		ValidateArtifactAbilityForHook(this, OnPlayerTurnStartAbility, TEXT("OnPlayerTurnStartAbility"), EJargonEffectTrigger::OnTurnStart, EJargonAbilityHookContextType::ArtifactPlayerTurnStart, Context);
	}

	if (OnEnemyDeathAbility)
	{
		ValidateArtifactAbilityForHook(this, OnEnemyDeathAbility, TEXT("OnEnemyDeathAbility"), EJargonEffectTrigger::OnEnemyDeath, EJargonAbilityHookContextType::ArtifactEnemyDeath, Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
