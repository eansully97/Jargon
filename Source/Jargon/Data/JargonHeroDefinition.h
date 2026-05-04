#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonHeroTypes.h"
#include "Engine/DataAsset.h"
#include "JargonHeroDefinition.generated.h"

class USkeletalMesh;
class UTexture2D;
class UJargonAbilityDefinition;

USTRUCT(BlueprintType)
struct JARGON_API FJargonHeroClassPassiveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Passive", meta = (ToolTip = "Player-facing passive name used by combat cues and HUD text. Empty names fall back to the hero display name."))
	FText PassiveName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Passive", meta = (ToolTip = "Preferred reusable ability definition for this passive hook. When assigned at runtime, it replaces the raw Effects array for this hook."))
	TObjectPtr<UJargonAbilityDefinition> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Passive", meta = (TitleProperty = "Operation", ToolTip = "Effects resolved when this passive hook fires. Empty arrays intentionally mean this passive does nothing."))
	TArray<FJargonEffectSpec> Effects;

	bool HasEffects() const
	{
		return Ability != nullptr || Effects.Num() > 0;
	}
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonHeroAspectDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect", meta = (ToolTip = "Aspect identity locked when this element transformation triggers."))
	EJargonHeroAspect Aspect = EJargonHeroAspect::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect", meta = (ToolTip = "Element that must reach the combat element charge cap to lock this transformation. The current threshold is 10 charges."))
	EJargonElementType RequiredElement = EJargonElementType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect", meta = (ToolTip = "Player-facing aspect name shown by HUD and combat cues. Empty names fall back to the aspect enum display name."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect", meta = (MultiLine = "true", ToolTip = "Player-facing aspect description shown by HUD."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Transformation", meta = (ToolTip = "Player-facing name for the one-shot transformation moment. Empty names fall back to the aspect display name."))
	FText TransformationName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Transformation", meta = (ToolTip = "Preferred reusable ability definition resolved once when this aspect transforms. When assigned at runtime, it replaces TransformationEffects."))
	TObjectPtr<UJargonAbilityDefinition> TransformationAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Transformation", meta = (TitleProperty = "Operation", ToolTip = "Effects resolved once when this aspect transforms. Empty arrays intentionally mean the transformation only locks the aspect."))
	TArray<FJargonEffectSpec> TransformationEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Turn Start", meta = (ToolTip = "Name for the aspect passive that fires at player turn start while this aspect is active."))
	FText TurnStartPassiveName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Turn Start", meta = (ToolTip = "Preferred reusable ability definition for the turn-start aspect passive. When assigned at runtime, it replaces TurnStartEffects."))
	TObjectPtr<UJargonAbilityDefinition> TurnStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Turn Start", meta = (TitleProperty = "Operation", ToolTip = "Effects resolved at player turn start while this aspect is active. Empty arrays intentionally mean no turn-start aspect passive."))
	TArray<FJargonEffectSpec> TurnStartEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Enemy Death", meta = (ToolTip = "Name for the aspect passive that fires when an enemy dies while this aspect is active."))
	FText EnemyDeathPassiveName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Enemy Death", meta = (ToolTip = "Preferred reusable ability definition for the enemy-death aspect passive. When assigned at runtime, it replaces EnemyDeathEffects."))
	TObjectPtr<UJargonAbilityDefinition> EnemyDeathAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspect|Enemy Death", meta = (TitleProperty = "Operation", ToolTip = "Effects resolved when an enemy dies while this aspect is active. Empty arrays intentionally mean no enemy-death aspect passive."))
	TArray<FJargonEffectSpec> EnemyDeathEffects;

	bool HasAnyPassiveEffects() const
	{
		return TurnStartAbility != nullptr || EnemyDeathAbility != nullptr || TurnStartEffects.Num() > 0 || EnemyDeathEffects.Num() > 0;
	}

	bool HasAnyTransformationOrPassiveEffects() const
	{
		return TransformationAbility != nullptr || TransformationEffects.Num() > 0 || HasAnyPassiveEffects();
	}
};

/**
 * Base hero identity, combat stats, passives, and elemental aspect tuning.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Hero Definition"))
class JARGON_API UJargonHeroDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (DisplayPriority = "1", ToolTip = "Player-facing hero name shown in UI. This does not change gameplay by itself."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (DisplayPriority = "2", ToolTip = "Simple RPG class identity for this hero. Current supported classes are Mage, Rogue, and Paladin. Legacy hidden classes are mapped safely if old assets still use them."))
	EJargonHeroClass HeroClass = EJargonHeroClass::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (DisplayName = "Skeletal Mesh Override", DisplayPriority = "3", ToolTip = "Optional single mesh override used by both the Town/Exploration character and combat player unit. Leave empty to keep Blueprint defaults."))
	TObjectPtr<USkeletalMesh> HeroSkeletalMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "1", DisplayName = "Max HP", DisplayPriority = "1", ToolTip = "Player hero maximum health in combat. This is the authoritative player stat and overrides the player battle unit Blueprint default."))
	int32 MaxHP = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "0", DisplayPriority = "2", ToolTip = "Player hero movement range in combat tiles. This is applied to the player battle unit at combat start."))
	int32 MoveRange = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "1", DisplayPriority = "3", ToolTip = "Player hero basic attack range in combat tiles. This is applied to the player battle unit at combat start."))
	int32 AttackRange = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "0", DisplayPriority = "4", ToolTip = "Player hero basic attack damage. This is applied to the player battle unit at combat start."))
	int32 AttackDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Optional UI", meta = (MultiLine = "true", AdvancedDisplay, ToolTip = "Optional flavor text for menus or future hero selection UI. It does not affect gameplay."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Optional UI", meta = (AdvancedDisplay, ToolTip = "Optional portrait for future UI. It is not used by combat yet."))
	TObjectPtr<UTexture2D> Portrait = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Passives", meta = (DisplayName = "Combat Start Passive", ToolTip = "Class passive resolved once on the first player turn. Empty effects mean this hero has no combat-start passive."))
	FJargonHeroClassPassiveDefinition CombatStartPassive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Passives", meta = (DisplayName = "Player Turn Start Passive", ToolTip = "Class passive resolved at the start of each player turn. Empty effects mean this hero has no turn-start passive."))
	FJargonHeroClassPassiveDefinition PlayerTurnStartPassive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Aspects", meta = (TitleProperty = "Aspect", ToolTip = "Aspect transformations this hero can enter when the matching element reaches the combat element charge cap. Empty arrays mean this hero has no aspect transformations."))
	TArray<FJargonHeroAspectDefinition> HeroAspects;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hero|Preset", meta = (ToolTip = "Seeds recommended stats for the selected Hero Class. This is only an authoring shortcut; you can freely tune the values afterward."))
	void ApplyRecommendedClassPreset();

	UFUNCTION(BlueprintPure, Category = "Hero|Validation", meta = (ToolTip = "Returns true when required combat stat values are usable. This is a backend validation helper, not an editor workflow requirement."))
	bool IsValidDefinition() const;

	const FJargonHeroAspectDefinition* FindAspectDefinitionByAspect(EJargonHeroAspect Aspect) const;
	const FJargonHeroAspectDefinition* FindAspectDefinitionForElement(EJargonElementType Element) const;

	const TArray<FJargonHeroAspectDefinition>& GetAuthoredAspectDefinitions() const
	{
		return HeroAspects;
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
