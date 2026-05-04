#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonHeroTypes.h"
#include "JargonCombatCueTypes.generated.h"

class ABattleTileEffect;
class ABattleUnit;
class AGridTile;
class UCardDefinition;
class UJargonArtifactDefinition;

/** Presentation-only cue taxonomy emitted by gameplay systems after gameplay has already resolved. */
UENUM(BlueprintType)
enum class EJargonCombatCueType : uint8
{
	None UMETA(DisplayName = "None"),
	CardPlayed UMETA(DisplayName = "Card Played"),
	Damage UMETA(DisplayName = "Damage"),
	Heal UMETA(DisplayName = "Heal"),
	ShieldGained UMETA(DisplayName = "Shield Gained"),
	ShieldBroken UMETA(DisplayName = "Shield Broken"),
	StunApplied UMETA(DisplayName = "Stun Applied"),
	StunConsumed UMETA(DisplayName = "Stun Consumed"),
	UnitSummoned UMETA(DisplayName = "Unit Summoned"),
	UnitDied UMETA(DisplayName = "Unit Died"),
	ChainJump UMETA(DisplayName = "Chain Jump"),
	AoEPulse UMETA(DisplayName = "AoE Pulse"),
	Push UMETA(DisplayName = "Push"),
	PushCollision UMETA(DisplayName = "Push Collision"),
	Pull UMETA(DisplayName = "Pull"),
	TileEffectPlaced UMETA(DisplayName = "Tile Effect Placed"),
	TileEffectTriggered UMETA(DisplayName = "Tile Effect Triggered"),
	TileEffectExpired UMETA(DisplayName = "Tile Effect Expired"),
	ArtifactTriggered UMETA(DisplayName = "Artifact Triggered"),
	TurnStart UMETA(DisplayName = "Turn Start"),
	EnemyTurnStart UMETA(DisplayName = "Enemy Turn Start"),
	ClassPassiveTriggered UMETA(DisplayName = "Class Passive Triggered"),
	HeroAspectTriggered UMETA(DisplayName = "Hero Aspect Triggered"),
	FreezeApplied UMETA(DisplayName = "Freeze Applied"),
	FreezeConsumed UMETA(DisplayName = "Freeze Consumed"),
	BurnApplied UMETA(DisplayName = "Burn Applied"),
	BurnTick UMETA(DisplayName = "Burn Tick"),
	RootApplied UMETA(DisplayName = "Root Applied"),
	RootConsumed UMETA(DisplayName = "Root Consumed"),
	VulnerableApplied UMETA(DisplayName = "Vulnerable Applied"),
	VulnerableConsumed UMETA(DisplayName = "Vulnerable Consumed"),
	ElementalBonusTriggered UMETA(DisplayName = "Elemental Bonus Triggered"),
	HeroAspectActivated UMETA(DisplayName = "Hero Aspect Activated"),
	RegenApplied UMETA(DisplayName = "Regen Applied"),
	RegenTick UMETA(DisplayName = "Regen Tick"),
	WeakApplied UMETA(DisplayName = "Weak Applied"),
	WeakConsumed UMETA(DisplayName = "Weak Consumed"),
	StatusCleansed UMETA(DisplayName = "Status Cleansed"),
	Lifesteal UMETA(DisplayName = "Lifesteal")
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonCombatCueEvent
{
	GENERATED_BODY()

	/** Presentation event type. This should never be used as a gameplay source of truth. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	EJargonCombatCueType CueType = EJargonCombatCueType::None;

	/** Gameplay operation that caused the cue, when it came from the shared effect runtime. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	EJargonEffectOperation Operation = EJargonEffectOperation::None;

	/** Hook that produced the cue, such as card play, turn start, trap entry, or death. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	EJargonEffectTrigger Trigger = EJargonEffectTrigger::OnPlayed;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<ABattleUnit> SourceUnit = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<ABattleUnit> TargetUnit = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<AGridTile> SourceTile = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<AGridTile> TargetTile = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<ABattleTileEffect> OwningTileEffect = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<UCardDefinition> SourceCard = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	TObjectPtr<UJargonArtifactDefinition> SourceArtifact = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	EJargonHeroClass HeroClass = EJargonHeroClass::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	EJargonHeroAspect HeroAspect = EJargonHeroAspect::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue|Element")
	EJargonElementType ElementType = EJargonElementType::None;

	/** Current or spent combat-local element charge amount relevant to the cue. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue|Element")
	int32 ElementChargeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue|Element")
	bool bSpentElementCharges = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue|Element")
	int32 ElementalBonusIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	int32 Value = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	int32 Radius = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	FText TextOverride;

	/** Optional explicit world-space cue location when source/target actors are insufficient. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	bool bHasWorldLocation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	bool bHasColorOverride = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Cue")
	FLinearColor ColorOverride = FLinearColor::White;
};
