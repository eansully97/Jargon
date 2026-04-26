#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JargonExplorationPlayerController.generated.h"

class UNiagaraSystem;
class UInputAction;
class UInputMappingContext;
class UPathFollowingComponent;
class USpringArmComponent;
class AJargonInteractableActor;
struct FInputKeyEventArgs;

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

	UFUNCTION(BlueprintCallable, Category = "Exploration|Input")
	void ResetClickMoveState();

	UFUNCTION(BlueprintPure, Category = "Exploration|Input")
	bool IsWorldClickMovementEnabled() const
	{
		return bWorldClickMovementEnabled;
	}

	void NotifyInteractableEnteredRange(AJargonInteractableActor* Interactable);
	void NotifyInteractableExitedRange(AJargonInteractableActor* Interactable);

	UFUNCTION(BlueprintCallable, Category = "Exploration|Interaction")
	void InteractWithCurrentInteractable();

	UFUNCTION(BlueprintPure, Category = "Exploration|Interaction")
	AJargonInteractableActor* GetCurrentInteractable() const
	{
		return CurrentInteractable.Get();
	}

protected:
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	virtual void PlayerTick(float DeltaTime) override;

	virtual bool ShouldHandleWorldClickMovement() const;
	bool ShouldHandleAttachedCameraInput() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exploration|Input")
	bool bWorldClickMovementEnabled = true;

	/** Component used for moving along a NavMesh path. */
	UPROPERTY(VisibleDefaultsOnly, Category = "AI")
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

	/** Time threshold to know if the press was short enough to count as a click. */
	UPROPERTY(EditAnywhere, Category = "Input")
	float ShortPressThreshold;

	/** How often a held click updates the navigation destination. */
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.01"))
	float HoldMoveUpdateInterval = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bLogExplorationMovementDebug = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera")
	bool bEnableAttachedCameraControls = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0"))
	float MinCameraArmLength = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0"))
	float MaxCameraArmLength = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0"))
	float CameraZoomStep = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0"))
	float CameraPanSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0"))
	float MaxCameraPanOffset = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera")
	bool bInvertCameraPanX = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera")
	bool bInvertCameraPanY = false;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/** Set to true if we're using touch input. */
	uint32 bIsTouch : 1;

	uint32 bHasCachedDestination : 1;
	uint32 bHasIssuedHoldMove : 1;
	uint32 bCameraPanning : 1;

	/** Saved location of the character movement destination. */
	FVector CachedDestination;

	/** Time that the click input has been pressed. */
	float FollowTime = 0.0f;
	float FollowMoveUpdateTime = 0.0f;

	TWeakObjectPtr<USpringArmComponent> CachedCameraSpringArm;
	FVector BaseCameraTargetOffset = FVector::ZeroVector;
	FVector CameraPanOffset = FVector::ZeroVector;

	TArray<TWeakObjectPtr<AJargonInteractableActor>> NearbyInteractables;
	TWeakObjectPtr<AJargonInteractableActor> CurrentInteractable;

	void OnInputStarted();
	void OnTouchStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();
	bool TryUpdateCachedDestination();
	bool ProjectCachedDestinationToNavigation();
	bool MoveToCachedDestination(bool bSpawnCursorFX);
	void SpawnCursorFXAtCachedDestination() const;

	void HandleCameraZoom(float AxisValue);
	void BeginCameraPan();
	void EndCameraPan();
	void HandleCameraPanX(float AxisValue);
	void HandleCameraPanY(float AxisValue);
	bool HandleRawCameraInputKey(const FInputKeyEventArgs& Params);
	void ApplyCameraPanInput(float AxisX, float AxisY);
	USpringArmComponent* GetControlledPawnSpringArm();
	void CacheCameraDefaultsIfNeeded(USpringArmComponent* SpringArm);
	void ApplyCameraPanOffset(USpringArmComponent* SpringArm);
	void TickHeldCursorFollow(float DeltaTime);

	void HandleInteractPressed();
	void RefreshCurrentInteractable();
	void SetCurrentInteractable(AJargonInteractableActor* NewInteractable);
};
