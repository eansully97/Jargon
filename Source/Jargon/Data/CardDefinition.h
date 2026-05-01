// CardDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "CardDefinition.generated.h"

class ABattleUnit;
class UJargonSummonedUnitDefinition;

UENUM(BlueprintType)
enum class ECardEffectOperation : uint8
{
	None UMETA(DisplayName = "None"),
	DealDamage UMETA(DisplayName = "Deal Damage"),
	Heal UMETA(DisplayName = "Heal"),
	ApplyShield UMETA(DisplayName = "Apply Shield"),
	ApplyStun UMETA(DisplayName = "Apply Stun"),
	MoveSelf UMETA(DisplayName = "Move Self"),
	PushTarget UMETA(DisplayName = "Push Target"),
	PullTarget UMETA(DisplayName = "Pull Target"),
	SummonUnit UMETA(DisplayName = "Summon Unit"),
	PlaceTileEffect UMETA(DisplayName = "Place Tile Effect"),
	DrawCards UMETA(DisplayName = "Draw Cards"),
	GainEnergy UMETA(DisplayName = "Gain Energy"),
	ChainDamage UMETA(DisplayName = "Chain Damage"),
	ChainHeal UMETA(DisplayName = "Chain Heal"),
	ChainStun UMETA(DisplayName = "Chain Stun"),
	DestroyTileEffect UMETA(DisplayName = "Destroy Tile Effect"),
	GainElementCharge UMETA(DisplayName = "Gain Element Charge"),
	ApplyFreeze UMETA(DisplayName = "Apply Freeze")
};

/**
 * Lightweight data-authored card effect operation.
 *
 * Author card behavior by adding one or more effect specs. Examples:
 * - Firebolt: DealDamage with Value.
 * - Shield Bash: DealDamage, then PushTarget.
 * - Dash: MoveSelf.
 * - Orc Summon: SummonUnit with UnitClass.
 * - // Spike Trap / Area of Aegis: PlaceTileEffect with TileEffectClass, Value, and EffectRadius if needed.
 */
USTRUCT(BlueprintType)
struct JARGON_API FCardEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ToolTip = "The keyword-style operation this effect performs."))
	ECardEffectOperation Operation = ECardEffectOperation::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0", ToolTip = "Primary numeric amount. DealDamage = damage, Heal = healing, ApplyShield = shield, ApplyStun/ApplyFreeze = turns, DrawCards = cards, GainEnergy = energy, GainElementCharge = element charges. PlaceTileEffect uses this as an optional tile-effect payload, such as trap damage or aura shield amount.", EditCondition = "Operation == ECardEffectOperation::DealDamage || Operation == ECardEffectOperation::Heal || Operation == ECardEffectOperation::ApplyShield || Operation == ECardEffectOperation::PlaceTileEffect || Operation == ECardEffectOperation::DrawCards || Operation == ECardEffectOperation::GainEnergy || Operation == ECardEffectOperation::GainElementCharge || Operation == ECardEffectOperation::ApplyStun || Operation == ECardEffectOperation::ApplyFreeze || Operation == ECardEffectOperation::ChainDamage || Operation == ECardEffectOperation::ChainHeal || Operation == ECardEffectOperation::ChainStun", EditConditionHides))
	int32 Value = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element", meta = (ToolTip = "Element charge type gained by GainElementCharge.", EditCondition = "Operation == ECardEffectOperation::GainElementCharge", EditConditionHides))
	EJargonElementType ElementType = EJargonElementType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0", ToolTip = "How many tiles this MoveSelf effect can move the caster.", EditCondition = "Operation == ECardEffectOperation::MoveSelf", EditConditionHides))
	int32 MoveDistance = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "0", ToolTip = "Area radius around the chosen target for AoE effects. For Chain effects, this is the jump/search radius from the previous chained unit. 0 means target only for normal effects.", EditCondition = "Operation == ECardEffectOperation::DestroyTileEffect || Operation == ECardEffectOperation::DealDamage || Operation == ECardEffectOperation::Heal || Operation == ECardEffectOperation::ApplyShield || Operation == ECardEffectOperation::ApplyStun || Operation == ECardEffectOperation::ApplyFreeze || Operation == ECardEffectOperation::PlaceTileEffect || Operation == ECardEffectOperation::ChainDamage || Operation == ECardEffectOperation::ChainHeal || Operation == ECardEffectOperation::ChainStun", EditConditionHides))
	int32 EffectRadius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chain", meta = (ClampMin = "1", ToolTip = "Maximum number of units this chain effect can affect, including the initial target.", EditCondition = "Operation == ECardEffectOperation::ChainDamage || Operation == ECardEffectOperation::ChainHeal || Operation == ECardEffectOperation::ChainStun", EditConditionHides))
	int32 ChainCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1", ToolTip = "Push distance for PushTarget.", EditCondition = "Operation == ECardEffectOperation::PushTarget", EditConditionHides))
	int32 PushDistance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1", ToolTip = "Reserved for a future PullTarget implementation. PullTarget currently logs a warning and does not resolve.", EditCondition = "Operation == ECardEffectOperation::PullTarget", EditConditionHides))
	int32 PullDistance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0", ToolTip = "Damage dealt when pushed movement collides with an obstruction.", EditCondition = "Operation == ECardEffectOperation::PushTarget", EditConditionHides))
	int32 CollisionDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Unit class used by SummonUnit.", EditCondition = "Operation == ECardEffectOperation::SummonUnit", EditConditionHides))
	TSubclassOf<ABattleUnit> UnitClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Optional data-driven summon definition. If set, runtime uses this definition before falling back to legacy UnitClass behavior.", EditCondition = "Operation == ECardEffectOperation::SummonUnit", EditConditionHides))
	TObjectPtr<UJargonSummonedUnitDefinition> SummonedUnitDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Whether a summoned unit enters with its attack already spent. This keeps summon tempo data-authorable.", EditCondition = "Operation == ECardEffectOperation::SummonUnit", EditConditionHides))
	bool bSummonEntersWithAttackExhausted = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (ToolTip = "Tile effect actor class used by PlaceTileEffect.", EditCondition = "Operation == ECardEffectOperation::PlaceTileEffect", EditConditionHides))
	TSubclassOf<ABattleTileEffect> TileEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
	ClampMin = "0",
	ToolTip = "How many player turn starts this placed tile effect lasts. 0 means infinite.",
	EditCondition = "Operation == ECardEffectOperation::PlaceTileEffect",
	EditConditionHides))
	int32 TileEffectDuration = 0;
};

USTRUCT(BlueprintType)
struct JARGON_API FCardElementalBonusGroup
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ToolTip = "Element charge type required for this optional bonus to trigger."))
	EJargonElementType ElementType = EJargonElementType::Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ClampMin = "1", ToolTip = "How many charges of the selected element are required."))
	int32 RequiredCharges = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ToolTip = "If true, the required element charges are spent before the bonus effects resolve. If false, the charges are only checked."))
	bool bSpendCharges = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elemental Bonus", meta = (ToolTip = "Extra effects resolved after the base card effects when the element condition is met. These use the same target context as the card."))
	TArray<FCardEffectSpec> BonusEffects;
};

UCLASS(BlueprintType)
class JARGON_API UCardDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ToolTip = "Name shown on the card in UI and used by the art prompt generator."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = "true", ToolTip = "Short rules/flavor text shown on the card and included in generated art prompts."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0", ToolTip = "Normal Energy cost to play this card. Element charges are optional combo resources and do not replace this cost."))
	int32 Cost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ToolTip = "Broad card type used by UI, validation, prompt text, and tile-effect placement semantics."))
	ECardCategory Category = ECardCategory::Spell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ToolTip = "What kind of target the card asks the player to choose. Runtime effects still come from Effects[]."))
	ECardTargetType TargetType = ECardTargetType::Unit;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0", ToolTip = "Maximum targeting distance from the player unit for cards that target units or tiles."))
	int32 Range = 3;

	/** Primary card behavior.||| Add effect specs in the order they should resolve |||. Cards with no Effects will not resolve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effects", meta = (ToolTip = "Primary card behavior. Add keyword-style effects in resolve order. Cards with no Effects log a warning and do not resolve."))
	TArray<FCardEffectSpec> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Elemental Bonuses", meta = (ToolTip = "Optional combo bonuses. The card remains playable without these charges; matching groups trigger after base Effects resolve."))
	TArray<FCardElementalBonusGroup> ElementalBonusGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual", meta = (ToolTip = "Manually assigned card artwork. The prompt generator never creates, imports, or assigns this texture."))
	TObjectPtr<UTexture2D> CardArt = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|Manual", meta = (ToolTip = "If enabled, the Generate Card Art Prompt editor button writes a fantasy card art prompt to Saved/CardArtPrompts. Disable this on cards where no prompt is needed. This never creates, imports, or assigns art."))
	bool bGenerateArtPrompt = true;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Debug|Manual", meta = (DisplayName = "Generate Card Art Prompt", ToolTip = "Writes a polished fantasy card art prompt to Saved/CardArtPrompts and logs the full prompt. CardArt remains blank/manual."))
	void GenerateCardArtPrompt() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|Manual", meta = (ToolTip = "Explicit opt-in for the Generate Summon Unit Definition editor button. Keeps non-summon cards and accidental clicks from creating assets."))
	bool bGenerateSummonUnitDefinition = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|Manual", meta = (ToolTip = "Content folder where generated summon definition Data Assets are created. Uses Unreal long package paths, for example /Game/Jargon/Data/SummonedUnits."))
	FString SummonDefinitionOutputFolder = TEXT("/Game/Jargon/Data/SummonedUnits");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|Manual", meta = (ToolTip = "When true, the generated summon definition is assigned back to the selected SummonUnit effect. UnitClass is preserved as fallback."))
	bool bAssignGeneratedSummonDefinition = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|Manual", meta = (AdvancedDisplay, ToolTip = "When true, the button may reuse an existing matching summon definition asset or replace this card's existing definition reference. Existing definition asset fields are not overwritten."))
	bool bReplaceExistingSummonDefinition = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Debug|Manual", meta = (DisplayName = "Generate Summon Unit Definition", ToolTip = "Creates a matching UJargonSummonedUnitDefinition asset for one SummonUnit effect and optionally assigns it back to this card. Editor-only; never overwrites by default."))
	void GenerateSummonUnitDefinition();

	UFUNCTION(BlueprintPure, Category = "Card")
	bool UsesBoardTileTargeting() const
	{
		return TargetType != ECardTargetType::Self;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool RequiresUnitOnTargetTile() const
	{
		return TargetType == ECardTargetType::Unit;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool CreatesPersistentTileEffect() const
	{
		return HasEffectOperation(ECardEffectOperation::PlaceTileEffect);
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool RequiresEmptyTargetTile() const
	{
		return HasEffectOperation(ECardEffectOperation::SummonUnit)
			|| HasEffectOperation(ECardEffectOperation::PlaceTileEffect)
			|| HasEffectOperation(ECardEffectOperation::MoveSelf);
	}

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool UsesEffectSpecs() const
	{
		return Effects.Num() > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool HasEffectOperation(ECardEffectOperation Operation) const;

	UFUNCTION(BlueprintPure, Category = "Card")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Card|Audit")
	FString GetEffectAuditSummary(const FCardEffectSpec& EffectSpec) const;

	UFUNCTION(BlueprintPure, Category = "Card|Audit")
	FString GetElementalBonusAuditSummary(const FCardElementalBonusGroup& BonusGroup) const;

	UFUNCTION(BlueprintPure, Category = "Card|Audit")
	FString GetAuditSummary() const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	ECardEffectOperation GetPrimaryEffectOperation() const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	int32 GetConfiguredRangeForEffect(const FCardEffectSpec& EffectSpec) const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	int32 GetConfiguredRadiusForEffect(const FCardEffectSpec& EffectSpec) const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	int32 GetConfiguredValueForEffect(const FCardEffectSpec& EffectSpec) const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	int32 GetConfiguredPushDistanceForEffect(const FCardEffectSpec& EffectSpec) const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	int32 GetConfiguredCollisionDamageForEffect(const FCardEffectSpec& EffectSpec) const;
};
