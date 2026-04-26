// CardDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "Grid/BattleTileEffect.h"
#include "CardDefinition.generated.h"

class ABattleUnit;

UENUM(BlueprintType)
enum class ECardEffectOperation : uint8
{
	None UMETA(DisplayName = "None"),
	DealDamage UMETA(DisplayName = "Deal Damage"),
	Heal UMETA(DisplayName = "Heal"),
	ApplyShield UMETA(DisplayName = "Apply Shield"),
	MoveSelf UMETA(DisplayName = "Move Self"),
	PushTarget UMETA(DisplayName = "Push Target"),
	PullTarget UMETA(DisplayName = "Pull Target"),
	SummonUnit UMETA(DisplayName = "Summon Unit"),
	PlaceTileEffect UMETA(DisplayName = "Place Tile Effect"),
	DrawCards UMETA(DisplayName = "Draw Cards"),
	GainEnergy UMETA(DisplayName = "Gain Energy")
};

/**
 * Lightweight data-authored card effect operation.
 *
 * Author card behavior by adding one or more effect specs. Examples:
 * - Firebolt: DealDamage with Value.
 * - Shield Bash: DealDamage, then PushTarget.
 * - Dash: MoveSelf.
 * - Orc Summon: SummonUnit with UnitClass.
 * - Spike Trap / Area of Aegis: PlaceTileEffect with TileEffectClass, Value, and RadiusOverride if needed.
 */
USTRUCT(BlueprintType)
struct JARGON_API FCardEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ToolTip = "The keyword-style operation this effect performs."))
	ECardEffectOperation Operation = ECardEffectOperation::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0", ToolTip = "Primary numeric amount. DealDamage = damage, Heal = healing, ApplyShield = shield, DrawCards = cards, GainEnergy = energy. PlaceTileEffect uses this as an optional tile-effect payload, such as trap damage or aura shield amount.", EditCondition = "Operation == ECardEffectOperation::DealDamage || Operation == ECardEffectOperation::Heal || Operation == ECardEffectOperation::ApplyShield || Operation == ECardEffectOperation::PlaceTileEffect || Operation == ECardEffectOperation::DrawCards || Operation == ECardEffectOperation::GainEnergy", EditConditionHides))
	int32 Value = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "-1", ToolTip = "Optional resolver-side range override for MoveSelf. -1 uses the card's Range. Card-level targeting still controls UI/legal target selection.", EditCondition = "Operation == ECardEffectOperation::MoveSelf", EditConditionHides))
	int32 RangeOverride = -1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (ClampMin = "-1", ToolTip = "Optional area radius override. -1 uses the card's Radius. 0 means single-tile/single-target where supported.", EditCondition = "Operation == ECardEffectOperation::DealDamage || Operation == ECardEffectOperation::Heal || Operation == ECardEffectOperation::ApplyShield || Operation == ECardEffectOperation::PlaceTileEffect", EditConditionHides))
	int32 RadiusOverride = -1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1", ToolTip = "Push distance for PushTarget.", EditCondition = "Operation == ECardEffectOperation::PushTarget", EditConditionHides))
	int32 PushDistance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1", ToolTip = "Reserved for a future PullTarget implementation. PullTarget currently logs a warning and does not resolve.", EditCondition = "Operation == ECardEffectOperation::PullTarget", EditConditionHides))
	int32 PullDistance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0", ToolTip = "Damage dealt when pushed movement collides with an obstruction.", EditCondition = "Operation == ECardEffectOperation::PushTarget", EditConditionHides))
	int32 CollisionDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Unit class used by SummonUnit.", EditCondition = "Operation == ECardEffectOperation::SummonUnit", EditConditionHides))
	TSubclassOf<ABattleUnit> UnitClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (ToolTip = "Whether a summoned unit enters with its attack already spent. This keeps summon tempo data-authorable.", EditCondition = "Operation == ECardEffectOperation::SummonUnit", EditConditionHides))
	bool bSummonEntersWithAttackExhausted = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (ToolTip = "Tile effect actor class used by PlaceTileEffect.", EditCondition = "Operation == ECardEffectOperation::PlaceTileEffect", EditConditionHides))
	TSubclassOf<ABattleTileEffect> TileEffectClass;
};

UCLASS(BlueprintType)
class JARGON_API UCardDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Cost = 1;

	/** Current cast/targeting range from the source unit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Range = 3;

	/** Primary card behavior. Add effect specs in the order they should resolve. Cards with no Effects will not resolve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Effects", meta = (ToolTip = "Primary card behavior. Add keyword-style effects in resolve order. Cards with no Effects log a warning and do not resolve."))
	TArray<FCardEffectSpec> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardCategory Category = ECardCategory::Spell;

	/** Card-level area radius for targeting/highlighting and effect specs that leave RadiusOverride at -1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Radius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardTargetType TargetType = ECardTargetType::Unit;

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
		return HasEffectOperation(ECardEffectOperation::SummonUnit) || Category == ECardCategory::Trap;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool UsesRadiusField() const
	{
		return Radius > 0 ||
			HasEffectOperation(ECardEffectOperation::DealDamage) ||
			HasEffectOperation(ECardEffectOperation::Heal) ||
			HasEffectOperation(ECardEffectOperation::ApplyShield) ||
			HasEffectOperation(ECardEffectOperation::PlaceTileEffect);
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	int32 GetConfiguredAreaRadius() const
	{
		return FMath::Max(0, Radius);
	}

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool UsesEffectSpecs() const
	{
		return Effects.Num() > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool HasEffectOperation(ECardEffectOperation Operation) const;

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
