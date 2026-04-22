// JargonCombatGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "GameFramework/GameModeBase.h"
#include "JargonCombatGameMode.generated.h"

class AGridBoard;
class AGridTile;
class ABattleUnit;
class APlayerBattleUnit;
class AEnemyDummyUnit;
class ATacticsCameraPawn;
class UCardDefinition;
class UCombatHUDWidget;
class AJargonCombatPlayerController;

UCLASS()
class JARGON_API AJargonCombatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJargonCombatGameMode();

	virtual void BeginPlay() override;

	void InitializeCombat();

	bool TryMovePlayerUnitToTile(AGridTile* DestinationTile);
	bool TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target);

	void HandleUnitDied(ABattleUnit* DeadUnit);
	void HandleVictory();
	void HandleDefeat();
	void ReturnToExploration();

	void RequestEndPlayerTurn();

	APlayerBattleUnit* GetPlayerUnit() const
	{
		return PlayerUnit;
	}

	const TArray<TObjectPtr<ABattleUnit>>& GetEnemyUnits() const
	{
		return EnemyUnits;
	}

	AGridBoard* GetGridBoard() const
	{
		return GridBoard;
	}

	TSubclassOf<UCombatHUDWidget> GetCombatHUDClass() const
	{
		return CombatHUDClass;
	}

	const TArray<TObjectPtr<UCardDefinition>>& GetStartingDeckDefinitions() const
	{
		return StartingDeckDefinitions;
	}

	int32 GetStartingHandSize() const
	{
		return StartingHandSize;
	}

	ECombatPhase GetCombatPhase() const
	{
		return CombatPhase;
	}

	int32 GetCurrentRound() const
	{
		return CurrentRound;
	}

	int32 GetCurrentEnergy() const
	{
		return CurrentEnergy;
	}

	bool HasPlayerMoveRemaining() const
	{
		return !bPlayerMoveUsed;
	}

protected:
	void InitializeCameraPawn();
	void FindGridBoard();
	void SpawnCombatants();
	void SpawnEnemiesFromPendingEncounter();
	void SpawnLegacyFallbackEnemy();
	bool AreAllEnemiesDefeated() const;

	void StartBattleFlow();
	void StartPlayerTurn();
	void EndPlayerTurn();
	void StartEnemyTurn();
	void ResolveEnemyTurn();
	void ResolveSingleEnemyAction(ABattleUnit* EnemyUnit);
	void EndEnemyTurn();
	void RefreshPlayerMovementHighlights();
	AJargonCombatPlayerController* GetCombatPlayerController() const;

	int32 GetTileDistance(const AGridTile* TileA, const AGridTile* TileB) const;
	AGridTile* FindBestEnemyMoveDestination(ABattleUnit* EnemyUnit, ABattleUnit* TargetUnit) const;

protected:
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<AGridBoard> GridBoard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<APlayerBattleUnit> PlayerUnit = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TArray<TObjectPtr<ABattleUnit>> EnemyUnits;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ATacticsCameraPawn> SpawnedCameraPawn = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	ECombatPhase CombatPhase = ECombatPhase::BattleStart;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<ATacticsCameraPawn> CameraPawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<APlayerBattleUnit> PlayerUnitClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Legacy")
	TSubclassOf<AEnemyDummyUnit> EnemyUnitClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|UI")
	TSubclassOf<UCombatHUDWidget> CombatHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<UCardDefinition>> StartingDeckDefinitions;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Cards", meta = (ClampMin = "1"))
	int32 StartingHandSize = 3;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentRound = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentEnergy = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	bool bPlayerMoveUsed = false;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0"))
	int32 EnergyPerTurn = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0"))
	int32 CardsDrawnPerTurn = 1;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	bool bFirstPlayerTurnStarted = false;
};