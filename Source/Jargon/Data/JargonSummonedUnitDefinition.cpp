#include "Data/JargonSummonedUnitDefinition.h"

#include "Animation/AnimationAsset.h"
#include "Data/JargonAbilityDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

namespace
{
const TCHAR* GetTeamDebugName(ETeam Team)
{
	switch (Team)
	{
	case ETeam::Player:
		return TEXT("Player");

	case ETeam::Enemy:
		return TEXT("Enemy");

	default:
		return TEXT("Unknown");
	}
}

FString BoolToAuditText(bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

FString BuildWarningsSummary(const UJargonSummonedUnitDefinition* Definition)
{
	if (!Definition)
	{
		return TEXT("Invalid: null definition");
	}

	TArray<FString> Warnings;
	if (Definition->DisplayName.IsEmpty())
	{
		Warnings.Add(TEXT("DisplayName empty"));
	}

	if (Definition->MaxHP <= 0)
	{
		Warnings.Add(TEXT("MaxHP <= 0"));
	}

	if (Definition->MoveRange < 0)
	{
		Warnings.Add(TEXT("MoveRange < 0"));
	}

	if (Definition->AttackRange <= 0)
	{
		Warnings.Add(TEXT("AttackRange <= 0"));
	}

	if (Definition->AttackDamage < 0)
	{
		Warnings.Add(TEXT("AttackDamage < 0"));
	}

	if (!Definition->IdleAnimationOverride)
	{
		Warnings.Add(TEXT("IdleAnimationOverride missing"));
	}

	if (!Definition->BasicAttackAnimationOverride)
	{
		Warnings.Add(TEXT("BasicAttackAnimationOverride missing"));
	}

	if (!Definition->DeathAnimationOverride)
	{
		Warnings.Add(TEXT("DeathAnimationOverride missing"));
	}

	return Warnings.Num() > 0 ? FString::Join(Warnings, TEXT("; ")) : TEXT("None");
}

#if WITH_EDITOR
void ValidateSummonAbilityForHook(
	const UJargonSummonedUnitDefinition* Owner,
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
#endif
}

bool UJargonSummonedUnitDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& IdleAnimationOverride
		&& BasicAttackAnimationOverride
		&& DeathAnimationOverride
		&& MaxHP > 0
		&& MoveRange >= 0
		&& AttackRange > 0
		&& AttackDamage >= 0
		&& (!OnSummonedAbility || OnSummonedAbility->IsValidDefinition())
		&& (!OnTurnStartAbility || OnTurnStartAbility->IsValidDefinition())
		&& (!OnDeathAbility || OnDeathAbility->IsValidDefinition());
}

FString UJargonSummonedUnitDefinition::GetDebugSummary() const
{
	return GetAuditSummary();
}

FString UJargonSummonedUnitDefinition::GetAuditSummary() const
{
	const FString NameText = DisplayName.IsEmpty()
		? GetNameSafe(this)
		: DisplayName.ToString();

	return FString::Printf(
		TEXT("DisplayName=%s IdleAnimationOverride=%s BasicAttackAnimationOverride=%s DeathAnimationOverride=%s MaxHP=%d MoveRange=%d AttackRange=%d AttackDamage=%d Team=%s CanMove=%s CanAttack=%s AttackExhaustedOnSpawn=%s OnSummonedAbility=%s OnTurnStartAbility=%s OnDeathAbility=%s IsValidDefinition=%s Warnings=[%s]"),
		*NameText,
		*GetNameSafe(IdleAnimationOverride.Get()),
		*GetNameSafe(BasicAttackAnimationOverride.Get()),
		*GetNameSafe(DeathAnimationOverride.Get()),
		MaxHP,
		MoveRange,
		AttackRange,
		AttackDamage,
		GetTeamDebugName(Team),
		*BoolToAuditText(bCanMove),
		*BoolToAuditText(bCanAttack),
		*BoolToAuditText(bSummonEntersWithAttackExhausted),
		*GetNameSafe(OnSummonedAbility.Get()),
		*GetNameSafe(OnTurnStartAbility.Get()),
		*GetNameSafe(OnDeathAbility.Get()),
		*BoolToAuditText(IsValidDefinition()),
		*BuildWarningsSummary(this));
}

#if WITH_EDITOR
EDataValidationResult UJargonSummonedUnitDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (MaxHP <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("MaxHP must be greater than 0. Current value: %d."), MaxHP));
	}

	if (!IdleAnimationOverride)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("IdleAnimationOverride is required for the generic summon runtime shell."));
	}

	if (!BasicAttackAnimationOverride)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("BasicAttackAnimationOverride is required for the generic summon runtime shell."));
	}

	if (!DeathAnimationOverride)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DeathAnimationOverride is required for the generic summon runtime shell."));
	}

	if (MoveRange < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("MoveRange must be >= 0. Current value: %d."), MoveRange));
	}

	if (AttackRange <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("AttackRange must be greater than 0. Current value: %d."), AttackRange));
	}

	if (AttackDamage < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("AttackDamage must be >= 0. Current value: %d."), AttackDamage));
	}

	if (OnSummonedAbility)
	{
		ValidateSummonAbilityForHook(this, OnSummonedAbility, TEXT("OnSummonedAbility"), EJargonEffectTrigger::OnSummoned, EJargonAbilityHookContextType::SummonOnSummoned, Context);
	}

	if (OnTurnStartAbility)
	{
		ValidateSummonAbilityForHook(this, OnTurnStartAbility, TEXT("OnTurnStartAbility"), EJargonEffectTrigger::OnTurnStart, EJargonAbilityHookContextType::SummonTurnStart, Context);
	}

	if (OnDeathAbility)
	{
		ValidateSummonAbilityForHook(this, OnDeathAbility, TEXT("OnDeathAbility"), EJargonEffectTrigger::OnDeath, EJargonAbilityHookContextType::SummonDeath, Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
