#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "JargonEffectTypes.generated.h"

class ABattleTileEffect;
class ABattleUnit;
class AGridTile;
class AJargonCombatGameMode;
class UCardDefinition;
class UJargonStatusEffectDefinition;
class UJargonSummonedUnitDefinition;
class UJargonTileEffectDefinition;

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
	GainEnergy UMETA(DisplayName = "Gain Energy"),
	GainElementCharge UMETA(DisplayName = "Gain Element Charge"),
	ApplyFreeze UMETA(DisplayName = "Apply Freeze"),
	ApplyBurn UMETA(DisplayName = "Apply Burn"),
	ApplyRoot UMETA(DisplayName = "Apply Root"),
	ApplyVulnerable UMETA(DisplayName = "Apply Vulnerable"),
	ApplyStatus UMETA(DisplayName = "Apply Status")
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
	OnDeath UMETA(DisplayName = "On Death"),
	OnCombatStart UMETA(DisplayName = "On Combat Start"),
	OnEnemyDeath UMETA(DisplayName = "On Enemy Death")
};

/**
 * Reusable authored gameplay effect spec.
 *
 * Operation is the backend primitive. Target acquisition lives in Delivery and TargetFilter.
 * Payload fields are operation-specific parameters, not hidden conditions or modifiers.
 * CardScript actions, tile effects, summons, boons, and hero passives build this shared type directly.
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
		ToolTip = "Primary amount for this effect. Damage, healing, shield, stun/freeze/root turns, burn stacks, vulnerable bonus damage, cards drawn, energy gained, element charges, or tile-effect payload value.",
		EditCondition = "Operation == EJargonEffectOperation::DealDamage || Operation == EJargonEffectOperation::Heal || Operation == EJargonEffectOperation::ApplyShield || Operation == EJargonEffectOperation::ApplyStun || Operation == EJargonEffectOperation::ApplyFreeze || Operation == EJargonEffectOperation::ApplyBurn || Operation == EJargonEffectOperation::ApplyRoot || Operation == EJargonEffectOperation::ApplyVulnerable || Operation == EJargonEffectOperation::ApplyStatus || Operation == EJargonEffectOperation::PlaceTileEffect || Operation == EJargonEffectOperation::DrawCards || Operation == EJargonEffectOperation::GainEnergy || Operation == EJargonEffectOperation::GainElementCharge",
		EditConditionHides))
	int32 Value = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (
		ToolTip = "Required data-driven status definition used by ApplyStatus. Existing direct ApplyStun/ApplyBurn/etc. operations are temporary migration scaffolding.",
		EditCondition = "Operation == EJargonEffectOperation::ApplyStatus",
		EditConditionHides))
	TObjectPtr<UJargonStatusEffectDefinition> StatusEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element", meta = (
		ToolTip = "Element charge type affected by GainElementCharge.",
		EditCondition = "Operation == EJargonEffectOperation::GainElementCharge",
		EditConditionHides))
	EJargonElementType ElementType = EJargonElementType::None;

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
		ToolTip = "Number of tiles to pull the target toward the source, stopping before blocked, occupied, or source tiles.",
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
		ToolTip = "Required data-driven summon definition used by SummonUnit.",
		EditCondition = "Operation == EJargonEffectOperation::SummonUnit",
		EditConditionHides))
	TObjectPtr<UJargonSummonedUnitDefinition> SummonedUnitDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (
		ToolTip = "Required runtime Blueprint child used by SummonUnit. The effect/card chooses the runtime actor shell; the definition supplies gameplay data.",
		EditCondition = "Operation == EJargonEffectOperation::SummonUnit",
		EditConditionHides))
	TSubclassOf<ABattleUnit> RuntimeSummonedUnitClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Summon", meta = (
		ToolTip = "Whether the summoned unit enters with its attack already spent.",
		EditCondition = "Operation == EJargonEffectOperation::SummonUnit",
		EditConditionHides))
	bool bSummonEntersWithAttackExhausted = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
		ToolTip = "Required data-driven trap/aura definition spawned by PlaceTileEffect.",
		EditCondition = "Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	TObjectPtr<UJargonTileEffectDefinition> TileEffectDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
		ToolTip = "Required runtime Blueprint child used by PlaceTileEffect. The effect/card chooses the runtime actor shell; the definition supplies gameplay data.",
		EditCondition = "Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (
		ToolTip = "Used by PlaceTileEffect when the effect is not sourced from a card. Card-sourced tile effects still use the card category.",
		EditCondition = "Operation == EJargonEffectOperation::PlaceTileEffect",
		EditConditionHides))
	ECardCategory TileEffectCategory = ECardCategory::Trap;
};

namespace JargonEffectContracts
{
	inline FString GetEnumTokenName(const UEnum* Enum, int64 Value)
	{
		if (Enum)
		{
			const FString Name = Enum->GetNameStringByValue(Value);
			if (!Name.IsEmpty())
			{
				return Name;
			}
		}

		return FString::Printf(TEXT("%lld"), Value);
	}

	inline FString GetOperationName(EJargonEffectOperation Operation)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectOperation>(), static_cast<int64>(Operation));
	}

	inline FString GetDeliveryName(EJargonEffectDelivery Delivery)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectDelivery>(), static_cast<int64>(Delivery));
	}

	inline FString GetTargetFilterName(EJargonEffectTargetFilter TargetFilter)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectTargetFilter>(), static_cast<int64>(TargetFilter));
	}

	inline FString GetElementName(EJargonElementType ElementType)
	{
		return GetEnumTokenName(StaticEnum<EJargonElementType>(), static_cast<int64>(ElementType));
	}

	inline bool IsDirectStatusMigrationOperation(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::ApplyStun
			|| Operation == EJargonEffectOperation::ApplyFreeze
			|| Operation == EJargonEffectOperation::ApplyBurn
			|| Operation == EJargonEffectOperation::ApplyRoot
			|| Operation == EJargonEffectOperation::ApplyVulnerable;
	}

	inline bool RequiresValue(EJargonEffectOperation Operation)
	{
		switch (Operation)
		{
		case EJargonEffectOperation::DealDamage:
		case EJargonEffectOperation::Heal:
		case EJargonEffectOperation::ApplyShield:
		case EJargonEffectOperation::ApplyStun:
		case EJargonEffectOperation::ApplyFreeze:
		case EJargonEffectOperation::ApplyBurn:
		case EJargonEffectOperation::ApplyRoot:
		case EJargonEffectOperation::ApplyVulnerable:
		case EJargonEffectOperation::ApplyStatus:
		case EJargonEffectOperation::PlaceTileEffect:
		case EJargonEffectOperation::DrawCards:
		case EJargonEffectOperation::GainEnergy:
		case EJargonEffectOperation::GainElementCharge:
			return true;
		default:
			return false;
		}
	}

	inline bool SupportsRadius(EJargonEffectOperation Operation, EJargonEffectDelivery Delivery)
	{
		return Delivery == EJargonEffectDelivery::UnitsInRadius
			|| Delivery == EJargonEffectDelivery::TilesInRadius
			|| Delivery == EJargonEffectDelivery::ChainUnits
			|| Operation == EJargonEffectOperation::PlaceTileEffect;
	}

	inline bool RequiresUnitTargets(EJargonEffectOperation Operation)
	{
		switch (Operation)
		{
		case EJargonEffectOperation::DealDamage:
		case EJargonEffectOperation::Heal:
		case EJargonEffectOperation::ApplyShield:
		case EJargonEffectOperation::ApplyStun:
		case EJargonEffectOperation::ApplyFreeze:
		case EJargonEffectOperation::ApplyBurn:
		case EJargonEffectOperation::ApplyRoot:
		case EJargonEffectOperation::ApplyVulnerable:
		case EJargonEffectOperation::ApplyStatus:
		case EJargonEffectOperation::PushTarget:
		case EJargonEffectOperation::PullTarget:
			return true;
		default:
			return false;
		}
	}

	inline bool RequiresTileTarget(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::MoveSource
			|| Operation == EJargonEffectOperation::SummonUnit
			|| Operation == EJargonEffectOperation::PlaceTileEffect
			|| Operation == EJargonEffectOperation::DestroyTileEffect;
	}

	inline bool IsUnitDelivery(EJargonEffectDelivery Delivery)
	{
		return Delivery == EJargonEffectDelivery::ExplicitUnit
			|| Delivery == EJargonEffectDelivery::UnitsInRadius
			|| Delivery == EJargonEffectDelivery::ChainUnits;
	}

	inline bool IsTileDelivery(EJargonEffectDelivery Delivery)
	{
		return Delivery == EJargonEffectDelivery::ExplicitTile
			|| Delivery == EJargonEffectDelivery::TilesInRadius;
	}

	inline bool RequiresStatusDefinition(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::ApplyStatus;
	}

	inline bool RequiresElementType(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::GainElementCharge;
	}

	inline bool RequiresSummonPayload(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::SummonUnit;
	}

	inline bool RequiresTileEffectPayload(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::PlaceTileEffect;
	}

	inline FString BuildPayloadSummary(const FJargonEffectSpec& EffectSpec)
	{
		TArray<FString> Fields;

		if (RequiresValue(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("Value=%d"), EffectSpec.Value));
		}
		if (RequiresStatusDefinition(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("StatusDefinition=%s"), EffectSpec.StatusEffectDefinition ? TEXT("Assigned") : TEXT("None")));
		}
		if (RequiresElementType(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("Element=%s"), *GetElementName(EffectSpec.ElementType)));
		}
		if (SupportsRadius(EffectSpec.Operation, EffectSpec.Delivery))
		{
			Fields.Add(FString::Printf(TEXT("Radius=%d"), EffectSpec.Radius));
		}
		if (EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits)
		{
			Fields.Add(FString::Printf(TEXT("ChainCount=%d"), EffectSpec.ChainCount));
		}

		switch (EffectSpec.Operation)
		{
		case EJargonEffectOperation::MoveSource:
			Fields.Add(FString::Printf(TEXT("MoveDistance=%d"), EffectSpec.MoveDistance));
			break;
		case EJargonEffectOperation::PushTarget:
			Fields.Add(FString::Printf(TEXT("PushDistance=%d"), EffectSpec.PushDistance));
			Fields.Add(FString::Printf(TEXT("CollisionDamage=%d"), EffectSpec.CollisionDamage));
			break;
		case EJargonEffectOperation::PullTarget:
			Fields.Add(FString::Printf(TEXT("PullDistance=%d"), EffectSpec.PullDistance));
			break;
		case EJargonEffectOperation::SummonUnit:
			Fields.Add(FString::Printf(TEXT("SummonDefinition=%s"), EffectSpec.SummonedUnitDefinition ? TEXT("Assigned") : TEXT("None")));
			if (UClass* RuntimeSummonClass = EffectSpec.RuntimeSummonedUnitClass.DebugAccessRawClassPtr())
			{
				Fields.Add(FString::Printf(TEXT("RuntimeSummonClass=%s"), *GetNameSafe(RuntimeSummonClass)));
			}
			else
			{
				Fields.Add(TEXT("RuntimeSummonClass=None"));
			}
			Fields.Add(FString::Printf(TEXT("AttackExhausted=%s"), EffectSpec.bSummonEntersWithAttackExhausted ? TEXT("true") : TEXT("false")));
			break;
		case EJargonEffectOperation::PlaceTileEffect:
			Fields.Add(FString::Printf(TEXT("TileEffectDefinition=%s"), EffectSpec.TileEffectDefinition ? TEXT("Assigned") : TEXT("None")));
			if (UClass* RuntimeTileEffectClass = EffectSpec.RuntimeTileEffectClass.DebugAccessRawClassPtr())
			{
				Fields.Add(FString::Printf(TEXT("RuntimeTileEffectClass=%s"), *GetNameSafe(RuntimeTileEffectClass)));
			}
			else
			{
				Fields.Add(TEXT("RuntimeTileEffectClass=None"));
			}
			break;
		default:
			break;
		}

		return Fields.Num() > 0 ? FString::Join(Fields, TEXT(" ")) : TEXT("None");
	}

	inline void AppendIgnoredPayloadWarnings(const FJargonEffectSpec& EffectSpec, TArray<FString>& OutWarnings)
	{
		const FString OperationName = GetOperationName(EffectSpec.Operation);

		if (!RequiresValue(EffectSpec.Operation) && EffectSpec.Value != 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores Value=%d."), *OperationName, EffectSpec.Value));
		}
		if (!RequiresElementType(EffectSpec.Operation) && EffectSpec.ElementType != EJargonElementType::None)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores ElementType=%s."), *OperationName, *GetElementName(EffectSpec.ElementType)));
		}
		if (!SupportsRadius(EffectSpec.Operation, EffectSpec.Delivery) && EffectSpec.Radius > 0)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s with Delivery=%s ignores Radius=%d."), *OperationName, *GetDeliveryName(EffectSpec.Delivery), EffectSpec.Radius));
		}
		if (EffectSpec.Delivery != EJargonEffectDelivery::ChainUnits && EffectSpec.ChainCount != 3)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s with Delivery=%s ignores ChainCount=%d."), *OperationName, *GetDeliveryName(EffectSpec.Delivery), EffectSpec.ChainCount));
		}
		if (EffectSpec.Operation != EJargonEffectOperation::MoveSource && EffectSpec.MoveDistance > 0)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores MoveDistance=%d."), *OperationName, EffectSpec.MoveDistance));
		}
		if (EffectSpec.Operation != EJargonEffectOperation::PushTarget && EffectSpec.PushDistance != 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores PushDistance=%d."), *OperationName, EffectSpec.PushDistance));
		}
		if (EffectSpec.Operation != EJargonEffectOperation::PushTarget && EffectSpec.CollisionDamage != 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores CollisionDamage=%d."), *OperationName, EffectSpec.CollisionDamage));
		}
		if (EffectSpec.Operation != EJargonEffectOperation::PullTarget && EffectSpec.PullDistance != 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores PullDistance=%d."), *OperationName, EffectSpec.PullDistance));
		}
		if (!RequiresStatusDefinition(EffectSpec.Operation) && EffectSpec.StatusEffectDefinition)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores StatusEffectDefinition."), *OperationName));
		}
		if (!RequiresSummonPayload(EffectSpec.Operation) &&
			(EffectSpec.SummonedUnitDefinition || EffectSpec.RuntimeSummonedUnitClass.DebugAccessRawClassPtr()))
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores summon payload fields."), *OperationName));
		}
		if (!RequiresTileEffectPayload(EffectSpec.Operation) &&
			(EffectSpec.TileEffectDefinition || EffectSpec.RuntimeTileEffectClass.DebugAccessRawClassPtr()))
		{
			OutWarnings.Add(FString::Printf(TEXT("%s ignores tile-effect payload fields."), *OperationName));
		}
	}

	inline void AppendDeliveryFilterWarnings(const FJargonEffectSpec& EffectSpec, TArray<FString>& OutWarnings)
	{
		const FString OperationName = GetOperationName(EffectSpec.Operation);
		const FString DeliveryName = GetDeliveryName(EffectSpec.Delivery);
		const FString FilterName = GetTargetFilterName(EffectSpec.TargetFilter);

		if (RequiresUnitTargets(EffectSpec.Operation) && IsTileDelivery(EffectSpec.Delivery))
		{
			OutWarnings.Add(FString::Printf(TEXT("%s usually needs unit delivery, but Delivery=%s targets tiles."), *OperationName, *DeliveryName));
		}
		if (RequiresTileTarget(EffectSpec.Operation) && IsUnitDelivery(EffectSpec.Delivery))
		{
			OutWarnings.Add(FString::Printf(TEXT("%s usually needs tile delivery, but Delivery=%s targets units."), *OperationName, *DeliveryName));
		}
		if (EffectSpec.Delivery == EJargonEffectDelivery::Self &&
			(EffectSpec.TargetFilter == EJargonEffectTargetFilter::EnemyToSource ||
				EffectSpec.TargetFilter == EJargonEffectTargetFilter::FriendlyToSource))
		{
			OutWarnings.Add(FString::Printf(TEXT("Delivery=Self with Filter=%s is suspicious; SourceOnly is usually clearer."), *FilterName));
		}
		if (RequiresUnitTargets(EffectSpec.Operation) &&
			EffectSpec.TargetFilter == EJargonEffectTargetFilter::None)
		{
			OutWarnings.Add(FString::Printf(TEXT("%s uses unit targets with Filter=None; verify Any, EnemyToSource, FriendlyToSource, or SourceOnly is intended."), *OperationName));
		}
	}
}

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

	UPROPERTY()
	EJargonEffectTrigger Trigger = EJargonEffectTrigger::OnPlayed;
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

UENUM(BlueprintType)
enum class EJargonEffectTraceEventType : uint8
{
	ResolveStarted UMETA(DisplayName = "Resolve Started"),
	ValidationFailed UMETA(DisplayName = "Validation Failed"),
	ValidationWarning UMETA(DisplayName = "Validation Warning"),
	TargetsGathered UMETA(DisplayName = "Targets Gathered"),
	TargetSkipped UMETA(DisplayName = "Target Skipped"),
	NoTargetsNoOp UMETA(DisplayName = "No Targets No-op"),
	OperationApplied UMETA(DisplayName = "Operation Applied"),
	OperationFailed UMETA(DisplayName = "Operation Failed"),
	FallbackUsed UMETA(DisplayName = "Fallback Used"),
	AsyncStarted UMETA(DisplayName = "Async Started"),
	EffectsSkipped UMETA(DisplayName = "Effects Skipped"),
	ResolveFinished UMETA(DisplayName = "Resolve Finished")
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonEffectTraceEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	int32 EffectIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	EJargonEffectTraceEventType EventType = EJargonEffectTraceEventType::ResolveStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	EJargonEffectOperation Operation = EJargonEffectOperation::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	EJargonEffectDelivery Delivery = EJargonEffectDelivery::ExplicitUnit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	EJargonEffectTargetFilter TargetFilter = EJargonEffectTargetFilter::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<UCardDefinition> SourceCard = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<ABattleUnit> SourceUnit = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<AGridTile> SourceTile = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<ABattleUnit> ExplicitUnitTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<AGridTile> ExplicitTileTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TArray<TObjectPtr<ABattleUnit>> ResolvedUnitTargets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TArray<TObjectPtr<AGridTile>> ResolvedTileTargets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<ABattleUnit> SkippedUnitTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<AGridTile> SkippedTileTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	FString Reason;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	FString Warning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	bool bOperationSucceeded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	bool bResolvedAnyEffect = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	bool bContinuesAsynchronously = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<ABattleUnit> SpawnedUnit = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<ABattleTileEffect> SpawnedTileEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	int32 Value = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	int32 EnergyGain = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	EJargonElementType ElementType = EJargonElementType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	int32 ElementChargeDelta = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	FString PayloadSummary;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonEffectTrace
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	FGuid TraceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	EJargonEffectTrigger Trigger = EJargonEffectTrigger::OnPlayed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TObjectPtr<UCardDefinition> SourceCard = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	TArray<FJargonEffectTraceEvent> Events;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	bool bResolved = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	bool bResolvedAnyEffect = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	bool bContinuedAsynchronously = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect Trace")
	FString Summary;

	void ResetForContext(const FJargonEffectContext& Context)
	{
		TraceId = FGuid::NewGuid();
		Trigger = Context.Trigger;
		SourceObject = Context.SourceObject;
		SourceCard = Context.SourceCard;
		Events.Reset();
		bResolved = false;
		bResolvedAnyEffect = false;
		bContinuedAsynchronously = false;
		Summary.Reset();
	}

	void AddEvent(const FJargonEffectTraceEvent& Event)
	{
		Events.Add(Event);
	}

	bool HasWarnings() const
	{
		for (const FJargonEffectTraceEvent& Event : Events)
		{
			if (!Event.Warning.IsEmpty() ||
				Event.EventType == EJargonEffectTraceEventType::ValidationFailed ||
				Event.EventType == EJargonEffectTraceEventType::ValidationWarning ||
				Event.EventType == EJargonEffectTraceEventType::OperationFailed ||
				Event.EventType == EJargonEffectTraceEventType::FallbackUsed ||
				Event.EventType == EJargonEffectTraceEventType::EffectsSkipped)
			{
				return true;
			}
		}

		return false;
	}

	static FString GetEnumTokenName(const UEnum* Enum, int64 Value)
	{
		if (Enum)
		{
			const FString Name = Enum->GetNameStringByValue(Value);
			if (!Name.IsEmpty())
			{
				return Name;
			}
		}

		return FString::Printf(TEXT("%lld"), Value);
	}

	static FString GetTraceEventName(EJargonEffectTraceEventType Value)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectTraceEventType>(), static_cast<int64>(Value));
	}

	static FString GetOperationName(EJargonEffectOperation Value)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectOperation>(), static_cast<int64>(Value));
	}

	static FString GetDeliveryName(EJargonEffectDelivery Value)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectDelivery>(), static_cast<int64>(Value));
	}

	static FString GetTargetFilterName(EJargonEffectTargetFilter Value)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectTargetFilter>(), static_cast<int64>(Value));
	}

	static FString GetTriggerName(EJargonEffectTrigger Value)
	{
		return GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(Value));
	}

	static FString GetElementName(EJargonElementType Value)
	{
		return GetEnumTokenName(StaticEnum<EJargonElementType>(), static_cast<int64>(Value));
	}

	FString ToCompactString() const
	{
		return FString::Printf(
			TEXT("Trace=%s Trigger=%s Source=%s Card=%s Events=%d Resolved=%s AnyEffect=%s Async=%s Warnings=%s"),
			*TraceId.ToString(EGuidFormats::DigitsWithHyphens),
			*GetTriggerName(Trigger),
			*GetNameSafe(SourceObject.Get()),
			SourceCard.Get() ? TEXT("Assigned") : TEXT("None"),
			Events.Num(),
			bResolved ? TEXT("true") : TEXT("false"),
			bResolvedAnyEffect ? TEXT("true") : TEXT("false"),
			bContinuedAsynchronously ? TEXT("true") : TEXT("false"),
			HasWarnings() ? TEXT("true") : TEXT("false"));
	}

	FString ToMultilineString() const
	{
		TArray<FString> Lines;
		Lines.Add(ToCompactString());

		for (const FJargonEffectTraceEvent& Event : Events)
		{
			Lines.Add(FString::Printf(
				TEXT("[%d] Event=%s Operation=%s Delivery=%s Filter=%s Payload=(%s) Source=%s Card=%s UnitTarget=%s TileTarget=%s Units=%d Tiles=%d Success=%s AnyEffect=%s Async=%s SpawnedUnit=%s SpawnedTileEffect=%s Value=%d EnergyGain=%d Element=%s ElementDelta=%d Reason=%s Warning=%s"),
				Event.EffectIndex,
				*GetTraceEventName(Event.EventType),
				*GetOperationName(Event.Operation),
				*GetDeliveryName(Event.Delivery),
				*GetTargetFilterName(Event.TargetFilter),
				*Event.PayloadSummary,
				*GetNameSafe(Event.SourceObject.Get()),
				Event.SourceCard.Get() ? TEXT("Assigned") : TEXT("None"),
				Event.ExplicitUnitTarget.Get() ? TEXT("Assigned") : TEXT("None"),
				Event.ExplicitTileTarget.Get() ? TEXT("Assigned") : TEXT("None"),
				Event.ResolvedUnitTargets.Num(),
				Event.ResolvedTileTargets.Num(),
				Event.bOperationSucceeded ? TEXT("true") : TEXT("false"),
				Event.bResolvedAnyEffect ? TEXT("true") : TEXT("false"),
				Event.bContinuesAsynchronously ? TEXT("true") : TEXT("false"),
				Event.SpawnedUnit.Get() ? TEXT("Assigned") : TEXT("None"),
				Event.SpawnedTileEffect.Get() ? TEXT("Assigned") : TEXT("None"),
				Event.Value,
				Event.EnergyGain,
				*GetElementName(Event.ElementType),
				Event.ElementChargeDelta,
				*Event.Reason,
				*Event.Warning));
		}

		return FString::Join(Lines, LINE_TERMINATOR);
	}
};
