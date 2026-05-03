#include "Data/JargonSummonedUnitDefinition.h"

#include "Animation/AnimationAsset.h"

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
		&& AttackDamage >= 0;
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
		TEXT("DisplayName=%s IdleAnimationOverride=%s BasicAttackAnimationOverride=%s DeathAnimationOverride=%s MaxHP=%d MoveRange=%d AttackRange=%d AttackDamage=%d Team=%s CanMove=%s CanAttack=%s AttackExhaustedOnSpawn=%s OnSummonedEffects=%d OnTurnStartEffects=%d OnDeathEffects=%d IsValidDefinition=%s Warnings=[%s]"),
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
		OnSummonedEffects.Num(),
		OnTurnStartEffects.Num(),
		OnDeathEffects.Num(),
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

	for (int32 EffectIndex = 0; EffectIndex < OnSummonedEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			OnSummonedEffects[EffectIndex],
			FString::Printf(TEXT("OnSummonedEffects effect %d"), EffectIndex),
			EJargonEffectTrigger::OnSummoned,
			Context);
	}

	for (int32 EffectIndex = 0; EffectIndex < OnTurnStartEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			OnTurnStartEffects[EffectIndex],
			FString::Printf(TEXT("OnTurnStartEffects effect %d"), EffectIndex),
			EJargonEffectTrigger::OnTurnStart,
			Context);
	}

	for (int32 EffectIndex = 0; EffectIndex < OnDeathEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			OnDeathEffects[EffectIndex],
			FString::Printf(TEXT("OnDeathEffects effect %d"), EffectIndex),
			EJargonEffectTrigger::OnDeath,
			Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
