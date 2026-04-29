// ExplorationEnemyCharacter.cpp

#include "Exploration/ExplorationEnemyCharacter.h"

#include "AIController.h"
#include "Components/SphereComponent.h"
#include "Core/JargonGameInstance.h"
#include "Encounters/EncounterDefinition.h"
#include "Encounters/EncounterTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

AExplorationEnemyCharacter::AExplorationEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AggroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AggroSphere"));
	AggroSphere->SetupAttachment(GetRootComponent());
	AggroSphere->SetSphereRadius(180.f);
	AggroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AggroSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AggroSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AggroSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AggroSphere->SetGenerateOverlapEvents(true);

	bEncounterStarting = false;
	bEnableScoutMovement = true;
	ScoutRadius = 800.f;
	MinWaitTime = 1.f;
	MaxWaitTime = 3.f;
	AcceptanceRadius = 100.f;
	MoveSpeed = 150.f;
	bEnableSimplePacing = false;
	PatrolOffset = FVector(300.f, 0.f, 0.f);
	PatrolSpeed = 120.f;
	bMovingToOffset = true;
	ScoutWaitRemaining = 0.f;
	bScoutMoveInProgress = false;
}

void AExplorationEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (ensure(AggroSphere))
	{
		AggroSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&AExplorationEnemyCharacter::OnAggroSphereBeginOverlap
		);
	}

	PatrolStartLocation = GetActorLocation();
	PatrolTargetLocation = PatrolStartLocation + PatrolOffset;
	ScoutOriginLocation = GetActorLocation();

	if (bEnableSimplePacing && !bEnableScoutMovement)
	{
		bEnableScoutMovement = true;
		ScoutRadius = FMath::Max(ScoutRadius, PatrolOffset.Size());
		MoveSpeed = PatrolSpeed > 0.f ? PatrolSpeed : MoveSpeed;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (MoveSpeed > 0.f)
		{
			MovementComponent->MaxWalkSpeed = MoveSpeed;
		}
	}

	if (EncounterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' has no EncounterId set."), *GetName());
	}

	if (!EncounterDefinition && EncounterDefinitionPool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' has no EncounterDefinition or EncounterDefinitionPool assigned."), *GetName());
	}

	if (UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>())
	{
		if (GameInstance->IsEncounterCleared(EncounterId))
		{
			DisableEncounter();
			SetActorHiddenInGame(true);
			return;
		}
	}

	BeginScoutMovement();
}

void AExplorationEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateScoutMovement(DeltaSeconds);
}

void AExplorationEnemyCharacter::OnAggroSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bEncounterStarting)
	{
		return;
	}

	if (!IsValidTriggeringActor(OtherActor))
	{
		return;
	}

	StartEncounterForPlayer(OtherActor);
}

bool AExplorationEnemyCharacter::IsValidTriggeringActor(AActor* OtherActor) const
{
	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return false;
	}

	return Pawn->IsPlayerControlled();
}

void AExplorationEnemyCharacter::DisableEncounter()
{
	if (AggroSphere)
	{
		AggroSphere->SetGenerateOverlapEvents(false);
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AAIController* ScoutController = Cast<AAIController>(GetController()))
	{
		ScoutController->StopMovement();
	}

	bEncounterStarting = true;
	SetActorTickEnabled(false);
}

void AExplorationEnemyCharacter::StartEncounterForPlayer(AActor* TriggeringActor)
{
	if (EncounterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' cannot start encounter because EncounterId is None."), *GetName());
		return;
	}

	UEncounterDefinition* SelectedEncounterDefinition = ResolveEncounterDefinitionToStart();
	if (!SelectedEncounterDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' cannot start encounter because no valid encounter definition was resolved."), *GetName());
		return;
	}

	if (!SelectedEncounterDefinition->IsValidDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' resolved an invalid EncounterDefinition."), *GetName());
		return;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' could not find UJargonGameInstance."), *GetName());
		return;
	}

	if (GameInstance->IsEncounterCleared(EncounterId))
	{
		DisableEncounter();
		SetActorHiddenInGame(true);
		return;
	}

	const APawn* TriggeringPawn = Cast<APawn>(TriggeringActor);
	if (!TriggeringPawn)
	{
		return;
	}

	FPendingEncounterRuntimeData PendingEncounter;
	PendingEncounter.EncounterId = EncounterId;
	PendingEncounter.CombatMapName = SelectedEncounterDefinition->CombatMapName;
	PendingEncounter.VictoryCurrencyReward = SelectedEncounterDefinition->VictoryCurrencyReward;

	for (const FEncounterEnemySpawn& SpawnEntry : SelectedEncounterDefinition->EnemySpawns)
	{
		if (SpawnEntry.IsValid())
		{
			PendingEncounter.EnemySpawns.Add(SpawnEntry);
		}
	}

	if (!PendingEncounter.HasConfiguredCombatEncounter())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' failed to build valid pending encounter runtime data."), *GetName());
		return;
	}

	bEncounterStarting = true;
	DisableEncounter();

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	const FTransform SourceTransform = TriggeringPawn->GetActorTransform();

	GameInstance->StartEncounterWithRuntimeData(
		PendingEncounter,
		FName(*CurrentLevelName),
		SourceTransform
	);

	UGameplayStatics::OpenLevel(this, PendingEncounter.CombatMapName);
}

UEncounterDefinition* AExplorationEnemyCharacter::ResolveEncounterDefinitionToStart() const
{
	TArray<UEncounterDefinition*> ValidEncounterDefinitions;
	ValidEncounterDefinitions.Reserve(EncounterDefinitionPool.Num());

	for (UEncounterDefinition* CandidateDefinition : EncounterDefinitionPool)
	{
		if (IsValid(CandidateDefinition) && CandidateDefinition->IsValidDefinition())
		{
			ValidEncounterDefinitions.Add(CandidateDefinition);
		}
	}

	if (ValidEncounterDefinitions.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, ValidEncounterDefinitions.Num() - 1);
		return ValidEncounterDefinitions[RandomIndex];
	}

	return (IsValid(EncounterDefinition) && EncounterDefinition->IsValidDefinition())
		? EncounterDefinition
		: nullptr;
}

void AExplorationEnemyCharacter::BeginScoutMovement()
{
	if (!bEnableScoutMovement || bEncounterStarting)
	{
		return;
	}

	ScoutRadius = FMath::Max(0.f, ScoutRadius);
	MinWaitTime = FMath::Max(0.f, MinWaitTime);
	MaxWaitTime = FMath::Max(MinWaitTime, MaxWaitTime);
	AcceptanceRadius = FMath::Max(1.f, AcceptanceRadius);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (MoveSpeed > 0.f)
		{
			MovementComponent->MaxWalkSpeed = MoveSpeed;
		}
	}

	StartScoutWait();
}

void AExplorationEnemyCharacter::UpdateScoutMovement(float DeltaSeconds)
{
	if (!bEnableScoutMovement || bEncounterStarting)
	{
		return;
	}

	if (ScoutWaitRemaining > 0.f)
	{
		ScoutWaitRemaining -= DeltaSeconds;
		if (ScoutWaitRemaining <= 0.f)
		{
			ChooseNextScoutTarget();
		}

		return;
	}

	if (!bScoutMoveInProgress)
	{
		ChooseNextScoutTarget();
		return;
	}

	if (AAIController* ScoutController = Cast<AAIController>(GetController()))
	{
		if (ScoutController->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			bScoutMoveInProgress = false;
			StartScoutWait();
			return;
		}
	}

	const float DistanceToTarget = FVector::Dist2D(GetActorLocation(), ScoutTargetLocation);
	if (DistanceToTarget <= AcceptanceRadius)
	{
		if (AAIController* ScoutController = Cast<AAIController>(GetController()))
		{
			ScoutController->StopMovement();
		}

		bScoutMoveInProgress = false;
		StartScoutWait();
	}
}

void AExplorationEnemyCharacter::StartScoutWait()
{
	const float WaitMax = FMath::Max(MinWaitTime, MaxWaitTime);
	ScoutWaitRemaining = WaitMax > 0.f
		? FMath::FRandRange(MinWaitTime, WaitMax)
		: 0.f;
	bScoutMoveInProgress = false;
}

void AExplorationEnemyCharacter::ChooseNextScoutTarget()
{
	FVector CandidateLocation = FVector::ZeroVector;
	if (!FindReachableScoutLocation(CandidateLocation))
	{
		StartScoutWait();
		return;
	}

	if (!TryMoveToScoutTarget(CandidateLocation))
	{
		StartScoutWait();
	}
}

bool AExplorationEnemyCharacter::TryMoveToScoutTarget(const FVector& TargetLocation)
{
	AAIController* ScoutController = GetOrCreateScoutController();
	if (!ScoutController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' could not start scout movement because it has no AI controller."), *GetName());
		return false;
	}

	ScoutTargetLocation = TargetLocation;

	const EPathFollowingRequestResult::Type MoveResult = ScoutController->MoveToLocation(
		ScoutTargetLocation,
		AcceptanceRadius,
		true,
		true,
		true,
		false);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ExplorationEnemyCharacter '%s' failed to move to scout target."), *GetName());
		return false;
	}

	bScoutMoveInProgress = true;
	return true;
}

bool AExplorationEnemyCharacter::FindReachableScoutLocation(FVector& OutLocation) const
{
	if (ScoutRadius <= 0.f)
	{
		OutLocation = ScoutOriginLocation;
		return true;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem)
	{
		return false;
	}

	FNavLocation NavLocation;
	const bool bFoundLocation = NavigationSystem->GetRandomReachablePointInRadius(
		ScoutOriginLocation,
		ScoutRadius,
		NavLocation);

	if (!bFoundLocation)
	{
		return false;
	}

	OutLocation = NavLocation.Location;
	return true;
}

AAIController* AExplorationEnemyCharacter::GetOrCreateScoutController()
{
	AAIController* ScoutController = Cast<AAIController>(GetController());
	if (ScoutController)
	{
		return ScoutController;
	}

	SpawnDefaultController();
	return Cast<AAIController>(GetController());
}
