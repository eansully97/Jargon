#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "JargonSummonedUnitDefinition.generated.h"

class ABattleUnit;
class UTexture2D;

/**
 * Data-driven gameplay definition for a basic summoned combat unit.
 *
 * Definition-backed summon effects can spawn a generic summon unit and apply this data at runtime.
 * Existing UnitClass summon cards remain supported as the legacy/special-case path.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Summoned Unit Definition"))
class JARGON_API UJargonSummonedUnitDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Player-facing summon name for UI, logs, and future generation helpers."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (MultiLine = "true", ToolTip = "Short description of this summon for authoring, UI, and logs."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Visual", meta = (ToolTip = "Optional portrait or icon for future summon UI. Runtime mesh/animation visuals are still owned by the spawned unit Blueprint."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Runtime", meta = (ToolTip = "Optional special unit class for summons that still need unique Blueprint visuals or behavior. Leave empty to use the CombatGameMode DefaultSummonedUnitClass."))
	TSubclassOf<ABattleUnit> OptionalUnitClassOverride;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Combat", meta = (ToolTip = "Whether this summon should enter with its attack already spent. Card effect overrides/legacy flags remain supported."))
	bool bSummonEntersWithAttackExhausted = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Effects", meta = (ToolTip = "Shared effects appended to the spawned unit before OnSummonedEffects execute."))
	TArray<FJargonEffectSpec> OnSummonedEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Effects", meta = (ToolTip = "Shared effects appended to the spawned unit and resolved at the start of this summon side's turn."))
	TArray<FJargonEffectSpec> OnTurnStartEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon|Effects", meta = (ToolTip = "Shared effects appended to the spawned unit and resolved once when this summon dies."))
	TArray<FJargonEffectSpec> OnDeathEffects;

	UFUNCTION(BlueprintPure, Category = "Summon|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Summon|Debug")
	FString GetDebugSummary() const;

	UFUNCTION(BlueprintPure, Category = "Summon|Debug")
	FString GetAuditSummary() const;
};
