#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "JargonEffectTypes.generated.h"

class ABattleTileEffect;
class ABattleUnit;
class AGridTile;
class AJargonCombatGameMode;
class UCardDefinition;

UENUM(BlueprintType)
enum class EJargonEffectOperation : uint8
{
	None UMETA(DisplayName = "None"),
	DealDamage UMETA(DisplayName = "Deal Damage"),
	Heal UMETA(DisplayName = "Heal"),
	ApplyShield UMETA(DisplayName = "Apply Shield"),
	ApplyStun UMETA(DisplayName = "Apply Stun"),
	MoveSource UMETA(DisplayName = "Move Source"),
	PushTarget UMETA(DisplayName = "Push Target"),
	PullTarget UMETA(DisplayName = "Pull Target"),
	SummonUnit UMETA(DisplayName = "Summon Unit"),
	PlaceTileEffect UMETA(DisplayName = "Place Tile Effect"),
	DestroyTileEffect UMETA(DisplayName = "Destroy Tile Effect"),
	DrawCards UMETA(DisplayName = "Draw Cards"),
	GainEnergy UMETA(DisplayName = "Gain Energy")
};

UENUM(BlueprintType)
enum class EJargonEffectDelivery : uint8
{
	Self UMETA(DisplayName = "Self"),
	ExplicitUnit UMETA(DisplayName = "Explicit Unit"),
	ExplicitTile UMETA(DisplayName = "Explicit Tile"),
	UnitsInRadius UMETA(DisplayName = "Units In Radius"),
	TilesInRadius UMETA(DisplayName = "Tiles In Radius"),
	ChainUnits UMETA(DisplayName = "Chain Units")
};

UENUM(BlueprintType)
enum class EJargonEffectTargetFilter : uint8
{
	None UMETA(DisplayName = "None"),
	FriendlyToSource UMETA(DisplayName = "Friendly To Source"),
	EnemyToSource UMETA(DisplayName = "Enemy To Source"),
	Any UMETA(DisplayName = "Any"),
	SourceOnly UMETA(DisplayName = "Source Only")
};

UENUM(BlueprintType)
enum class EJargonEffectTrigger : uint8
{
	OnPlayed UMETA(DisplayName = "On Played"),
	OnSummoned UMETA(DisplayName = "On Summoned"),
	OnEnterTile UMETA(DisplayName = "On Enter Tile"),
	OnTurnStart UMETA(DisplayName = "On Turn Start"),
	Activated UMETA(DisplayName = "Activated"),
	OnDeath UMETA(DisplayName = "On Death")
};

/**
 * Reusable authored gameplay effect spec.
 *
 * Payload lives in Operation; target acquisition lives in Delivery and TargetFilter.
 * Existing card assets are adapted into this type at runtime and are not migrated yet.
 */
USTRUCT(BlueprintType)
struct JARGON_API FJargonEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ToolTip = "What this effect does once targets are found, such as damage, heal, shield, summon, or place a tile effect."))
	EJargonEffectOperation Operation = EJargonEffectOperation::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ToolTip = "How this effect finds targets, such as self, explicit target, units in radius, tiles in radius, or chained units."))
	EJargonEffectDelivery Delivery = EJargonEffectDelivery::ExplicitUnit;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (ToolTip = "Which units are valid after delivery gathers candidates. Use EnemyToSource for hostile effects, FriendlyToSource for buffs/heals, SourceOnly for self effects, or Any when team does not matter."))
	EJargonEffectTargetFilter TargetFilter = EJargonEffectTargetFilter::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (
		ClampMin = "0",
		ToolTip = "Primary amount for this effect. Damage, healing, shield, stun turns, cards drawn, energy gained, or tile-effect payload value.",
		EditCondition = "Operation == EJargonEffectOperation::DealDamage || Operation == EJargonEffectOperation::Heal || Operation == EJargonEffectOperation::ApplyShield || Operation == EJargonEffectOperation::ApplyStun || Operation == EJargonEffectOperation::PlaceTileEffect || Operation == EJargonEffectOperation::DrawCards || Operation == EJargonEffectOperation::GainEnergy",
		EditConditionHides))
	int32 Value = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (
		ClampMin = "0",
		ToolTip = "Area radius for UnitsInRadius/TilesInRadius, or jump search radius for ChainUnits.",
		EditCondition = "Delivery == EJargonEffectDelivery::UnitsInRadius || Delivery == EJargonEffectDelivery::TilesInRadius || Delivery == EJargonEffectDelivery::ChainUnits || Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	int32 Radius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chain", meta = (
		ClampMin = "1",
		ToolTip = "Maximum number of units affected by ChainUnits, including the first target.",
		EditCondition = "Delivery == EJargonEffectDelivery::ChainUnits",
		EditConditionHides))
	int32 ChainCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (
		ClampMin = "0",
		ToolTip = "Maximum tiles the source can move for MoveSource.",
		EditCondition = "Operation == EJargonEffectOperation::MoveSource",
		EditConditionHides))
	int32 MoveDistance = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (
		ClampMin = "1",
		ToolTip = "Number of tiles to push the target away from the source.",
		EditCondition = "Operation == EJargonEffectOperation::PushTarget",
		EditConditionHides))
	int32 PushDistance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (
		ClampMin = "1",
		ToolTip = "Reserved for future PullTarget behavior. PullTarget currently warns/fails gracefully.",
		EditCondition = "Operation == EJargonEffectOperation::PullTarget",
		EditConditionHides))
	int32 PullDistance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (
		ClampMin = "0",
		ToolTip = "Damage dealt when a pushed target collides with blocked or occupied movement.",
		EditCondition = "Operation == EJargonEffectOperation::PushTarget",
		EditConditionHides))
	int32 CollisionDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (
		ToolTip = "Unit class spawned by SummonUnit.",
		EditCondition = "Operation == EJargonEffectOperation::SummonUnit",
		EditConditionHides))
	TSubclassOf<ABattleUnit> UnitClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (
		ToolTip = "Whether the summoned unit enters with its attack already spent.",
		EditCondition = "Operation == EJargonEffectOperation::SummonUnit",
		EditConditionHides))
	bool bSummonEntersWithAttackExhausted = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
		ToolTip = "Tile effect actor class spawned by PlaceTileEffect.",
		EditCondition = "Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	TSubclassOf<ABattleTileEffect> TileEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
		ClampMin = "0",
		ToolTip = "How many player turn starts this placed tile effect lasts. 0 means infinite.",
		EditCondition = "Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	int32 TileEffectDuration = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
		ToolTip = "Used by PlaceTileEffect when the effect is not sourced from a card. Card-sourced tile effects still use the card category.",
		EditCondition = "Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	ECardCategory TileEffectCategory = ECardCategory::Trap;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonEffectContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AJargonCombatGameMode> GameMode = nullptr;

	UPROPERTY()
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleUnit> SourceUnit = nullptr;

	UPROPERTY()
	ETeam SourceTeam = ETeam::Player;

	UPROPERTY()
	TObjectPtr<AGridTile> SourceTile = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleUnit> PrimaryUnitTarget = nullptr;

	UPROPERTY()
	TObjectPtr<AGridTile> PrimaryTileTarget = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleUnit> TriggeringUnit = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleTileEffect> OwningTileEffect = nullptr;

	UPROPERTY()
	TObjectPtr<UCardDefinition> SourceCard = nullptr;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonEffectResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bResolvedAnyEffect = false;

	UPROPERTY()
	bool bContinuesAsynchronously = false;

	UPROPERTY()
	bool bConsumePlayerMove = false;

	UPROPERTY()
	int32 EnergyGainAfterCost = 0;

	UPROPERTY()
	TObjectPtr<ABattleUnit> SpawnedUnit = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleTileEffect> SpawnedTileEffect = nullptr;
};
