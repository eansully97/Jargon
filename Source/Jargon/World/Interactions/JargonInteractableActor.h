#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JargonInteractableActor.generated.h"

class AJargonExplorationPlayerController;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class JARGON_API AJargonInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	AJargonInteractableActor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AJargonExplorationPlayerController* InteractingController);
	virtual void Interact_Implementation(AJargonExplorationPlayerController* InteractingController);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetInteractionPromptText() const
	{
		return InteractionPromptText;
	}

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetInteractionVerbText() const
	{
		return InteractionVerbText;
	}

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsPlayerInRange() const
	{
		return bPlayerInRange;
	}

	void NotifyInteractionPromptShown(AJargonExplorationPlayerController* InteractingController);
	void NotifyInteractionPromptHidden(AJargonExplorationPlayerController* InteractingController);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UBoxComponent> InteractionBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionVerbText;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction")
	bool bPlayerInRange = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BP_OnPlayerEnteredRange(AJargonExplorationPlayerController* InteractingController);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BP_OnPlayerExitedRange(AJargonExplorationPlayerController* InteractingController);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BP_OnInteractionPromptShown(AJargonExplorationPlayerController* InteractingController);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BP_OnInteractionPromptHidden(AJargonExplorationPlayerController* InteractingController);

	virtual void HandlePlayerEnteredRange(AJargonExplorationPlayerController* InteractingController);
	virtual void HandlePlayerExitedRange(AJargonExplorationPlayerController* InteractingController);

private:
	UFUNCTION()
	void HandleInteractionBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleInteractionBoxEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	AJargonExplorationPlayerController* ResolveInteractingController(AActor* OtherActor) const;

	TArray<TWeakObjectPtr<AJargonExplorationPlayerController>> OverlappingControllers;
};
