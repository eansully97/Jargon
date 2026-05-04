#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonHeroTypes.h"
#include "Engine/DataAsset.h"
#include "JargonArtifactDefinition.generated.h"

class UJargonHeroDefinition;
class UJargonAbilityDefinition;
class UTexture2D;

UENUM(BlueprintType)
enum class EJargonArtifactRole : uint8
{
	RunReward UMETA(DisplayName = "Run Reward"),
	ClassDefault UMETA(DisplayName = "Class Default")
};

UENUM(BlueprintType)
enum class EJargonArtifactRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Uncommon UMETA(DisplayName = "Uncommon"),
	Rare UMETA(DisplayName = "Rare"),
	Legendary UMETA(DisplayName = "Legendary")
};

/** Run-persistent Artifact that owns class/default and collected ability hooks. */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Artifact Definition"))
class JARGON_API UJargonArtifactDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact", meta = (ToolTip = "Player-facing Artifact name shown when this run-scoped passive upgrade is granted or triggered."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact", meta = (MultiLine = "true", ToolTip = "Player-facing description for this run-scoped Artifact."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact", meta = (ToolTip = "Optional Artifact icon for reward and run UI."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact", meta = (DisplayName = "Artifact Rarity", ToolTip = "Rarity tier for reward presentation and future artifact pools."))
	EJargonArtifactRarity Rarity = EJargonArtifactRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact", meta = (ToolTip = "Class Default artifacts seed a hero's base ability layout at run start. Run Reward artifacts are collected during a run."))
	EJargonArtifactRole ArtifactRole = EJargonArtifactRole::RunReward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact|Eligibility", meta = (ToolTip = "Optional hero class filter. Empty means any hero class can claim this artifact."))
	TArray<EJargonHeroClass> EligibleHeroClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact|Abilities", meta = (ToolTip = "Reusable ability definition resolved once after combatants spawn and before the first player turn starts. Leave empty for no combat-start hook."))
	TObjectPtr<UJargonAbilityDefinition> OnCombatStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact|Abilities", meta = (ToolTip = "Reusable ability definition resolved during player turn start. Leave empty for no player-turn-start hook."))
	TObjectPtr<UJargonAbilityDefinition> OnPlayerTurnStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Artifact|Abilities", meta = (ToolTip = "Reusable ability definition resolved after an enemy dies. Leave empty for no enemy-death hook."))
	TObjectPtr<UJargonAbilityDefinition> OnEnemyDeathAbility = nullptr;

	UFUNCTION(BlueprintPure, Category = "Artifact")
	bool HasAnyEffects() const;

	UFUNCTION(BlueprintPure, Category = "Artifact")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Artifact|Eligibility", meta = (ToolTip = "Returns true if this artifact can be claimed by the supplied Hero Definition. Empty eligibility filters allow any hero."))
	bool IsEligibleForHeroDefinition(const UJargonHeroDefinition* HeroDefinition) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
