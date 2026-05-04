#pragma once

#include "CoreMinimal.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Units/SummonedBattleUnit.h"
#include "Core/JargonTypes.h"
#include "UObject/Object.h"
#include "CardScriptDefinition.generated.h"

class UCardDefinition;
class UJargonStatusEffectDefinition;
class UJargonSummonedUnitDefinition;
class UJargonTileEffectDefinition;
enum class EJargonEffectOperation : uint8;
struct FJargonEffectSpec;

#if WITH_EDITOR
class FDataValidationContext;
#endif

UENUM(BlueprintType)
enum class EJargonCardKeyword : uint8
{
	None UMETA(DisplayName = "None"),
	Damage UMETA(DisplayName = "Damage"),
	Heal UMETA(DisplayName = "Heal"),
	Shield UMETA(DisplayName = "Shield"),
	Status UMETA(DisplayName = "Status"),
	Move UMETA(DisplayName = "Move"),
	Push UMETA(DisplayName = "Push"),
	Pull UMETA(DisplayName = "Pull"),
	Summon UMETA(DisplayName = "Summon"),
	PlaceTileEffect UMETA(DisplayName = "Place Tile Effect"),
	DestroyTileEffect UMETA(DisplayName = "Destroy Tile Effect"),
	Draw UMETA(DisplayName = "Draw"),
	GainEnergy UMETA(DisplayName = "Gain Energy"),
	GainElement UMETA(DisplayName = "Gain Element"),
	Chain UMETA(DisplayName = "Chain"),
	CleanseStatus UMETA(DisplayName = "Cleanse Status"),
	Lifesteal UMETA(DisplayName = "Lifesteal")
};

/** Focused authoring options for the Chain card action before it becomes shared effect specs. */
UENUM(BlueprintType)
enum class EJargonCardChainActionType : uint8
{
	Damage UMETA(DisplayName = "Damage"),
	Heal UMETA(DisplayName = "Heal"),
	Stun UMETA(DisplayName = "Status")
};

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Card Effect Line")
class JARGON_API UJargonCardAction : public UObject
{
	GENERATED_BODY()

public:
	UJargonCardAction();

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "Effect Line", meta = (ToolTip = "Readable collapsed editor label generated from this effect line's operation, delivery, and payload values."))
	FText EditorTitle;

	/** Converts this designer-facing card effect line into shared runtime effect specs. */
	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const;

	/** Audit helper used by CardDefinition and editor reports to find authored operations. */
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const;
	virtual EJargonCardKeyword GetKeyword() const;
	virtual FString GetKeywordName() const;
	virtual FString GetDeliverySummary() const;
	virtual FString GetPayloadSummary() const;
	virtual FString GetActionSummary() const;
	virtual FString GetRulesText() const;
	virtual void RefreshEditorTitle();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Damage")
class JARGON_API UJargonCardDamageAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardDamageAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: damage dealt to the delivered target or each enemy in the radius."))
	int32 Damage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 means the chosen enemy only. Higher values deliver the damage to enemies around the chosen tile."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Lifesteal")
class JARGON_API UJargonCardLifestealAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardLifestealAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: damage dealt to the delivered enemy target or each enemy in the radius. The source heals for unblocked HP damage dealt."))
	int32 Damage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 means the chosen enemy only. Higher values deliver the lifesteal damage to enemies around the chosen tile."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Heal")
class JARGON_API UJargonCardHealAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardHealAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: healing applied to the delivered friendly target or each ally in the radius."))
	int32 Healing = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 means the chosen ally or self fallback. Higher values deliver healing to allies around the chosen tile."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Shield")
class JARGON_API UJargonCardShieldAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardShieldAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: temporary Shield applied to the delivered friendly target or each ally in the radius."))
	int32 Shield = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 means the chosen ally or self fallback. Higher values deliver Shield to allies around the chosen tile."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Apply Status")
class JARGON_API UJargonCardStatusAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardStatusAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven status keyword definition applied by this effect line. Existing status enum data is migration scaffolding only."))
	TObjectPtr<UJargonStatusEffectDefinition> StatusEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload amount. Stun/Freeze/Root use turns. Burn uses stacks. Vulnerable uses next-hit bonus damage."))
	int32 Amount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 means the chosen enemy only. Higher values deliver the status to enemies around the chosen tile."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Cleanse Status")
class JARGON_API UJargonCardCleanseStatusAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardCleanseStatusAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: optional status keyword definition to cleanse. Leave empty to cleanse all negative statuses."))
	TObjectPtr<UJargonStatusEffectDefinition> StatusEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 means the chosen ally or self fallback. Higher values cleanse allies around the chosen tile."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Move")
class JARGON_API UJargonCardMoveSelfAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardMoveSelfAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: maximum tiles the source can move to the chosen tile."))
	int32 Distance = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Push")
class JARGON_API UJargonCardPushAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardPushAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: tiles to push the delivered enemy away from the source."))
	int32 Distance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "0", ToolTip = "Payload: damage dealt if the push collides with blocked movement."))
	int32 CollisionDamage = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Pull")
class JARGON_API UJargonCardPullAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardPullAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: tiles to pull the delivered enemy toward the source."))
	int32 Distance = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Summon")
class JARGON_API UJargonCardSummonAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardSummonAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven summon definition. Cards no longer author summon unit classes directly."))
	TObjectPtr<UJargonSummonedUnitDefinition> SummonedUnitDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: required runtime Blueprint child of the summon unit shell. The card explicitly chooses the presentation/runtime actor; the definition supplies gameplay data."))
	TSubclassOf<ASummonedBattleUnit> RuntimeSummonedUnitClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: whether the summoned unit enters with its attack already spent."))
	bool bSummonEntersWithAttackExhausted = true;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Place Tile Effect")
class JARGON_API UJargonCardPlaceTileEffectAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardPlaceTileEffectAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven trap/aura definition. Cards no longer author tile effect Blueprint classes directly."))
	TObjectPtr<UJargonTileEffectDefinition> TileEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: required runtime Blueprint child used to place this trap or aura. Trap definitions should use a trap shell child; aura definitions should use an aura shell child."))
	TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Destroy Tile Effect")
class JARGON_API UJargonCardDestroyTileEffectAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardDestroyTileEffectAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "0", ToolTip = "Delivery: 0 destroys the targeted tile effect only. Higher values destroy opposing tile effects in a radius."))
	int32 Radius = 0;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Draw")
class JARGON_API UJargonCardDrawCardsAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardDrawCardsAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: number of cards to draw."))
	int32 Count = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Gain Energy")
class JARGON_API UJargonCardGainEnergyAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardGainEnergyAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: Energy gained after paying the card cost."))
	int32 Amount = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Gain Element")
class JARGON_API UJargonCardGainElementChargeAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardGainElementChargeAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: element charge type to gain."))
	EJargonElementType ElementType = EJargonElementType::Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload: number of charges gained."))
	int32 Amount = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Effect Line - Chain")
class JARGON_API UJargonCardChainAction : public UJargonCardAction
{
	GENERATED_BODY()

public:
	UJargonCardChainAction();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Operation applied by each chain jump. Status-like chain entries should migrate toward ApplyStatus plus a status definition."))
	EJargonCardChainActionType ChainType = EJargonCardChainActionType::Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ToolTip = "Payload: required data-driven status keyword definition when ChainType is Status.", EditCondition = "ChainType == EJargonCardChainActionType::Stun", EditConditionHides))
	TObjectPtr<UJargonStatusEffectDefinition> StatusEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Payload", meta = (ClampMin = "1", ToolTip = "Payload amount: damage, healing, or stun duration applied to each chain target."))
	int32 Amount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "1", ToolTip = "Delivery: maximum units affected, including the initial target."))
	int32 ChainCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect Line|Delivery", meta = (ClampMin = "1", ToolTip = "Delivery: search radius for each chain jump."))
	int32 Radius = 1;

	virtual void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const override;
	virtual bool HasRuntimeOperation(EJargonEffectOperation Operation) const override;
	virtual EJargonCardKeyword GetKeyword() const override;
	virtual FString GetKeywordName() const override;
	virtual FString GetDeliverySummary() const override;
	virtual FString GetPayloadSummary() const override;
	virtual FString GetActionSummary() const override;
	virtual FString GetRulesText() const override;

#if WITH_EDITOR
	virtual bool ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const override;
#endif
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonCardElementalBonusScript
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ToolTip = "Readable collapsed editor label generated from this bonus group's effect lines."))
	FText EditorTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ToolTip = "Element charge type required for this optional bonus to trigger."))
	EJargonElementType ElementType = EJargonElementType::Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ClampMin = "1", ToolTip = "Charges required before the bonus can resolve."))
	int32 RequiredCharges = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ToolTip = "If true, charges are spent before the bonus effect lines resolve. If the bonus fails, charges are refunded by CardResolver."))
	bool bSpendCharges = true;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Elemental Bonus", meta = (TitleProperty = "EditorTitle", ToolTip = "Focused effect lines resolved after base card effect lines when this bonus triggers."))
	TArray<TObjectPtr<UJargonCardAction>> Actions;

	/** Returns whether this manually chosen bonus group has any authored effect lines. */
	bool HasAnyActions() const;

	/** Builds specs only after the player manually chooses this bonus group. */
	void BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const;
	bool HasRuntimeOperation(EJargonEffectOperation Operation) const;
	FString GetBonusSummary() const;
	FString GetRulesText() const;
	void RefreshEditorTitle();

#if WITH_EDITOR
	bool ValidateBonus(const UCardDefinition* Card, int32 BonusIndex, FDataValidationContext& Context) const;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName = "Card Effect Script")
class JARGON_API UJargonCardScript : public UObject
{
	GENERATED_BODY()

public:
	/** Base card effect lines resolved every time the card successfully plays. */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Card Effects|Base", meta = (TitleProperty = "EditorTitle", ToolTip = "Base card effect lines in resolve order. Each line exposes only the operation, delivery, and payload fields it uses."))
	TArray<TObjectPtr<UJargonCardAction>> Actions;

	/** Optional manual elemental bonus groups shown by the assigned choice widget when affordable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Effects|Elemental Bonuses", meta = (TitleProperty = "EditorTitle", ToolTip = "Optional elemental payoff groups using focused effect-line lists."))
	TArray<FJargonCardElementalBonusScript> ElementalBonuses;

	/** Returns whether this script has any base card effect lines. */
	bool HasAnyActions() const;

	/** Shared conversion boundary from card-authored base lines into FJargonEffectSpec arrays. */
	void BuildBaseEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const;
	bool BuildElementalBonusEffectSpecs(const UCardDefinition* Card, int32 BonusIndex, TArray<FJargonEffectSpec>& OutEffects) const;
	bool HasRuntimeOperation(EJargonEffectOperation Operation) const;
	FString GetScriptSummary() const;
	FString GetRulesText() const;
	void RefreshEditorTitles();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	bool ValidateScript(const UCardDefinition* Card, FDataValidationContext& Context) const;
#endif
};
