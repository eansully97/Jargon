#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonHeroTypes.h"
#include "Engine/DataAsset.h"
#include "JargonRelicDefinition.generated.h"

class UJargonHeroDefinition;
class UJargonAbilityDefinition;
class UTexture2D;

UENUM(BlueprintType)
enum class EJargonRelicRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Uncommon UMETA(DisplayName = "Uncommon"),
	Rare UMETA(DisplayName = "Rare"),
	Legendary UMETA(DisplayName = "Legendary")
};

/**
 * Run-persistent Hero Boon that resolves shared Jargon effects during combat.
 *
 * The UJargonRelicDefinition name and serialized fields are retained for
 * compatibility while the editor-facing concept moves toward Hero Boons.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Hero Boon Definition"))
class JARGON_API UJargonRelicDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon", meta = (ToolTip = "Player-facing Hero Boon name shown when this run-scoped passive upgrade is granted or triggered."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon", meta = (MultiLine = "true", ToolTip = "Player-facing description for this run-scoped Hero Boon."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon", meta = (ToolTip = "Optional Hero Boon icon for reward and run UI."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon", meta = (DisplayName = "Boon Rarity", ToolTip = "Rarity tier for reward presentation and future boon pools."))
	EJargonRelicRarity Rarity = EJargonRelicRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon|Eligibility", meta = (ToolTip = "Optional hero class filter. Empty means any hero class can claim this boon."))
	TArray<EJargonHeroClass> EligibleHeroClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon|Eligibility", meta = (ToolTip = "Optional hero aspect-kit filter. Empty means any hero aspect kit can claim this boon. Eligibility checks aspects authored on the active Hero Definition, not the currently active combat aspect."))
	TArray<EJargonHeroAspect> EligibleHeroAspects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon|Abilities", meta = (ToolTip = "Reusable ability definition resolved once after combatants spawn and before the first player turn starts. Leave empty for no combat-start hook."))
	TObjectPtr<UJargonAbilityDefinition> OnCombatStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon|Abilities", meta = (ToolTip = "Reusable ability definition resolved during player turn start. Leave empty for no player-turn-start hook."))
	TObjectPtr<UJargonAbilityDefinition> OnPlayerTurnStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Boon|Abilities", meta = (ToolTip = "Reusable ability definition resolved after an enemy dies. Leave empty for no enemy-death hook."))
	TObjectPtr<UJargonAbilityDefinition> OnEnemyDeathAbility = nullptr;

	UFUNCTION(BlueprintPure, Category = "Hero Boon")
	bool HasAnyEffects() const;

	UFUNCTION(BlueprintPure, Category = "Hero Boon")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Hero Boon|Eligibility", meta = (ToolTip = "Returns true if this boon can be claimed by the supplied Hero Definition. Empty eligibility filters allow any hero."))
	bool IsEligibleForHeroDefinition(const UJargonHeroDefinition* HeroDefinition) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
