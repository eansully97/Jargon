// EncounterTrigger.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterTrigger.generated.h"

class UBoxComponent;
class UJargonGameInstance;

UCLASS()
class JARGON_API AEncounterTrigger : public AActor
{
	GENERATED_BODY()

public:
	AEncounterTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Unique ID for this encounter in the exploration world. */
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FName EncounterId = NAME_None;

	/** Level name to open when this trigger is activated. */
	UPROPERTY(EditAnywhere, Category = "Encounter")
	FName CombatMapName = NAME_None;

	/** Prevents duplicate activation while travel is being started. */
	UPROPERTY(Transient)
	bool bEncounterStarting = false;

	UFUNCTION()
	void OnTriggerBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	bool IsValidTriggeringActor(AActor* OtherActor) const;
	void DisableTrigger();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};