#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "UObject/Object.h"
#include "JargonAbilityDefinition.generated.h"

class ABattleTileEffect;
class ABattleUnit;
class UTexture2D;
class UJargonStatusEffectDefinition;
class UJargonSummonedUnitDefinition;
class UJargonTileEffectDefinition;

#if WITH_EDITOR
class FDataValidationContext;
#endif

class UJargonAbilityDefinition;

UENUM(BlueprintType)
enum class EJargonAbilityTargetingPreset : uint8
{
	Self UMETA(DisplayName = "Self"),
	SelectedEnemy UMETA(DisplayName = "Selected Enemy"),
	SelectedAlly UMETA(DisplayName = "Selected Ally"),
	SelectedUnit UMETA(DisplayName = "Selected Unit"),
	TargetTile UMETA(DisplayName = "Target Tile"),
	EnemiesInRadius UMETA(DisplayName = "Enemies In Radius"),
	AlliesInRadius UMETA(DisplayName = "Allies In Radius"),
	UnitsInRadius UMETA(DisplayName = "Units In Radius"),
	TilesInRadius UMETA(DisplayName = "Tiles In Radius"),
	ChainEnemies UMETA(DisplayName = "Chain Enemies")
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonAbilityTargetingProfile
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ToolTip = "Readable targeting preset. This builds the underlying Delivery, Filter, Radius, and ChainCount used by the shared effect resolver."))
	EJargonAbilityTargetingPreset Preset = EJargonAbilityTargetingPreset::SelectedEnemy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ClampMin = "0", ToolTip = "Radius used by radius and chain presets.", EditCondition = "Preset == EJargonAbilityTargetingPreset::EnemiesInRadius || Preset == EJargonAbilityTargetingPreset::AlliesInRadius || Preset == EJargonAbilityTargetingPreset::UnitsInRadius || Preset == EJargonAbilityTargetingPreset::TilesInRadius || Preset == EJargonAbilityTargetingPreset::ChainEnemies", EditConditionHides))
	int32 Radius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ClampMin = "1", ToolTip = "Maximum unit count for chain presets.", EditCondition = "Preset == EJargonAbilityTargetingPreset::ChainEnemies", EditConditionHides))
	int32 ChainCount = 3;

	EJargonEffectDelivery GetDelivery() const;
	EJargonEffectTargetFilter GetTargetFilter() const;
	FString GetSummary() const;
	bool UsesRadius() const;
	bool UsesChain() const;
	void ApplyToEffectSpec(FJargonEffectSpec& Effect) const;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonAbilityCueDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation", meta = (ToolTip = "Optional short label for future cue presentation. Empty labels fall back to DisplayName."))
	FText CueLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation", meta = (ToolTip = "Optional icon for future presentation. Gameplay never depends on this."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation", meta = (ToolTip = "Optional style/type hint for Blueprint presentation. Gameplay ignores this value."))
	FName CueTypeHint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation", meta = (ToolTip = "Optional color hint for Blueprint presentation. Gameplay ignores this value."))
	FLinearColor CueColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation", meta = (MultiLine = "true", ToolTip = "Optional floating text override for future presentation. Empty text lets the cue presenter derive text from effect results."))
	FText FloatingTextOverride;

	FText GetLabelOrFallback(const FText& Fallback) const;
	FString GetAuditSummary() const;
};

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line")
class JARGON_API UJargonAbilityAction : public UObject
{
	GENERATED_BODY()

public:
	UJargonAbilityAction();

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "Ability Effect Line", meta = (ToolTip = "Readable collapsed editor label generated from this ability effect line."))
	FText EditorTitle;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const;
	virtual FString GetOperationName() const;
	virtual FString GetDeliverySummary() const;
	virtual FString GetPayloadSummary() const;
	virtual FString GetActionSummary() const;
	virtual FString GetRulesText() const;
	virtual void RefreshEditorTitle();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ValidateAction(const UJargonAbilityDefinition* AbilityDefinition, const FString& ActionLabel, FDataValidationContext& Context) const;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Damage")
class JARGON_API UJargonAbilityDamageAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityDamageAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: damage dealt to delivered targets."))
	int32 Damage = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Lifesteal Damage")
class JARGON_API UJargonAbilityLifestealDamageAction : public UJargonAbilityDamageAction
{
	GENERATED_BODY()

public:
	UJargonAbilityLifestealDamageAction();

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Heal")
class JARGON_API UJargonAbilityHealAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityHealAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: healing applied to delivered targets."))
	int32 Healing = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Shield")
class JARGON_API UJargonAbilityShieldAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityShieldAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: Shield applied to delivered targets."))
	int32 Shield = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Apply Status")
class JARGON_API UJargonAbilityStatusAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityStatusAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven status keyword definition applied by this effect line."))
	TObjectPtr<UJargonStatusEffectDefinition> StatusEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload amount. Meaning depends on the status definition."))
	int32 Amount = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Cleanse Status")
class JARGON_API UJargonAbilityCleanseStatusAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityCleanseStatusAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: optional status keyword definition to cleanse. Leave empty to cleanse all negative statuses."))
	TObjectPtr<UJargonStatusEffectDefinition> StatusEffectDefinition = nullptr;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Move")
class JARGON_API UJargonAbilityMoveAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityMoveAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: maximum tiles the source can move."))
	int32 Distance = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Push")
class JARGON_API UJargonAbilityPushAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityPushAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: tiles to push delivered targets."))
	int32 Distance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "0", ToolTip = "Payload: damage dealt if the push collides with blocked movement."))
	int32 CollisionDamage = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Pull")
class JARGON_API UJargonAbilityPullAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityPullAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: tiles to pull delivered targets."))
	int32 Distance = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Summon")
class JARGON_API UJargonAbilitySummonAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilitySummonAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven summon definition."))
	TObjectPtr<UJargonSummonedUnitDefinition> SummonedUnitDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: required runtime Blueprint child used by SummonUnit."))
	TSubclassOf<ABattleUnit> RuntimeSummonedUnitClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: whether the summoned unit enters with its attack already spent."))
	bool bSummonEntersWithAttackExhausted = true;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Place Tile Effect")
class JARGON_API UJargonAbilityPlaceTileEffectAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityPlaceTileEffectAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven trap/aura definition."))
	TObjectPtr<UJargonTileEffectDefinition> TileEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: required runtime Blueprint child used by PlaceTileEffect."))
	TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: card category represented by this tile effect when the ability is not sourced by a card."))
	ECardCategory TileEffectCategory = ECardCategory::Trap;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Destroy Tile Effect")
class JARGON_API UJargonAbilityDestroyTileEffectAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityDestroyTileEffectAction();

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Draw")
class JARGON_API UJargonAbilityDrawCardsAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityDrawCardsAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: number of cards to draw."))
	int32 Count = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Gain Energy")
class JARGON_API UJargonAbilityGainEnergyAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityGainEnergyAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: Energy gained."))
	int32 Amount = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Ability Effect Line - Gain Element Charge")
class JARGON_API UJargonAbilityGainElementChargeAction : public UJargonAbilityAction
{
	GENERATED_BODY()

public:
	UJargonAbilityGainElementChargeAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ToolTip = "Payload: element charge type to gain."))
	EJargonElementType ElementType = EJargonElementType::Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: number of charges gained."))
	int32 Amount = 1;

	virtual void BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual FString GetOperationName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, meta = (DisplayName = "Jargon Ability Definition"))
class JARGON_API UJargonAbilityDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (DisplayPriority = "1", ToolTip = "Player-facing ability name used by future hooks, logs, cues, and audit output."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (DisplayPriority = "2", MultiLine = "true", ToolTip = "Authoring description for this reusable non-card ability."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (DisplayPriority = "3", MultiLine = "true", ToolTip = "Player-facing rules text. Leave empty only for internal/debug abilities."))
	FText RulesText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Trigger", meta = (ToolTip = "Expected trigger context for validation and future hook wiring. This does not auto-register the ability yet."))
	EJargonEffectTrigger ExpectedTrigger = EJargonEffectTrigger::Activated;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ToolTip = "Readable targeting profile. Ability effect lines use this profile unless they are explicitly self-only operations such as draw, energy, or element charge gain."))
	FJargonAbilityTargetingProfile TargetingProfile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation", meta = (ToolTip = "Optional presentation metadata for future cue widgets and floating text. It never changes gameplay resolution."))
	FJargonAbilityCueDefinition CueDefinition;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Ability|Effects", meta = (TitleProperty = "EditorTitle", ToolTip = "Readable ability effect lines. Each line builds one or more FJargonEffectSpec entries for the shared executor/resolver pipeline."))
	TArray<TObjectPtr<UJargonAbilityAction>> Actions;

	void BuildEffectSpecs(TArray<FJargonEffectSpec>& OutEffects) const;

	UFUNCTION(BlueprintPure, Category = "Ability|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Ability|Debug")
	FString GetAuditSummary() const;

	UFUNCTION(BlueprintPure, Category = "Ability|Presentation")
	FText GetExecutionLabel() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
