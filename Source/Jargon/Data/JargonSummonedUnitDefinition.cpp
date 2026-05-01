#include "Data/JargonSummonedUnitDefinition.h"

#include "Combat/Units/BattleUnit.h"

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

	if (!Definition->OptionalUnitClassOverride)
	{
		Warnings.Add(TEXT("No OptionalUnitClassOverride; verify CombatGameMode DefaultSummonedUnitClass is assigned"));
	}

	return Warnings.Num() > 0 ? FString::Join(Warnings, TEXT("; ")) : TEXT("None");
}
}

bool UJargonSummonedUnitDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
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
		TEXT("DisplayName=%s MaxHP=%d MoveRange=%d AttackRange=%d AttackDamage=%d Team=%s OptionalUnitClassOverride=%s CanMove=%s CanAttack=%s AttackExhaustedOnSpawn=%s OnSummonedEffects=%d OnTurnStartEffects=%d OnDeathEffects=%d IsValidDefinition=%s Warnings=[%s]"),
		*NameText,
		MaxHP,
		MoveRange,
		AttackRange,
		AttackDamage,
		GetTeamDebugName(Team),
		*GetNameSafe(OptionalUnitClassOverride.Get()),
		*BoolToAuditText(bCanMove),
		*BoolToAuditText(bCanAttack),
		*BoolToAuditText(bSummonEntersWithAttackExhausted),
		OnSummonedEffects.Num(),
		OnTurnStartEffects.Num(),
		OnDeathEffects.Num(),
		*BoolToAuditText(IsValidDefinition()),
		*BuildWarningsSummary(this));
}
