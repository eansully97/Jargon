// ExplorationEnemyCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ExplorationEnemyCharacter.generated.h"

class USphereComponent;
class UEncounterDefinition;
class UJargonGameInstance;
class AAIController;

UCLASS()
class JARGON_API AExplorationEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AExplorationEnemyCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnAggroSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	bool IsValidTriggeringActor(AActor* OtherActor) const;
	void DisableEncounter();
	void StartEncounterForPlayer(AActor* TriggeringActor);
	void UpdateScoutMovement(float DeltaSeconds);
	void BeginScoutMovement();
	void StartScoutWait();
	void ChooseNextScoutTarget();
	bool TryMoveToScoutTarget(const FVector& TargetLocation);
	bool FindReachableScoutLocation(FVector& OutLocation) const;
	AAIController* GetOrCreateScoutController();
	UEncounterDefinition* ResolveEncounterDefinitionToStart() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> AggroSphere = nullptr;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	FName EncounterId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	TObjectPtr<UEncounterDefinition> EncounterDefinition = nullptr;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	TArray<TObjectPtr<UEncounterDefinition>> EncounterDefinitionPool;

	UPROPERTY(Transient)
	bool bEncounterStarting = false;

	UPROPERTY(EditAnywhere, Category = "Scouting")
	bool bEnableScoutMovement = false;

	UPROPERTY(EditAnywhere, Category = "Scouting", meta = (ClampMin = "0.0", EditCondition = "bEnableScoutMovement"))
	float ScoutRadius = 650.f;

	UPROPERTY(EditAnywhere, Category = "Scouting", meta = (ClampMin = "0.0", EditCondition = "bEnableScoutMovement"))
	float MinWaitTime = 1.f;

	UPROPERTY(EditAnywhere, Category = "Scouting", meta = (ClampMin = "0.0", EditCondition = "bEnableScoutMovement"))
	float MaxWaitTime = 3.f;

	UPROPERTY(EditAnywhere, Category = "Scouting", meta = (ClampMin = "1.0", EditCondition = "bEnableScoutMovement"))
	float AcceptanceRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Scouting", meta = (ClampMin = "0.0", EditCondition = "bEnableScoutMovement"))
	float MoveSpeed = 150.f;

	/** Legacy back-and-forth pacing toggle. Kept so older placed actors upgrade safely into scout movement. */
	UPROPERTY(EditAnywhere, Category = "Pacing")
	bool bEnableSimplePacing = false;

	/** Local-space offset from the spawn location to pace toward. */
	UPROPERTY(EditAnywhere, Category = "Pacing", meta = (EditCondition = "bEnableSimplePacing"))
	FVector PatrolOffset = FVector(300.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Pacing", meta = (ClampMin = "0.0", EditCondition = "bEnableSimplePacing"))
	float PatrolSpeed = 120.f;

	UPROPERTY(Transient)
	FVector PatrolStartLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector PatrolTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bMovingToOffset = true;

	UPROPERTY(Transient)
	FVector ScoutOriginLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector ScoutTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float ScoutWaitRemaining = 0.f;

	UPROPERTY(Transient)
	bool bScoutMoveInProgress = false;
};
