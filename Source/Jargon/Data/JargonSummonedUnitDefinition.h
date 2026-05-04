#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "JargonSummonedUnitDefinition.generated.h"

class UAnimationAsset;
class UTexture2D;
class UJargonAbilityDefinition;

/**
 * Data-driven gameplay definition for a basic summoned combat unit.
 *
 * Cards/effects choose the runtime summon Blueprint child explicitly and apply this definition's
 * gameplay data at runtime. Mesh/presentation setup belongs on the runtime Blueprint child.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Summoned Unit Definition"))
class JARGON_API UJargonSummonedUnitDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Player-facing summon name for UI, logs, and future generation helpers."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (MultiLine = "true", ToolTip = "Short description of this summon for authoring, UI, and logs."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Visual", meta = (ToolTip = "Optional portrait or icon for future summon UI."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Animation", meta = (ToolTip = "Required looping idle animation applied to the generic summon runtime shell. This plays immediately after the definition is applied and after non-terminal one-shot animations."))
	TObjectPtr<UAnimationAsset> IdleAnimationOverride = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Animation", meta = (ToolTip = "Required one-shot basic attack animation applied to the generic summon runtime shell. The unit returns to idle after it finishes."))
	TObjectPtr<UAnimationAsset> BasicAttackAnimationOverride = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Animation", meta = (ToolTip = "Required terminal death animation applied to the generic summon runtime shell. Death does not return to idle."))
	TObjectPtr<UAnimationAsset> DeathAnimationOverride = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Stats", meta = (ClampMin = "1", ToolTip = "Summoned unit max health. Future runtime application should restore the unit to this full health on spawn."))
	int32 MaxHP = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Stats", meta = (ClampMin = "0", ToolTip = "Summoned unit movement range in combat tiles."))
	int32 MoveRange = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Stats", meta = (ClampMin = "1", ToolTip = "Summoned unit basic attack range in combat tiles."))
	int32 AttackRange = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Stats", meta = (ClampMin = "0", ToolTip = "Summoned unit basic attack damage."))
	int32 AttackDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Combat", meta = (ToolTip = "Team used when this definition is applied in a later runtime phase. Basic card summons should normally remain Player."))
	ETeam Team = ETeam::Player;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Combat", meta = (ToolTip = "Reserved for a later unit capability pass. Current runtime action logic does not consume this yet."))
	bool bCanMove = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Combat", meta = (ToolTip = "Reserved for a later unit capability pass. Current runtime action logic does not consume this yet."))
	bool bCanAttack = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Combat", meta = (ToolTip = "Whether this summon should enter with its attack already spent. Card effects can still override this timing value explicitly."))
	bool bSummonEntersWithAttackExhausted = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Abilities", meta = (ToolTip = "Reusable ability definition resolved when this summon enters combat. Leave empty for no enter-combat hook."))
	TObjectPtr<UJargonAbilityDefinition> OnSummonedAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Abilities", meta = (ToolTip = "Reusable ability definition resolved at the start of this summon side's turn. Leave empty for no turn-start hook."))
	TObjectPtr<UJargonAbilityDefinition> OnTurnStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Abilities", meta = (ToolTip = "Reusable ability definition resolved once when this summon dies. Leave empty for no death hook."))
	TObjectPtr<UJargonAbilityDefinition> OnDeathAbility = nullptr;

	UFUNCTION(BlueprintPure, Category = "Summon|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Summon|Debug")
	FString GetDebugSummary() const;

	UFUNCTION(BlueprintPure, Category = "Summon|Debug")
	FString GetAuditSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
