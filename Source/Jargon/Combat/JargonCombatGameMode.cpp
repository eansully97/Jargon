// JargonCombatGameMode.cpp

#include "Combat/JargonCombatGameMode.h"

#include "Combat/CardResolver.h"
#include "Combat/JargonCombatPlayerController.h"
#include "Combat/TacticsCameraPawn.h"
#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Encounters/EncounterTypes.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Grid/BattleTileEffect.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"
#include "Kismet/GameplayStatics.h"
#include "Units/BattleUnit.h"
#include "Units/EnemyBattleUnit.h"
#include "Units/PlayerBattleUnit.h"

AJargonCombatGameMode::AJargonCombatGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AJargonCombatPlayerController::StaticClass();
	CombatPhase = ECombatPhase::BattleStart;
	CurrentRound = 0;
	CurrentEnergy = 0;
	CurrentMaxEnergy = 0;
	CurrentActingEnemy = nullptr;
	DefeatedEnemyCount = 0;
	AccumulatedEnemyKillCurrency = FJargonCurrencyAmount();
	EnergyPerTurn = 1;
	MaxEnergyIncreasePerRound = 1;
	MaxEnergyCap = 3;
	CardsDrawnPerTurn = 1;
	EnemyTurnActionIndex = 0;
	EnemyTurnStartDelay = 0.35f;
	EnemyActionDelay = 0.5f;
	EnemyTurnEndDelay = 0.35f;
}

void AJargonCombatGameMode::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombat();
}

void AJargonCombatGameMode::InitializeCombat()
{
	SetCombatPhase(ECombatPhase::BattleStart);
	SetCurrentEnergy(0);
	ResetCombatRewardState();

	UE_LOG(LogTemp, Log, TEXT("Combat GameMode initialized for world '%s'."), *GetNameSafe(GetWorld()));

	InitializeCameraPawn();
	FindGridBoard();
	SpawnCombatants();
	InitializeCombatantFacing();
	StartBattleFlow();
}

void AJargonCombatGameMode::InitializeCombatantFacing()
{
	if (!PlayerUnit)
	{
		return;
	}

	ABattleUnit* FirstValidEnemy = nullptr;

	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			FirstValidEnemy = EnemyUnit;
			break;
		}
	}

	if (FirstValidEnemy)
	{
		PlayerUnit->FaceLocation(FirstValidEnemy->GetActorLocation());
	}

	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
		{
			continue;
		}

		if (ABattleUnit* TargetUnit = FindPreferredEnemyTarget(EnemyUnit))
		{
			EnemyUnit->FaceLocation(TargetUnit->GetActorLocation());
		}
		else
		{
			EnemyUnit->FaceLocation(PlayerUnit->GetActorLocation());
		}
	}
}

void AJargonCombatGameMode::SetCombatPhase(ECombatPhase NewPhase)
{
	if (CombatPhase == NewPhase)
	{
		return;
	}

	CombatPhase = NewPhase;
	RefreshSelectedFriendlyUnitPresentation();
	OnPhaseChanged.Broadcast(CombatPhase);
	BroadcastPlayerActionAvailabilityChanged();
}

void AJargonCombatGameMode::SetCurrentEnergy(int32 NewEnergy)
{
	NewEnergy = FMath::Max(0, NewEnergy);

	if (CurrentEnergy == NewEnergy)
	{
		return;
	}

	CurrentEnergy = NewEnergy;
	OnEnergyChanged.Broadcast(CurrentEnergy);
}

void AJargonCombatGameMode::AddCurrentEnergy(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	SetCurrentEnergy(FMath::Min(CurrentEnergy + Amount, CurrentMaxEnergy));
}

void AJargonCombatGameMode::SetCurrentActingEnemy(ABattleUnit* NewActingEnemy)
{
	ABattleUnit* NormalizedNewActingEnemy =
		(IsValid(NewActingEnemy) && !NewActingEnemy->IsDead()) ? NewActingEnemy : nullptr;

	if (CurrentActingEnemy == NormalizedNewActingEnemy)
	{
		return;
	}

	if (IsValid(CurrentActingEnemy))
	{
		CurrentActingEnemy->SetActingHighlight(false);
	}

	CurrentActingEnemy = NormalizedNewActingEnemy;

	if (IsValid(CurrentActingEnemy))
	{
		CurrentActingEnemy->SetActingHighlight(true);
	}

	OnCurrentActingEnemyChanged.Broadcast(CurrentActingEnemy);
}

void AJargonCombatGameMode::BroadcastPlayerActionAvailabilityChanged()
{
	OnPlayerActionAvailabilityChanged.Broadcast(HasPlayerMoveRemaining(), HasPlayerAttackRemaining());
}

bool AJargonCombatGameMode::HasPlayerMoveRemaining() const
{
	ABattleUnit* SelectedUnit = SelectedFriendlyUnit.Get();
	return CombatPhase == ECombatPhase::PlayerTurn
		&& IsValid(SelectedUnit)
		&& !SelectedUnit->IsDead()
		&& SelectedUnit->HasMoveActionRemaining();
}

bool AJargonCombatGameMode::HasPlayerAttackRemaining() const
{
	ABattleUnit* SelectedUnit = SelectedFriendlyUnit.Get();
	return CombatPhase == ECombatPhase::PlayerTurn
		&& IsValid(SelectedUnit)
		&& !SelectedUnit->IsDead()
		&& SelectedUnit->HasAttackActionRemaining();
}

bool AJargonCombatGameMode::IsFriendlyUnitSelectable(const ABattleUnit* Unit) const
{
	if (!Unit || Unit->IsDead() || Unit->GetTeam() != ETeam::Player)
	{
		return false;
	}

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (FriendlyUnit == Unit)
		{
			return true;
		}
	}

	return false;
}

ABattleUnit* AJargonCombatGameMode::FindFallbackSelectedFriendlyUnit() const
{
	if (IsFriendlyUnitSelectable(PlayerUnit))
	{
		return PlayerUnit;
	}

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (IsFriendlyUnitSelectable(FriendlyUnit))
		{
			return FriendlyUnit.Get();
		}
	}

	return nullptr;
}

void AJargonCombatGameMode::RefreshSelectedFriendlyUnitPresentation()
{
	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead())
		{
			continue;
		}

		FriendlyUnit->SetHighlightEnabled(false);
	}

	if (CombatPhase != ECombatPhase::PlayerTurn || !IsFriendlyUnitSelectable(SelectedFriendlyUnit))
	{
		return;
	}

	SelectedFriendlyUnit->SetHighlightColor(SelectedFriendlyUnitHighlightColor);
	SelectedFriendlyUnit->SetHighlightEnabled(true);
}

void AJargonCombatGameMode::SetSelectedFriendlyUnit(ABattleUnit* NewSelectedFriendlyUnit)
{
	ABattleUnit* NormalizedSelectedUnit = IsFriendlyUnitSelectable(NewSelectedFriendlyUnit)
		? NewSelectedFriendlyUnit
		: FindFallbackSelectedFriendlyUnit();

	SelectedFriendlyUnit = NormalizedSelectedUnit;
	RefreshSelectedFriendlyUnitPresentation();
	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();
}

bool AJargonCombatGameMode::TrySelectFriendlyUnit(ABattleUnit* FriendlyUnit)
{
	if (CombatPhase != ECombatPhase::PlayerTurn || !IsFriendlyUnitSelectable(FriendlyUnit))
	{
		return false;
	}

	SetSelectedFriendlyUnit(FriendlyUnit);
	return true;
}

bool AJargonCombatGameMode::StartPresentedBasicAttack(
	ABattleUnit* Attacker,
	ABattleUnit* Target,
	bool bReturnToPlayerTurnAfterSequence,
	bool bContinueEnemyTurnAfterSequence)
{
	if (!Attacker || !Target || !Attacker->CanAttackTarget(Target))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ClearBasicAttackTimer();

	PendingAttackAttacker = Attacker;
	PendingAttackTarget = Target;
	bReturnToPlayerTurnAfterAttackSequence = bReturnToPlayerTurnAfterSequence;
	bContinueEnemyTurnAfterAttackSequence = bContinueEnemyTurnAfterSequence;

	Attacker->PlayBasicAttackPresentation(Target);

	const float DamageDelay = FMath::Max(0.f, Attacker->GetBasicAttackDamageDelay());
	const float PresentationDuration = FMath::Max(0.f, Attacker->GetBasicAttackPresentationDuration());
	const float CompletionDelay = FMath::Max(DamageDelay, PresentationDuration);

	if (DamageDelay <= KINDA_SMALL_NUMBER)
	{
		ApplyPresentedBasicAttackDamage();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			BasicAttackDamageTimerHandle,
			this,
			&AJargonCombatGameMode::ApplyPresentedBasicAttackDamage,
			DamageDelay,
			false
		);
	}

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return true;
	}

	if (CompletionDelay <= KINDA_SMALL_NUMBER)
	{
		HandlePresentedBasicAttackCompleted();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			BasicAttackCompletionTimerHandle,
			this,
			&AJargonCombatGameMode::HandlePresentedBasicAttackCompleted,
			CompletionDelay,
			false
		);
	}

	return true;
}

void AJargonCombatGameMode::ApplyPresentedBasicAttackDamage()
{
	ABattleUnit* Attacker = PendingAttackAttacker.Get();
	ABattleUnit* Target = PendingAttackTarget.Get();
	if (IsValid(Attacker) && IsValid(Target))
	{
		Attacker->PerformBasicAttack(Target);
	}
}

void AJargonCombatGameMode::HandlePresentedBasicAttackCompleted()
{
	const bool bShouldReturnToPlayerTurn = bReturnToPlayerTurnAfterAttackSequence;
	const bool bShouldContinueEnemyTurn = bContinueEnemyTurnAfterAttackSequence;
	ClearBasicAttackTimer();

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	if (bShouldReturnToPlayerTurn)
	{
		SetCombatPhase(ECombatPhase::PlayerTurn);
		RefreshPlayerMovementHighlights();
		BroadcastPlayerActionAvailabilityChanged();
		return;
	}

	if (bShouldContinueEnemyTurn && CombatPhase == ECombatPhase::EnemyTurn)
	{
		SetCurrentActingEnemy(nullptr);
		ScheduleNextEnemyAction(0.f);
	}
}

void AJargonCombatGameMode::ScheduleNextEnemyAction(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EnemyTurnTimerHandle);

	if (DelaySeconds <= KINDA_SMALL_NUMBER)
	{
		EnemyTurnTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &AJargonCombatGameMode::ResolveNextEnemyAction);
		return;
	}

	World->GetTimerManager().SetTimer(EnemyTurnTimerHandle, this, &AJargonCombatGameMode::ResolveNextEnemyAction, DelaySeconds, false);
}

void AJargonCombatGameMode::ScheduleEndEnemyTurn(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EnemyTurnTimerHandle);

	if (DelaySeconds <= KINDA_SMALL_NUMBER)
	{
		EnemyTurnTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &AJargonCombatGameMode::EndEnemyTurn);
		return;
	}

	World->GetTimerManager().SetTimer(EnemyTurnTimerHandle, this, &AJargonCombatGameMode::EndEnemyTurn, DelaySeconds, false);
}

void AJargonCombatGameMode::ClearEnemyTurnTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EnemyTurnTimerHandle);
}

void AJargonCombatGameMode::ClearBasicAttackTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(BasicAttackDamageTimerHandle);
	World->GetTimerManager().ClearTimer(BasicAttackCompletionTimerHandle);
	PendingAttackAttacker.Reset();
	PendingAttackTarget.Reset();
	bReturnToPlayerTurnAfterAttackSequence = false;
	bContinueEnemyTurnAfterAttackSequence = false;
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

void AJargonCombatGameMode::RegisterBattleUnitCallbacks(ABattleUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Unit->OnMovementCompleted().RemoveAll(this);
	Unit->OnMovementCompleted().AddUObject(this, &AJargonCombatGameMode::HandleBattleUnitMovementCompleted);
}

void AJargonCombatGameMode::SpawnCombatants()
{
	EnemyUnits.Reset();
	FriendlyUnits.Reset();
	PlayerUnit = nullptr;
	SelectedFriendlyUnit = nullptr;

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

	RegisterBattleUnitCallbacks(PlayerUnit);
	PlayerUnit->PlaceOnTile(PlayerSpawnTile);
	FriendlyUnits.Add(PlayerUnit);

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (GameInstance && GameInstance->HasPendingEncounterData())
	{
		SpawnEnemiesFromPendingEncounter();

		if (EnemyUnits.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Pending encounter produced no valid enemy spawns. Falling back to default enemy spawn."));
			SpawnFallbackEnemy();
		}
	}
	else
	{
		SpawnFallbackEnemy();
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

		AGridTile* SpawnTile = ResolveEnemySpawnTile(SpawnEntry.SpawnCoord);
		if (!SpawnTile)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter could not resolve a valid tile near coord %s."),
				*SpawnEntry.SpawnCoord.ToString());
			continue;
		}

		if (SpawnTile->GetCoord() != SpawnEntry.SpawnCoord)
		{
			UE_LOG(LogTemp, Log, TEXT("SpawnEnemiesFromPendingEncounter resolved fallback tile %s for preferred coord %s."),
				*SpawnTile->GetCoord().ToString(),
				*SpawnEntry.SpawnCoord.ToString());
		}

		ABattleUnit* SpawnedEnemy = SpawnEnemyUnitAtTile(SpawnEntry.UnitClass, SpawnTile);
		if (!SpawnedEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter failed to spawn enemy unit near coord %s."),
				*SpawnEntry.SpawnCoord.ToString());
			continue;
		}

		EnemyUnits.Add(SpawnedEnemy);
	}
}

void AJargonCombatGameMode::SpawnFallbackEnemy()
{
	if (!EnemyUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnFallbackEnemy aborted because EnemyUnitClass is not assigned."));
		return;
	}

	AGridTile* EnemySpawnTile = GridBoard ? GridBoard->GetEnemySpawnTile() : nullptr;
	if (!EnemySpawnTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnFallbackEnemy could not find a valid fallback enemy spawn tile."));
		return;
	}

	ABattleUnit* SpawnedEnemy = SpawnEnemyUnitAtTile(EnemyUnitClass, EnemySpawnTile);
	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnFallbackEnemy failed to spawn EnemyUnitClass."));
		return;
	}

	EnemyUnits.Add(SpawnedEnemy);
}

AGridTile* AJargonCombatGameMode::ResolveEnemySpawnTile(const FHexCoord& PreferredCoord) const
{
	return GridBoard ? GridBoard->FindNearestWalkableTile(PreferredCoord) : nullptr;
}

ABattleUnit* AJargonCombatGameMode::SpawnEnemyUnitAtTile(TSubclassOf<ABattleUnit> UnitClass, AGridTile* SpawnTile)
{
	if (!UnitClass || !SpawnTile || !SpawnTile->IsWalkable())
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABattleUnit* SpawnedEnemy = World->SpawnActor<ABattleUnit>(
		UnitClass,
		SpawnTile->GetUnitStandLocation(),
		FRotator::ZeroRotator
	);

	if (!SpawnedEnemy)
	{
		return nullptr;
	}

	RegisterBattleUnitCallbacks(SpawnedEnemy);
	SpawnedEnemy->PlaceOnTile(SpawnTile);
	return SpawnedEnemy;
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

void AJargonCombatGameMode::ResetCombatRewardState()
{
	DefeatedEnemyCount = 0;
	AccumulatedEnemyKillCurrency = FJargonCurrencyAmount();
}

void AJargonCombatGameMode::AccumulateEnemyKillReward(ABattleUnit* DeadEnemy)
{
	if (!DeadEnemy)
	{
		return;
	}

	DefeatedEnemyCount++;

	const FJargonCurrencyAmount KillReward = GetEnemyKillCurrencyReward(DeadEnemy);
	AccumulatedEnemyKillCurrency = FJargonCurrencyAmount::FromTotalCopper(
		AccumulatedEnemyKillCurrency.GetTotalCopperValue() + KillReward.GetTotalCopperValue()
	);
}

FJargonCurrencyAmount AJargonCombatGameMode::GetEnemyKillCurrencyReward(const ABattleUnit* DeadEnemy) const
{
	if (const AEnemyBattleUnit* EnemyUnit = Cast<AEnemyBattleUnit>(DeadEnemy))
	{
		return EnemyUnit->GetKillCurrencyReward();
	}

	FJargonCurrencyAmount FallbackReward;
	FallbackReward.Copper = 1;
	return FallbackReward;
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

	StartPlayerTurn();
}

void AJargonCombatGameMode::ClearPendingMovementSequence()
{
	PendingMovementContext = EPendingMovementContext::None;
	PendingMovementUnit.Reset();
}

bool AJargonCombatGameMode::StartPlayerControlledMoveSequence(
	ABattleUnit* MovingUnit,
	const TArray<AGridTile*>& Path,
	bool bConsumeMoveAction)
{
	if (!MovingUnit || MovingUnit->IsDead() || Path.Num() < 2)
	{
		return false;
	}

	PendingMovementContext = EPendingMovementContext::PlayerControlled;
	PendingMovementUnit = MovingUnit;
	SetCombatPhase(ECombatPhase::Resolving);

	if (!MovingUnit->MoveAlongPath(Path))
	{
		ClearPendingMovementSequence();

		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
		}

		return false;
	}

	if (bConsumeMoveAction)
	{
		MovingUnit->ConsumeMoveAction();
	}

	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();
	return true;
}

bool AJargonCombatGameMode::StartEnemyMoveSequence(ABattleUnit* EnemyUnit, const TArray<AGridTile*>& Path)
{
	if (!EnemyUnit || EnemyUnit->IsDead() || Path.Num() < 2)
	{
		return false;
	}

	PendingMovementContext = EPendingMovementContext::Enemy;
	PendingMovementUnit = EnemyUnit;
	if (EnemyUnit->MoveAlongPath(Path))
	{
		return true;
	}

	ClearPendingMovementSequence();
	return false;
}

void AJargonCombatGameMode::HandleBattleUnitMovementCompleted(ABattleUnit* MovedUnit)
{
	if (!MovedUnit || PendingMovementUnit.Get() != MovedUnit)
	{
		return;
	}

	const EPendingMovementContext CompletedContext = PendingMovementContext;
	ClearPendingMovementSequence();

	switch (CompletedContext)
	{
	case EPendingMovementContext::PlayerControlled:
		HandlePlayerControlledMoveCompleted(MovedUnit);
		break;

	case EPendingMovementContext::Enemy:
		HandleEnemyMoveCompleted(MovedUnit);
		break;

	case EPendingMovementContext::None:
	default:
		break;
	}
}

void AJargonCombatGameMode::HandlePlayerControlledMoveCompleted(ABattleUnit* MovedUnit)
{
	if (!MovedUnit)
	{
		return;
	}

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCombatPhase(ECombatPhase::PlayerTurn);
	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();
}

void AJargonCombatGameMode::HandleEnemyMoveCompleted(ABattleUnit* MovedUnit)
{
	if (!MovedUnit)
	{
		return;
	}

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	if (CombatPhase != ECombatPhase::EnemyTurn)
	{
		return;
	}

	if (MovedUnit->IsDead())
	{
		SetCurrentActingEnemy(nullptr);
		ScheduleNextEnemyAction(0.f);
		return;
	}

	ABattleUnit* PostMoveTarget = FindPreferredEnemyTarget(MovedUnit);
	if (PostMoveTarget && MovedUnit->CanAttackTarget(PostMoveTarget))
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' attacks '%s' after moving for %d damage."),
			*GetNameSafe(MovedUnit),
			*GetNameSafe(PostMoveTarget),
			MovedUnit->GetAttackDamage());

		if (StartPresentedBasicAttack(MovedUnit, PostMoveTarget, false, true))
		{
			return;
		}
	}

	SetCurrentActingEnemy(nullptr);
	ScheduleNextEnemyAction(0.f);
}

void AJargonCombatGameMode::StartPlayerTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	FriendlyUnits.RemoveAllSwap([](const TObjectPtr<ABattleUnit>& FriendlyUnit)
	{
		return !IsValid(FriendlyUnit) || FriendlyUnit->IsDead();
	});

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead())
		{
			continue;
		}

		FriendlyUnit->ClearTemporaryShield();
		FriendlyUnit->ResetTurnActions();
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearBasicAttackTimer();
	ClearPendingMovementSequence();
	EnemyTurnActionIndex = 0;
	SetCombatPhase(ECombatPhase::PlayerTurn);
	SetSelectedFriendlyUnit(SelectedFriendlyUnit);
	CurrentMaxEnergy = CalculateMaxEnergyForRound(CurrentRound);
	SetCurrentEnergy(CurrentMaxEnergy);
	NotifyPlayerTurnStartTileEffects();

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (CombatPC)
	{
		CombatPC->ClearSelectedCard();
		CombatPC->DrawCards(CardsDrawnPerTurn);
	}

	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();

	UE_LOG(LogTemp, Log, TEXT("Player Turn Start - Round %d, Energy %d"), CurrentRound, CurrentEnergy);
}

void AJargonCombatGameMode::EndPlayerTurn()
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return;
	}

	SetCombatPhase(ECombatPhase::Resolving);
	RefreshPlayerMovementHighlights();

	StartEnemyTurn();
}

void AJargonCombatGameMode::StartEnemyTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCombatPhase(ECombatPhase::EnemyTurn);

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
	SetCurrentActingEnemy(nullptr);
	ClearPendingMovementSequence();
	EnemyTurnActionIndex = 0;
	ScheduleNextEnemyAction(EnemyTurnStartDelay);
}

void AJargonCombatGameMode::ResolveNextEnemyAction()
{
	if (CombatPhase != ECombatPhase::EnemyTurn)
	{
		return;
	}

	if (!PlayerUnit || PlayerUnit->IsDead())
	{
		SetCurrentActingEnemy(nullptr);
		ClearEnemyTurnTimer();
		HandleDefeat();
		return;
	}

	SetCurrentActingEnemy(nullptr);

	while (EnemyTurnActionIndex < EnemyUnits.Num())
	{
		ABattleUnit* EnemyUnit = EnemyUnits[EnemyTurnActionIndex].Get();
		EnemyTurnActionIndex++;

		if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
		{
			continue;
		}

		if (!PlayerUnit || PlayerUnit->IsDead())
		{
			HandleDefeat();
			return;
		}

		SetCurrentActingEnemy(EnemyUnit);
		const bool bStartedAction = ResolveSingleEnemyAction(EnemyUnit);

		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			ClearEnemyTurnTimer();
			return;
		}

		if (bStartedAction)
		{
			return;
		}

		SetCurrentActingEnemy(nullptr);
	}

	SetCurrentActingEnemy(nullptr);
	ScheduleEndEnemyTurn(EnemyTurnEndDelay);
}

bool AJargonCombatGameMode::ResolveSingleEnemyAction(ABattleUnit* EnemyUnit)
{
	if (!EnemyUnit || EnemyUnit->IsDead())
	{
		return false;
	}

	ABattleUnit* TargetUnit = FindPreferredEnemyTarget(EnemyUnit);
	if (!TargetUnit)
	{
		return false;
	}

	if (EnemyUnit->CanAttackTarget(TargetUnit))
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' attacks '%s' for %d damage."),
			*GetNameSafe(EnemyUnit),
			*GetNameSafe(TargetUnit),
			EnemyUnit->GetAttackDamage());

		return StartPresentedBasicAttack(EnemyUnit, TargetUnit, false, true);
	}

	AGridTile* BestDestination = FindBestEnemyMoveDestination(EnemyUnit, TargetUnit);
	if (BestDestination && EnemyUnit->GetCurrentTile())
	{
		const TArray<AGridTile*> Path = GridBoard->BuildPath(EnemyUnit->GetCurrentTile(), BestDestination);
		if (Path.Num() >= 2)
		{
			if (StartEnemyMoveSequence(EnemyUnit, Path))
			{
				UE_LOG(LogTemp, Log, TEXT("Enemy '%s' moves to %s."),
					*GetNameSafe(EnemyUnit),
					*BestDestination->GetCoord().ToString());

				return true;
			}
		}
	}

	return false;
}

void AJargonCombatGameMode::EndEnemyTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearPendingMovementSequence();
	EnemyTurnActionIndex = 0;
	CurrentRound++;
	StartPlayerTurn();
}

void AJargonCombatGameMode::NotifyPlayerTurnStartTileEffects()
{
	ActiveTileEffects.RemoveAllSwap([](const TObjectPtr<ABattleTileEffect>& TileEffect)
	{
		return !IsValid(TileEffect);
	});

	TArray<TObjectPtr<ABattleTileEffect>> TileEffectsSnapshot = ActiveTileEffects;
	for (const TObjectPtr<ABattleTileEffect>& TileEffect : TileEffectsSnapshot)
	{
		if (!IsValid(TileEffect))
		{
			continue;
		}

		TileEffect->HandlePlayerTurnStart(this);
	}
}

void AJargonCombatGameMode::RefreshPlayerMovementHighlights()
{
	if (!GridBoard)
	{
		return;
	}

	GridBoard->ClearHighlights();

	if (!IsFriendlyUnitSelectable(SelectedFriendlyUnit) || !SelectedFriendlyUnit->GetCurrentTile())
	{
		return;
	}

	if (CombatPhase == ECombatPhase::PlayerTurn && SelectedFriendlyUnit->HasMoveActionRemaining())
	{
		GridBoard->HighlightReachableTilesFrom(SelectedFriendlyUnit->GetCurrentTile(), SelectedFriendlyUnit->GetMoveRange());
	}
	else
	{
		SelectedFriendlyUnit->GetCurrentTile()->SetHighlightState(ETileHighlightState::Selected);
	}
}

void AJargonCombatGameMode::RefreshCardTargetHighlights(ABattleUnit* SourceUnit, const UCardDefinition* Card)
{
	if (!GridBoard)
	{
		return;
	}

	GridBoard->ClearHighlights();

	if (!SourceUnit || !Card)
	{
		return;
	}

	AGridTile* SourceTile = SourceUnit->GetCurrentTile();
	if (!SourceTile)
	{
		return;
	}

	if (Card->TargetType == ECardTargetType::Self)
	{
		SourceTile->SetHighlightState(ETileHighlightState::Selected);
		return;
	}

	GridBoard->HighlightTilesInRangeFrom(SourceTile, Card->Range);
}

AJargonCombatPlayerController* AJargonCombatGameMode::GetCombatPlayerController() const
{
	return Cast<AJargonCombatPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

bool AJargonCombatGameMode::DrawCardsForPlayer(int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (!CombatPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("AJargonCombatGameMode::DrawCardsForPlayer failed because no combat player controller was found."));
		return false;
	}

	CombatPC->DrawCards(Count);
	return true;
}

int32 AJargonCombatGameMode::CalculateMaxEnergyForRound(int32 RoundNumber) const
{
	const int32 RoundIndex = FMath::Max(0, RoundNumber - 1);
	const int32 UncappedMaxEnergy = EnergyPerTurn + (RoundIndex * MaxEnergyIncreasePerRound);

	if (MaxEnergyCap > 0)
	{
		return FMath::Clamp(UncappedMaxEnergy, 0, MaxEnergyCap);
	}

	return FMath::Max(0, UncappedMaxEnergy);
}

ABattleUnit* AJargonCombatGameMode::FindPreferredEnemyTarget(ABattleUnit* EnemyUnit) const
{
	if (!GridBoard || !EnemyUnit || EnemyUnit->IsDead() || !EnemyUnit->GetCurrentTile())
	{
		return nullptr;
	}

	ABattleUnit* BestTarget = nullptr;
	int32 BestScore = MAX_int32;

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		ABattleUnit* CandidateUnit = FriendlyUnit.Get();
		if (!IsFriendlyUnitSelectable(CandidateUnit) || !CandidateUnit->GetCurrentTile())
		{
			continue;
		}

		const int32 DistanceToTarget = GridBoard->GetTileDistance(EnemyUnit->GetCurrentTile(), CandidateUnit->GetCurrentTile());
		if (DistanceToTarget == MAX_int32)
		{
			continue;
		}

		int32 TargetScore = DistanceToTarget * 10;

		if (EnemyUnit->CanAttackTarget(CandidateUnit))
		{
			TargetScore -= 1000;
		}

		TargetScore += CandidateUnit->GetCurrentHP();

		if (CandidateUnit == PlayerUnit)
		{
			TargetScore -= 1;
		}

		if (TargetScore < BestScore)
		{
			BestScore = TargetScore;
			BestTarget = CandidateUnit;
		}
	}

	return BestTarget;
}

ABattleTileEffect* AJargonCombatGameMode::SpawnPersistentTileEffectFromClass(
	TSubclassOf<ABattleTileEffect> TileEffectClass,
	const UCardDefinition* Card,
	const ABattleUnit* SourceUnit,
	AGridTile* TargetTile,
	ECardCategory EffectCategory,
	int32 EffectValue,
	int32 EffectRadius)
{
	if (!Card || !SourceUnit || !TargetTile || !TileEffectClass)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABattleTileEffect* SpawnedEffect = World->SpawnActor<ABattleTileEffect>(
		TileEffectClass,
		TargetTile->GetActorLocation(),
		FRotator::ZeroRotator
	);

	if (!SpawnedEffect)
	{
		return nullptr;
	}

	SpawnedEffect->InitializeFromCard(
		const_cast<UCardDefinition*>(Card),
		SourceUnit->GetTeam(),
		EffectCategory,
		EffectValue,
		EffectRadius);
	SpawnedEffect->PlaceOnTile(TargetTile);
	ActiveTileEffects.Add(SpawnedEffect);
	return SpawnedEffect;
}

ABattleUnit* AJargonCombatGameMode::SpawnSummonedUnitFromClass(
	TSubclassOf<ABattleUnit> UnitClass,
	const ABattleUnit* SourceUnit,
	AGridTile* TargetTile,
	bool bAttackExhaustedOnSpawn)
{
	if (!SourceUnit || !TargetTile || !UnitClass)
	{
		return nullptr;
	}

	if (!TargetTile->IsWalkable())
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABattleUnit* SpawnedUnit = World->SpawnActor<ABattleUnit>(
		UnitClass,
		TargetTile->GetUnitStandLocation(),
		FRotator::ZeroRotator
	);

	if (!SpawnedUnit)
	{
		return nullptr;
	}

	RegisterBattleUnitCallbacks(SpawnedUnit);
	SpawnedUnit->PlaceOnTile(TargetTile);

	if (SpawnedUnit->GetTeam() == ETeam::Player)
	{
		FriendlyUnits.AddUnique(SpawnedUnit);

		if (bAttackExhaustedOnSpawn)
		{
			SpawnedUnit->ConsumeAttackAction();
		}
	}

	ABattleUnit* FirstValidEnemy = nullptr;
	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			FirstValidEnemy = EnemyUnit;
			break;
		}
	}

	if (FirstValidEnemy)
	{
		SpawnedUnit->FaceLocation(FirstValidEnemy->GetActorLocation());
	}
	else
	{
		SpawnedUnit->FaceLocation(SourceUnit->GetActorLocation());
	}

	return SpawnedUnit;
}

void AJargonCombatGameMode::NotifyTileEffectsUnitEntered(ABattleUnit* EnteringUnit, AGridTile* EnteredTile)
{
	if (!EnteringUnit || !EnteredTile)
	{
		return;
	}

	ActiveTileEffects.RemoveAllSwap([](const TObjectPtr<ABattleTileEffect>& TileEffect)
	{
		return !IsValid(TileEffect);
	});

	TArray<TObjectPtr<ABattleTileEffect>> TileEffectsSnapshot = EnteredTile->GetTileEffects();
	for (const TObjectPtr<ABattleTileEffect>& TileEffect : TileEffectsSnapshot)
	{
		if (!IsValid(TileEffect))
		{
			continue;
		}

		TileEffect->HandleUnitEnteredTile(this, EnteringUnit);
	}
}

bool AJargonCombatGameMode::TryPlayCardWithResolvedTile(
	UCardDefinition* Card,
	AGridTile* TileTarget,
	ABattleUnit* ExplicitUnitTarget,
	bool bSkipRangeValidation)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (!Card || !PlayerUnit || !TileTarget)
	{
		return false;
	}

	if (!bSkipRangeValidation && !Card->UsesBoardTileTargeting())
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPlayCardWithResolvedTile received non-board-target card '%s'."),
			*Card->DisplayName.ToString());
		return false;
	}

	if (Card->Cost > CurrentEnergy)
	{
		UE_LOG(LogTemp, Log, TEXT("Not enough energy to play '%s'. Cost=%d CurrentEnergy=%d"),
			*Card->DisplayName.ToString(), Card->Cost, CurrentEnergy);
		return false;
	}

	if (!bSkipRangeValidation)
	{
		if (!GridBoard)
		{
			return false;
		}

		AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
		if (!PlayerTile)
		{
			return false;
		}

		const int32 TargetDistance = GridBoard->GetTileDistance(PlayerTile, TileTarget);
		if (TargetDistance > Card->Range)
		{
			UE_LOG(LogTemp, Log, TEXT("Card target out of range. Required <= %d, actual %d."),
				Card->Range, TargetDistance);
			return false;
		}
	}

	ABattleUnit* ResolvedUnitTarget = ExplicitUnitTarget ? ExplicitUnitTarget : TileTarget->GetOccupyingUnit();
	if (Card->RequiresUnitOnTargetTile() && !ResolvedUnitTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("Card '%s' requires a unit on the targeted tile."),
			*Card->DisplayName.ToString());
		return false;
	}

	if (Card->RequiresEmptyTargetTile() && !TileTarget->IsWalkable())
	{
		UE_LOG(LogTemp, Log, TEXT("Card '%s' requires an empty walkable target tile."),
			*Card->DisplayName.ToString());
		return false;
	}

	FCardResolveContext ResolveContext;
	ResolveContext.GameMode = this;
	ResolveContext.SourceUnit = PlayerUnit;
	ResolveContext.UnitTarget = ResolvedUnitTarget;
	ResolveContext.TileTarget = TileTarget;

	FCardResolveResult ResolveResult;

	SetCombatPhase(ECombatPhase::Resolving);

	const bool bResolved = FCardResolver::ResolveCard(Card, ResolveContext, ResolveResult);
	if (!bResolved)
	{
		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
		}
		return false;
	}

	if (ResolveResult.bConsumeEnergy)
	{
		SetCurrentEnergy(CurrentEnergy - Card->Cost);
	}

	if (ResolveResult.EnergyGainAfterCost > 0)
	{
		AddCurrentEnergy(ResolveResult.EnergyGainAfterCost);
	}

	if (ResolveResult.bConsumePlayerMove)
	{
		PlayerUnit->ConsumeMoveAction();
	}

	if (!ResolveResult.bContinuesAsynchronously &&
		CombatPhase != ECombatPhase::Victory &&
		CombatPhase != ECombatPhase::Defeat)
	{
		SetCombatPhase(ECombatPhase::PlayerTurn);
	}

	RefreshPlayerMovementHighlights();
	return true;
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
	int32 BestScore = MAX_int32;

	for (AGridTile* CandidateTile : ReachableTiles)
	{
		if (!CandidateTile)
		{
			continue;
		}

		const int32 CandidateScore = GetEnemyTileScore(EnemyUnit, CandidateTile, TargetUnit);
		if (CandidateScore < BestScore)
		{
			BestScore = CandidateScore;
			BestTile = CandidateTile;
		}
	}

	// Only move if the best tile is meaningfully better than staying put.
	const int32 CurrentScore = GetEnemyTileScore(EnemyUnit, StartTile, TargetUnit);
	if (BestTile && BestScore < CurrentScore)
	{
		return BestTile;
	}

	return nullptr;
}

int32 AJargonCombatGameMode::GetPreferredEnemyDistance(const ABattleUnit* EnemyUnit) const
{
	if (!EnemyUnit)
	{
		return 1;
	}

	return FMath::Max(1, EnemyUnit->GetAttackRange());
}

bool AJargonCombatGameMode::CanUnitAttackFromTile(
	const ABattleUnit* EnemyUnit,
	const AGridTile* FromTile,
	const ABattleUnit* TargetUnit
) const
{
	if (!EnemyUnit || !FromTile || !TargetUnit || !TargetUnit->GetCurrentTile())
	{
		return false;
	}

	return GridBoard && GridBoard->AreTilesWithinRange(FromTile, TargetUnit->GetCurrentTile(), EnemyUnit->GetAttackRange());
}

int32 AJargonCombatGameMode::GetEnemyTileScore(
	const ABattleUnit* EnemyUnit,
	const AGridTile* CandidateTile,
	const ABattleUnit* TargetUnit
) const
{
	if (!EnemyUnit || !CandidateTile || !TargetUnit || !TargetUnit->GetCurrentTile())
	{
		return MAX_int32;
	}

	const AGridTile* TargetTile = TargetUnit->GetCurrentTile();
	const int32 PreferredDistance = GetPreferredEnemyDistance(EnemyUnit);
	const int32 DistanceToTarget = GridBoard
		? GridBoard->GetTileDistance(CandidateTile, TargetTile)
		: MAX_int32;
	if (DistanceToTarget == MAX_int32)
	{
		return MAX_int32;
	}

	int32 Score = 0;

	// Main goal: end near preferred range.
	const int32 DistanceError = FMath::Abs(DistanceToTarget - PreferredDistance);
	Score += DistanceError * 10;

	// Strong preference for tiles that allow an attack immediately.
	if (CanUnitAttackFromTile(EnemyUnit, CandidateTile, TargetUnit))
	{
		Score -= 6;
	}

	// Mild penalty for being too close if this is a ranged unit.
	if (EnemyUnit->GetAttackRange() > 1 && DistanceToTarget <= 1)
	{
		Score += 8;
	}

	for (const TObjectPtr<ABattleUnit>& OtherEnemy : EnemyUnits)
	{
		if (!IsValid(OtherEnemy) || OtherEnemy == EnemyUnit || OtherEnemy->IsDead())
		{
			continue;
		}

		AGridTile* OtherTile = OtherEnemy->GetCurrentTile();
		if (!OtherTile)
		{
			continue;
		}

		const int32 DistanceToOther = GridBoard
			? GridBoard->GetTileDistance(CandidateTile, OtherTile)
			: MAX_int32;
		if (DistanceToOther == MAX_int32)
		{
			continue;
		}

		// Slight penalty for standing adjacent to allies.
		if (DistanceToOther <= 1)
		{
			Score += 2;
		}
	}

	return Score;
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

	if (!IsFriendlyUnitSelectable(SelectedFriendlyUnit) || !SelectedFriendlyUnit->HasMoveActionRemaining())
	{
		return false;
	}

	if (!GridBoard || !DestinationTile)
	{
		return false;
	}

	AGridTile* StartTile = SelectedFriendlyUnit->GetCurrentTile();
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
	if (StepsRequired > SelectedFriendlyUnit->GetMoveRange())
	{
		return false;
	}

	return StartPlayerControlledMoveSequence(SelectedFriendlyUnit, Path, true);
}

bool AJargonCombatGameMode::TryBasicAttackWithPlayerUnit(ABattleUnit* Target)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (!IsFriendlyUnitSelectable(SelectedFriendlyUnit) || !SelectedFriendlyUnit->HasAttackActionRemaining())
	{
		return false;
	}

	if (!Target || Target->IsDead())
	{
		return false;
	}

	if (Target == SelectedFriendlyUnit || Target->GetTeam() == SelectedFriendlyUnit->GetTeam())
	{
		return false;
	}

	if (!SelectedFriendlyUnit->CanAttackTarget(Target))
	{
		UE_LOG(LogTemp, Log, TEXT("Basic attack target is out of range."));
		return false;
	}

	SetCombatPhase(ECombatPhase::Resolving);

	if (!StartPresentedBasicAttack(SelectedFriendlyUnit, Target, true, false))
	{
		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
		}
		return false;
	}

	SelectedFriendlyUnit->ConsumeAttackAction();
	BroadcastPlayerActionAvailabilityChanged();
	RefreshPlayerMovementHighlights();
	return true;
}

bool AJargonCombatGameMode::TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target)
{
	if (!Card || !Target)
	{
		return false;
	}

	AGridTile* TargetTile = Target->GetCurrentTile();
	return TryPlayCardWithResolvedTile(Card, TargetTile, Target, false);
}

bool AJargonCombatGameMode::TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget)
{
	if (!Card || !TileTarget)
	{
		return false;
	}

	if (!Card->UsesBoardTileTargeting())
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPlayCardOnTile only supports board-target cards."));
		return false;
	}

	return TryPlayCardWithResolvedTile(Card, TileTarget, TileTarget->GetOccupyingUnit(), false);
}

bool AJargonCombatGameMode::TryPlayCardOnSelf(UCardDefinition* Card)
{
	if (!Card || !PlayerUnit)
	{
		return false;
	}

	if (Card->TargetType != ECardTargetType::Self)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPlayCardOnSelf only supports Self target cards."));
		return false;
	}

	if (Card->Cost > CurrentEnergy)
	{
		UE_LOG(LogTemp, Log, TEXT("Not enough energy to play '%s'. Cost=%d CurrentEnergy=%d"),
			*Card->DisplayName.ToString(), Card->Cost, CurrentEnergy);
		return false;
	}

	return TryPlayCardWithResolvedTile(Card, PlayerUnit->GetCurrentTile(), PlayerUnit, true);
}

void AJargonCombatGameMode::HandleUnitDied(ABattleUnit* DeadUnit)
{
	if (!DeadUnit)
	{
		return;
	}

	if (PendingMovementUnit.Get() == DeadUnit)
	{
		const EPendingMovementContext CompletedContext = PendingMovementContext;
		ClearPendingMovementSequence();
		const bool bShouldRestorePlayerTurnAfterMovement =
			(CompletedContext == EPendingMovementContext::PlayerControlled) &&
			(DeadUnit != PlayerUnit);

		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			if (bShouldRestorePlayerTurnAfterMovement)
			{
				SetCombatPhase(ECombatPhase::PlayerTurn);
				RefreshPlayerMovementHighlights();
				BroadcastPlayerActionAvailabilityChanged();
			}
			else if (CompletedContext == EPendingMovementContext::Enemy && CombatPhase == ECombatPhase::EnemyTurn)
			{
				SetCurrentActingEnemy(nullptr);
				ScheduleNextEnemyAction(0.f);
			}
		}
	}

	const bool bWasSelectedFriendlyUnit = (DeadUnit == SelectedFriendlyUnit);
	FriendlyUnits.RemoveSingleSwap(DeadUnit);

	if (DeadUnit == PlayerUnit)
	{
		SelectedFriendlyUnit = nullptr;
		RefreshSelectedFriendlyUnitPresentation();
		PlayerUnit = nullptr;
		HandleDefeat();
		return;
	}

	if (DeadUnit == CurrentActingEnemy)
	{
		SetCurrentActingEnemy(nullptr);
	}

	const int32 RemovedCount = EnemyUnits.RemoveSingleSwap(DeadUnit);
	if (RemovedCount > 0)
	{
		AccumulateEnemyKillReward(DeadUnit);

		if (AreAllEnemiesDefeated())
		{
			HandleVictory();
			return;
		}
	}

	if (bWasSelectedFriendlyUnit)
	{
		SetSelectedFriendlyUnit(nullptr);
	}
}

void AJargonCombatGameMode::HandleVictory()
{
	if (CombatPhase == ECombatPhase::Victory)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearBasicAttackTimer();
	ClearPendingMovementSequence();
	SetCombatPhase(ECombatPhase::Victory);

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

	GameInstance->HandleCombatVictory(AccumulatedEnemyKillCurrency, DefeatedEnemyCount);
	ReturnToExploration();
}

void AJargonCombatGameMode::HandleDefeat()
{
	if (CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearBasicAttackTimer();
	ClearPendingMovementSequence();
	SetCombatPhase(ECombatPhase::Defeat);

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat defeat occurred but UJargonGameInstance was not available."));
		return;
	}

	GameInstance->HandleCombatDefeat(AccumulatedEnemyKillCurrency, DefeatedEnemyCount);
	ReturnToExploration();
}

void AJargonCombatGameMode::ReturnToExploration()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnFromCombat failed because UJargonGameInstance was not available."));
		return;
	}

	const FName DestinationMapName = GameInstance->GetPostCombatDestinationMapName();
	if (DestinationMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnFromCombat failed because no destination map name was available."));
		return;
	}

	UGameplayStatics::OpenLevel(this, DestinationMapName);
}
