#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JargonExplorationPlayerController.generated.h"

class UNiagaraSystem;
class UInputAction;
class UInputMappingContext;
class UPathFollowingComponent;

/**
 * Exploration-facing player controller that preserves the current click-to-move
 * prototype controls without relying on the template BP controller as the owner.
 */
UCLASS()
class JARGON_API AJargonExplorationPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJargonExplorationPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Exploration|Input")
	void SetWorldClickMovementEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Exploration|Input")
	bool IsWorldClickMovementEnabled() const
	{
		return bWorldClickMovementEnabled;
	}


protected:
	virtual void SetupInputComponent() override;

	virtual bool ShouldHandleWorldClickMovement() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exploration|Input")
	bool bWorldClickMovementEnabled = true;

	/** Component used for moving along a NavMesh path. */
	UPROPERTY(VisibleDefaultsOnly, Category = "AI")
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

	/** Time threshold to know if the press was short enough to count as a click. */
	UPROPERTY(EditAnywhere, Category = "Input")
	float ShortPressThreshold;

	/** FX to spawn when confirming a move destination. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UNiagaraSystem> FXCursor;

	/** Mapping context that drives click-to-move input. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SetDestinationClickAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SetDestinationTouchAction;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/** Set to true if we're using touch input. */
	uint32 bIsTouch : 1;

	/** Saved location of the character movement destination. */
	FVector CachedDestination;

	/** Time that the click input has been pressed. */
	float FollowTime = 0.0f;

	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();
	void UpdateCachedDestination();
};
