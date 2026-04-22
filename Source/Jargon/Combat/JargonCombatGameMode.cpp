// JargonCombatGameMode.cpp

#include "Combat/JargonCombatGameMode.h"

#include "Data/CardDefinition.h"
#include "Units/BattleUnit.h"
#include "Units/EnemyDummyUnit.h"
#include "Combat/JargonCombatPlayerController.h"
#include "Units/PlayerBattleUnit.h"
#include "Combat/TacticsCameraPawn.h"
#include "Core/JargonGameInstance.h"
#include "Encounters/EncounterTypes.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"
#include "Kismet/GameplayStatics.h"

AJargonCombatGameMode::AJargonCombatGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AJargonCombatPlayerController::StaticClass();
	CombatPhase = ECombatPhase::BattleStart;
	CurrentRound = 0;
	CurrentEnergy = 0;
	bPlayerMoveUsed = false;
	EnergyPerTurn = 1;
	CardsDrawnPerTurn = 1;
	bFirstPlayerTurnStarted = false;
}

void AJargonCombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitializeCombat();
}

void AJargonCombatGameMode::InitializeCombat()
{
	CombatPhase = ECombatPhase::BattleStart;

	UE_LOG(LogTemp, Log, TEXT("Combat GameMode initialized for world '%s'."), *GetNameSafe(GetWorld()));

	InitializeCameraPawn();
	FindGridBoard();
	SpawnCombatants();
	StartBattleFlow();
}

void AJargonCombatGameMode::InitializeCameraPawn()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeCameraPawn could not find PlayerController 0."));
		return;
	}

	if (CameraPawnClass)
	{
		const FTransform CameraSpawnTransform(FRotator::ZeroRotator, FVector::ZeroVector);

		SpawnedCameraPawn = GetWorld()->SpawnActor<ATacticsCameraPawn>(
			CameraPawnClass,
			CameraSpawnTransform
		);

		if (!SpawnedCameraPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeCameraPawn failed to spawn CameraPawnClass."));
		}
	}
	else
	{
		for (TActorIterator<ATacticsCameraPawn> It(GetWorld()); It; ++It)
		{
			SpawnedCameraPawn = *It;
			break;
		}

		if (!SpawnedCameraPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeCameraPawn found no CameraPawnClass and no placed ATacticsCameraPawn."));
		}
	}

	if (SpawnedCameraPawn)
	{
		PlayerController->Possess(SpawnedCameraPawn);
	}
}

void AJargonCombatGameMode::FindGridBoard()
{
	GridBoard = nullptr;

	for (TActorIterator<AGridBoard> It(GetWorld()); It; ++It)
	{
		GridBoard = *It;
		break;
	}

	if (!GridBoard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat GameMode could not find an AGridBoard in the level."));
	}
}

void AJargonCombatGameMode::SpawnCombatants()
{
	EnemyUnits.Reset();
	PlayerUnit = nullptr;

	if (!GridBoard)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants aborted because GridBoard is null."));
		return;
	}

	if (!PlayerUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants aborted because PlayerUnitClass is not assigned."));
		return;
	}

	AGridTile* PlayerSpawnTile = GridBoard->GetPlayerSpawnTile();
	if (!PlayerSpawnTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants could not find a valid player spawn tile."));
		return;
	}

	if (!PlayerSpawnTile->IsWalkable())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants aborted because player spawn tile is not walkable."));
		return;
	}

	PlayerUnit = GetWorld()->SpawnActor<APlayerBattleUnit>(
		PlayerUnitClass,
		PlayerSpawnTile->GetUnitStandLocation(),
		FRotator::ZeroRotator
	);

	if (!PlayerUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants failed to spawn PlayerUnitClass."));
		return;
	}

	PlayerUnit->PlaceOnTile(PlayerSpawnTile);

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (GameInstance && GameInstance->HasPendingEncounterData())
	{
		SpawnEnemiesFromPendingEncounter();
	}
	else
	{
		SpawnLegacyFallbackEnemy();
	}

	if (EnemyUnits.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants completed but no enemy units were spawned."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("SpawnCombatants succeeded. Spawned %d enemy unit(s)."), EnemyUnits.Num());
	}
}

void AJargonCombatGameMode::SpawnEnemiesFromPendingEncounter()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter failed because GameInstance was null."));
		return;
	}

	const FPendingEncounterRuntimeData& PendingEncounter = GameInstance->GetPendingEncounterData();
	if (!PendingEncounter.HasConfiguredCombatEncounter())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter found no configured pending encounter."));
		return;
	}

	for (const FEncounterEnemySpawn& SpawnEntry : PendingEncounter.EnemySpawns)
	{
		if (!SpawnEntry.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter skipped an invalid spawn entry."));
			continue;
		}

		AGridTile* SpawnTile = GridBoard ? GridBoard->GetTile(SpawnEntry.SpawnCoord) : nullptr;
		if (!SpawnTile)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter could not find tile at coord (%d, %d)."),
				SpawnEntry.SpawnCoord.X,
				SpawnEntry.SpawnCoord.Y);
			continue;
		}

		if (!SpawnTile->IsWalkable())
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter skipped non-walkable tile at coord (%d, %d)."),
				SpawnEntry.SpawnCoord.X,
				SpawnEntry.SpawnCoord.Y);
			continue;
		}

		ABattleUnit* SpawnedEnemy = GetWorld()->SpawnActor<ABattleUnit>(
			SpawnEntry.UnitClass,
			SpawnTile->GetUnitStandLocation(),
			FRotator::ZeroRotator
		);

		if (!SpawnedEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter failed to spawn enemy unit at coord (%d, %d)."),
				SpawnEntry.SpawnCoord.X,
				SpawnEntry.SpawnCoord.Y);
			continue;
		}

		SpawnedEnemy->PlaceOnTile(SpawnTile);
		EnemyUnits.Add(SpawnedEnemy);
	}
}

void AJargonCombatGameMode::SpawnLegacyFallbackEnemy()
{
	if (!EnemyUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnLegacyFallbackEnemy aborted because EnemyUnitClass is not assigned."));
		return;
	}

	AGridTile* EnemySpawnTile = GridBoard ? GridBoard->GetEnemySpawnTile() : nullptr;
	if (!EnemySpawnTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnLegacyFallbackEnemy could not find a valid fallback enemy spawn tile."));
		return;
	}

	if (!EnemySpawnTile->IsWalkable())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnLegacyFallbackEnemy aborted because fallback enemy spawn tile is not walkable."));
		return;
	}

	AEnemyDummyUnit* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyDummyUnit>(
		EnemyUnitClass,
		EnemySpawnTile->GetUnitStandLocation(),
		FRotator::ZeroRotator
	);

	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnLegacyFallbackEnemy failed to spawn EnemyUnitClass."));
		return;
	}

	SpawnedEnemy->PlaceOnTile(EnemySpawnTile);
	EnemyUnits.Add(SpawnedEnemy);
}

bool AJargonCombatGameMode::AreAllEnemiesDefeated() const
{
	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			return false;
		}
	}

	return true;
}

void AJargonCombatGameMode::StartBattleFlow()
{
	if (!PlayerUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBattleFlow aborted because PlayerUnit is null."));
		return;
	}

	if (EnemyUnits.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBattleFlow aborted because no enemy units were spawned."));
		return;
	}

	CurrentRound = 1;
	bFirstPlayerTurnStarted = false;

	StartPlayerTurn();
}

void AJargonCombatGameMode::StartPlayerTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	CombatPhase = ECombatPhase::PlayerTurn;
	bPlayerMoveUsed = false;
	CurrentEnergy = EnergyPerTurn;

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (CombatPC)
	{
		CombatPC->ClearSelectedCard();

		if (bFirstPlayerTurnStarted)
		{
			CombatPC->DrawCards(CardsDrawnPerTurn);
		}
	}

	bFirstPlayerTurnStarted = true;
	RefreshPlayerMovementHighlights();

	UE_LOG(LogTemp, Log, TEXT("Player Turn Start - Round %d, Energy %d"), CurrentRound, CurrentEnergy);
}

void AJargonCombatGameMode::EndPlayerTurn()
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return;
	}

	CombatPhase = ECombatPhase::Resolving;

	if (GridBoard)
	{
		GridBoard->ClearHighlights();

		if (PlayerUnit && PlayerUnit->GetCurrentTile())
		{
			PlayerUnit->GetCurrentTile()->SetHighlightState(ETileHighlightState::Selected);
		}
	}

	StartEnemyTurn();
}

void AJargonCombatGameMode::StartEnemyTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	CombatPhase = ECombatPhase::EnemyTurn;

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (CombatPC)
	{
		CombatPC->ClearSelectedCard();
	}

	UE_LOG(LogTemp, Log, TEXT("Enemy Turn Start - Round %d"), CurrentRound);

	ResolveEnemyTurn();
}

void AJargonCombatGameMode::ResolveEnemyTurn()
{
	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
		{
			continue;
		}

		if (!PlayerUnit || PlayerUnit->IsDead())
		{
			HandleDefeat();
			return;
		}

		ResolveSingleEnemyAction(EnemyUnit);

		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}
	}

	EndEnemyTurn();
}

void AJargonCombatGameMode::ResolveSingleEnemyAction(ABattleUnit* EnemyUnit)
{
	if (!EnemyUnit || !PlayerUnit || EnemyUnit->IsDead() || PlayerUnit->IsDead())
	{
		return;
	}

	if (EnemyUnit->CanAttackTarget(PlayerUnit))
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' attacks player for %d damage."),
			*GetNameSafe(EnemyUnit), EnemyUnit->GetAttackDamage());

		EnemyUnit->PerformBasicAttack(PlayerUnit);
		return;
	}

	AGridTile* BestDestination = FindBestEnemyMoveDestination(EnemyUnit, PlayerUnit);
	if (BestDestination && EnemyUnit->GetCurrentTile())
	{
		const TArray<AGridTile*> Path = GridBoard->BuildPath(EnemyUnit->GetCurrentTile(), BestDestination);
		if (Path.Num() >= 2)
		{
			EnemyUnit->MoveAlongPath(Path);

			UE_LOG(LogTemp, Log, TEXT("Enemy '%s' moves to (%d, %d)."),
				*GetNameSafe(EnemyUnit),
				BestDestination->GetCoord().X,
				BestDestination->GetCoord().Y);
		}
	}

	if (EnemyUnit->CanAttackTarget(PlayerUnit))
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' attacks player after moving for %d damage."),
			*GetNameSafe(EnemyUnit), EnemyUnit->GetAttackDamage());

		EnemyUnit->PerformBasicAttack(PlayerUnit);
	}
}

void AJargonCombatGameMode::EndEnemyTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	CurrentRound++;
	StartPlayerTurn();
}

void AJargonCombatGameMode::RefreshPlayerMovementHighlights()
{
	if (!GridBoard || !PlayerUnit || !PlayerUnit->GetCurrentTile())
	{
		return;
	}

	if (CombatPhase == ECombatPhase::PlayerTurn && !bPlayerMoveUsed)
	{
		GridBoard->HighlightReachableTilesFrom(PlayerUnit->GetCurrentTile(), PlayerUnit->GetMoveRange());
	}
	else
	{
		GridBoard->ClearHighlights();
		PlayerUnit->GetCurrentTile()->SetHighlightState(ETileHighlightState::Selected);
	}
}

AJargonCombatPlayerController* AJargonCombatGameMode::GetCombatPlayerController() const
{
	return GetWorld() ? GetWorld()->GetFirstPlayerController<AJargonCombatPlayerController>() : nullptr;
}

int32 AJargonCombatGameMode::GetTileDistance(const AGridTile* TileA, const AGridTile* TileB) const
{
	if (!TileA || !TileB)
	{
		return MAX_int32;
	}

	const FIntPoint CoordA = TileA->GetCoord();
	const FIntPoint CoordB = TileB->GetCoord();

	return FMath::Abs(CoordA.X - CoordB.X) + FMath::Abs(CoordA.Y - CoordB.Y);
}

AGridTile* AJargonCombatGameMode::FindBestEnemyMoveDestination(ABattleUnit* EnemyUnit, ABattleUnit* TargetUnit) const
{
	if (!GridBoard || !EnemyUnit || !TargetUnit)
	{
		return nullptr;
	}

	AGridTile* StartTile = EnemyUnit->GetCurrentTile();
	AGridTile* TargetTile = TargetUnit->GetCurrentTile();
	if (!StartTile || !TargetTile)
	{
		return nullptr;
	}

	TArray<AGridTile*> ReachableTiles = GridBoard->FindReachableTiles(StartTile, EnemyUnit->GetMoveRange());
	if (ReachableTiles.Num() == 0)
	{
		return nullptr;
	}

	AGridTile* BestTile = nullptr;
	int32 BestDistance = MAX_int32;

	for (AGridTile* CandidateTile : ReachableTiles)
	{
		if (!CandidateTile)
		{
			continue;
		}

		const int32 CandidateDistance = GetTileDistance(CandidateTile, TargetTile);
		if (CandidateDistance < BestDistance)
		{
			BestDistance = CandidateDistance;
			BestTile = CandidateTile;
		}
	}

	const int32 CurrentDistance = GetTileDistance(StartTile, TargetTile);
	if (BestTile && BestDistance < CurrentDistance)
	{
		return BestTile;
	}

	return nullptr;
}

void AJargonCombatGameMode::RequestEndPlayerTurn()
{
	EndPlayerTurn();
}

bool AJargonCombatGameMode::TryMovePlayerUnitToTile(AGridTile* DestinationTile)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (bPlayerMoveUsed)
	{
		return false;
	}

	if (!GridBoard || !PlayerUnit || !DestinationTile)
	{
		return false;
	}

	AGridTile* StartTile = PlayerUnit->GetCurrentTile();
	if (!StartTile)
	{
		return false;
	}

	if (DestinationTile == StartTile)
	{
		return false;
	}

	if (!DestinationTile->IsWalkable())
	{
		return false;
	}

	const TArray<AGridTile*> Path = GridBoard->BuildPath(StartTile, DestinationTile);
	if (Path.Num() < 2)
	{
		return false;
	}

	const int32 StepsRequired = Path.Num() - 1;
	if (StepsRequired > PlayerUnit->GetMoveRange())
	{
		return false;
	}

	PlayerUnit->MoveAlongPath(Path);
	bPlayerMoveUsed = true;

	RefreshPlayerMovementHighlights();
	return true;
}

bool AJargonCombatGameMode::TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (!Card || !Target || !PlayerUnit)
	{
		return false;
	}

	if (Target->IsDead())
	{
		return false;
	}

	if (Target->GetTeam() == PlayerUnit->GetTeam())
	{
		return false;
	}

	if (Card->Cost > CurrentEnergy)
	{
		UE_LOG(LogTemp, Log, TEXT("Not enough energy to play '%s'. Cost=%d CurrentEnergy=%d"),
			*Card->DisplayName.ToString(), Card->Cost, CurrentEnergy);
		return false;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
	AGridTile* TargetTile = Target->GetCurrentTile();
	if (!PlayerTile || !TargetTile)
	{
		return false;
	}

	const FIntPoint PlayerCoord = PlayerTile->GetCoord();
	const FIntPoint TargetCoord = TargetTile->GetCoord();
	const int32 ManhattanDistance =
		FMath::Abs(PlayerCoord.X - TargetCoord.X) +
		FMath::Abs(PlayerCoord.Y - TargetCoord.Y);

	if (ManhattanDistance > Card->Range)
	{
		UE_LOG(LogTemp, Log, TEXT("Card target out of range. Required <= %d, actual %d."), Card->Range, ManhattanDistance);
		return false;
	}

	switch (Card->TargetType)
	{
	case ECardTargetType::Unit:
		break;

	default:
		return false;
	}

	switch (Card->EffectType)
	{
	case ECardEffectType::Damage:
		CombatPhase = ECombatPhase::Resolving;
		CurrentEnergy -= Card->Cost;
		Target->ApplyDamage(Card->Value);

		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			CombatPhase = ECombatPhase::PlayerTurn;
		}

		RefreshPlayerMovementHighlights();
		return true;

	default:
		return false;
	}
}

void AJargonCombatGameMode::HandleUnitDied(ABattleUnit* DeadUnit)
{
	if (!DeadUnit)
	{
		return;
	}

	if (DeadUnit == PlayerUnit)
	{
		PlayerUnit = nullptr;
		HandleDefeat();
		return;
	}

	const int32 RemovedCount = EnemyUnits.RemoveSingleSwap(DeadUnit);
	if (RemovedCount > 0 && AreAllEnemiesDefeated())
	{
		HandleVictory();
	}
}

void AJargonCombatGameMode::HandleVictory()
{
	if (CombatPhase == ECombatPhase::Victory)
	{
		return;
	}

	CombatPhase = ECombatPhase::Victory;

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat victory occurred but UJargonGameInstance was not available."));
		return;
	}

	const FName PendingEncounterId = GameInstance->GetPendingEncounterId();
	if (!PendingEncounterId.IsNone())
	{
		GameInstance->MarkEncounterCleared(PendingEncounterId);
	}

	GameInstance->PrepareReturnToExploration();
	ReturnToExploration();
}

void AJargonCombatGameMode::HandleDefeat()
{
	if (CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	CombatPhase = ECombatPhase::Defeat;

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat defeat occurred but UJargonGameInstance was not available."));
		return;
	}

	GameInstance->PrepareReturnToExploration();
	ReturnToExploration();
}

void AJargonCombatGameMode::ReturnToExploration()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToExploration failed because UJargonGameInstance was not available."));
		return;
	}

	const FName ReturnMapName = GameInstance->GetReturnMapName();
	if (ReturnMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToExploration failed because ReturnMapName is None."));
		return;
	}

	UGameplayStatics::OpenLevel(this, ReturnMapName);
}