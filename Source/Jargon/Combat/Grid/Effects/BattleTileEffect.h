#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "GameFramework/Actor.h"
#include "BattleTileEffect.generated.h"

class USceneComponent;
class AGridTile;
class UCardDefinition;
class AJargonCombatGameMode;
class ABattleUnit;
class UStaticMeshComponent;
class UJargonTileEffectDefinition;

UENUM(BlueprintType)
enum class EJargonTileEffectTargetFilter : uint8
{
	FriendlyToSource UMETA(DisplayName = "Friendly To Source"),
	EnemyToSource UMETA(DisplayName = "Enemy To Source"),
	Any UMETA(DisplayName = "Any")
};

UENUM(BlueprintType)
enum class EJargonTileEffectOperation : uint8
{
	None UMETA(DisplayName = "None"),
	DealDamage UMETA(DisplayName = "Deal Damage"),
	Heal UMETA(DisplayName = "Heal"),
	ApplyShield UMETA(DisplayName = "Apply Shield"),
	IncreaseAttack UMETA(DisplayName = "Increase Attack"),
	IncreaseMaxHealth UMETA(DisplayName = "Increase Max Health"),
	ApplyStun UMETA(DisplayName = "Apply Stun")
};

/*
 * Legacy tile-effect operation/filter enums.
 *
 * New trap/aura gameplay authoring lives in UJargonTileEffectDefinition.TriggerAbility.
 * These enums remain only for older Blueprint helper nodes until those visual
 * children are cleaned up.
 */
UCLASS(Blueprintable)
class JARGON_API ABattleTileEffect : public AActor
{
	GENERATED_BODY()

public:
	ABattleTileEffect();

	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable, Category = "Deprecated|Tile Effect", meta = (DeprecatedFunction, DeprecationMessage = "Use InitializeFromDefinition with a UJargonTileEffectDefinition."))
	void InitializeFromCard(
		UCardDefinition* InSourceCard,
		ETeam InSourceTeam,
		ECardCategory InCardCategory,
		int32 InEffectValue,
		int32 InEffectRadius,
		int32 InDuration = 0);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	void InitializeFromDefinition(
		UCardDefinition* InSourceCard,
		ETeam InSourceTeam,
		UJargonTileEffectDefinition* InDefinition);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	void PlaceOnTile(AGridTile* Tile);

	virtual void HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode);
	virtual void HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect|Effects")
	FJargonEffectContext BuildEffectContext(AJargonCombatGameMode* CombatGameMode, ABattleUnit* TriggeringUnit = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	void ShowAffectedTiles();

	UFUNCTION(BlueprintCallable, Category = "Tile Effect|Duration")
	void SetRemainingDuration(int32 NewDuration);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect|Duration")
	void ConsumeDurationTick();

	UFUNCTION(BlueprintPure, Category = "Tile Effect|Duration")
	bool HasFiniteDuration() const
	{
		return RemainingDuration > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect|Duration")
	int32 GetRemainingDuration() const
	{
		return RemainingDuration;
	}

	UFUNCTION(BlueprintCallable, Category = "Deprecated|Tile Effect|Spawning", meta = (DeprecatedFunction, DeprecationMessage = "Place tile effects through UJargonTileEffectDefinition and AJargonCombatGameMode::SpawnPersistentTileEffectFromDefinition."))
	ABattleTileEffect* SpawnTileEffectOnTile(
		TSubclassOf<ABattleTileEffect> TileEffectClass,
		AGridTile* TargetTile,
		int32 InEffectValue,
		int32 InEffectRadius,
		int32 InDuration);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect|Spawning")
	ABattleTileEffect* SpawnCopyOnTile(AGridTile* TargetTile);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect|Area")
	TArray<AGridTile*> GetCandidateTilesInRadius(AJargonCombatGameMode* CombatGameMode, int32 Radius) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Effect|Area")
	TArray<AGridTile*> GetEmptyWalkableTilesInRadius(AJargonCombatGameMode* CombatGameMode, int32 Radius) const;

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	AGridTile* GetCurrentTile() const
	{
		return CurrentTile;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	UCardDefinition* GetSourceCard() const
	{
		return SourceCard;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	ETeam GetSourceTeam() const
	{
		return SourceTeam;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	ECardCategory GetCardCategory() const
	{
		return CardCategory;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	int32 GetEffectRadius() const
	{
		return EffectRadius;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	int32 GetEffectValue() const
	{
		return EffectValue;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	UJargonTileEffectDefinition* GetTileEffectDefinition() const
	{
		return TileEffectDefinition;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Tile Effect")
	void BP_OnPlayerTurnStart(AJargonCombatGameMode* CombatGameMode);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tile Effect")
	void BP_OnInitializedFromCard();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tile Effect")
	void BP_OnPlacedOnTile(AGridTile* Tile);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	TArray<AGridTile*> GetTilesInEffectRadius(const AJargonCombatGameMode* CombatGameMode) const;
	
	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	TArray<ABattleUnit*> GetLivingUnitsInEffectRadius(const AJargonCombatGameMode* CombatGameMode) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	bool DoesUnitPassTargetFilter(const ABattleUnit* Unit, EJargonTileEffectTargetFilter TargetFilter) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	int32 ResolveEffectAmount(int32 DefaultEffectValue) const;

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	bool ApplyConfiguredOperationToUnit(ABattleUnit* TargetUnit, EJargonTileEffectOperation Operation, int32 Amount) const;

	void EmitTileEffectCue(EJargonCombatCueType CueType, ABattleUnit* TargetUnit = nullptr) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EffectMesh = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect")
	TObjectPtr<AGridTile> CurrentTile = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect")
	TObjectPtr<UCardDefinition> SourceCard = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect")
	TObjectPtr<UJargonTileEffectDefinition> TileEffectDefinition = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect")
	ETeam SourceTeam = ETeam::Player;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect")
	ECardCategory CardCategory = ECardCategory::Spell;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect|Area")
	int32 EffectRadius = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect")
	int32 EffectValue = 0;

	/**
	 * Runtime duration in player-turn ticks.
	 * 0 means infinite.
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tile Effect|Duration")
	int32 RemainingDuration = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Tile Effect", meta = (ClampMin = "0.0"))
	float TileEffectZOffset = 15.f;
};
