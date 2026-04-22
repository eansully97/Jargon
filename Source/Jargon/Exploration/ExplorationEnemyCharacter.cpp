// ExplorationEnemyCharacter.cpp

#include "Exploration/ExplorationEnemyCharacter.h"

#include "Components/SphereComponent.h"
#include "Core/JargonGameInstance.h"
#include "Encounters/EncounterDefinition.h"
#include "Encounters/EncounterTypes.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

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
	bEnableSimplePacing = false;
	PatrolOffset = FVector(300.f, 0.f, 0.f);
	PatrolSpeed = 120.f;
	bMovingToOffset = true;
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

	if (EncounterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' has no EncounterId set."), *GetName());
	}

	if (!EncounterDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' has no EncounterDefinition assigned."), *GetName());
	}

	if (UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>())
	{
		if (GameInstance->IsEncounterCleared(EncounterId))
		{
			DisableEncounter();
			SetActorHiddenInGame(true);
		}
	}
}

void AExplorationEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSimplePacing(DeltaSeconds);
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

	if (!EncounterDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' cannot start encounter because EncounterDefinition is null."), *GetName());
		return;
	}

	if (!EncounterDefinition->IsValidDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplorationEnemyCharacter '%s' has an invalid EncounterDefinition."), *GetName());
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
	PendingEncounter.CombatMapName = EncounterDefinition->CombatMapName;

	for (const FEncounterEnemySpawn& SpawnEntry : EncounterDefinition->EnemySpawns)
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

void AExplorationEnemyCharacter::UpdateSimplePacing(float DeltaSeconds)
{
	if (!bEnableSimplePacing || bEncounterStarting)
	{
		return;
	}

	const FVector CurrentTarget = bMovingToOffset ? PatrolTargetLocation : PatrolStartLocation;
	const FVector CurrentLocation = GetActorLocation();

	const FVector ToTarget = CurrentTarget - CurrentLocation;
	const float DistanceToTarget = ToTarget.Size();

	if (DistanceToTarget <= 5.f)
	{
		bMovingToOffset = !bMovingToOffset;
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();
	const FVector NewLocation = CurrentLocation + (Direction * PatrolSpeed * DeltaSeconds);

	SetActorLocation(NewLocation);
}