#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "World/Interactions/JargonInteractionTypes.h"
#include "JargonExplorationPlayerController.generated.h"

class UNiagaraSystem;
class UInputAction;
class UInputMappingContext;
class UJargonInteractionPromptWidget;
class UPathFollowingComponent;
class USpringArmComponent;
class AJargonInteractableActor;
struct FInputKeyEventArgs;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionPromptChangedSignature, const FJargonInteractionPromptData&, PromptData);

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

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void InteractWithCurrentInteractable();

	UFUNCTION(BlueprintCallable, Category = "Interaction|Prompt", meta = (ToolTip = "Forces the current interactable to rebuild and rebroadcast its shared prompt data. Useful after UI/modal state or Blueprint interaction state changes."))
	void RefreshInteractionPrompt();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AJargonInteractableActor* GetCurrentInteractable() const
	{
		return CurrentInteractable.Get();
	}

	UFUNCTION(BlueprintPure, Category = "Interaction|Prompt")
	FJargonInteractionPromptData GetCurrentInteractionPromptData() const
	{
		return CurrentInteractionPromptData;
	}

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Prompt")
	FOnInteractionPromptChangedSignature OnInteractionPromptChanged;

protected:
	virtual void BeginPlay() override;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ToolTip = "When enabled, holding right click rotates the attached Town/Exploration camera."))
	bool bEnableRightClickHoldCameraRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0", ToolTip = "Yaw degrees applied per mouse X input unit while holding right click."))
	float CameraRotationSpeed = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera")
	bool bInvertCameraRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera|Edge Pan", meta = (ToolTip = "When enabled, moving the mouse near the viewport edge pans the attached camera target offset."))
	bool bEnableEdgeScreenCameraPan = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera|Edge Pan", meta = (ClampMin = "0.0", ToolTip = "Distance from the viewport edge in pixels that starts camera edge panning."))
	float EdgePanBorderSize = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera|Edge Pan", meta = (ClampMin = "0.0", ToolTip = "Camera edge-pan movement speed in world units per second."))
	float EdgePanCameraMoveSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera", meta = (ClampMin = "0.0"))
	float MaxCameraPanOffset = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera")
	bool bInvertCameraPanX = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration|Camera")
	bool bInvertCameraPanY = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Prompt", meta = (AdvancedDisplay, ToolTip = "When enabled, logs shared interaction prompt decisions such as active interactable changes, modal suppression, widget creation, and widget updates."))
	bool bLogInteractionPromptDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt", meta = (ToolTip = "Optional shared widget class used to display the active Town/Exploration interaction prompt. Leave empty if a Blueprint binds directly to OnInteractionPromptChanged."))
	TSubclassOf<UJargonInteractionPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt", meta = (AdvancedDisplay, ClampMin = "0", ToolTip = "Viewport Z order for the optional shared interaction prompt widget."))
	int32 InteractionPromptZOrder = 15;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt|Layout", meta = (ToolTip = "When true, the prompt widget is positioned near the interactable's world location. When false, it uses the fallback viewport position."))
	bool bPositionInteractionPromptAtWorldLocation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt|Layout", meta = (AdvancedDisplay, ToolTip = "World-space offset added before projecting the interactable prompt location. Useful for lifting the prompt above the actor."))
	FVector InteractionPromptWorldOffset = FVector(0.f, 0.f, 120.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt|Layout", meta = (AdvancedDisplay, ToolTip = "Screen-space pixel offset applied after world projection or fallback positioning."))
	FVector2D InteractionPromptScreenOffset = FVector2D(0.f, -16.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt|Layout", meta = (AdvancedDisplay, ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Fallback normalized viewport position used when there is no world prompt location or projection fails. X/Y are 0 to 1 across the viewport."))
	FVector2D InteractionPromptFallbackViewportPosition = FVector2D(0.5f, 0.75f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Prompt|Layout", meta = (AdvancedDisplay, ToolTip = "Viewport alignment for the prompt widget. 0.5,1 anchors the widget's bottom center to the computed prompt position."))
	FVector2D InteractionPromptViewportAlignment = FVector2D(0.5f, 1.f);

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/** Set to true if we're using touch input. */
	uint32 bIsTouch : 1;

	uint32 bHasCachedDestination : 1;
	uint32 bHasIssuedHoldMove : 1;
	uint32 bCameraPanning : 1;
	uint32 bCameraRotating : 1;
	uint32 bHasRotatedCameraDuringHold : 1;

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
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction|Prompt", meta = (AllowPrivateAccess = "true"))
	FJargonInteractionPromptData CurrentInteractionPromptData;

	UPROPERTY(Transient)
	TObjectPtr<UJargonInteractionPromptWidget> InteractionPromptWidget = nullptr;

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
	void ApplyCameraPanOffsetDelta(const FVector& OffsetDelta);
	void TickEdgeScreenCameraPan(float DeltaTime);
	void BeginCameraRotate();
	void EndCameraRotate();
	void HandleCameraRotate(float AxisValue);
	USpringArmComponent* GetControlledPawnSpringArm();
	void CacheCameraDefaultsIfNeeded(USpringArmComponent* SpringArm);
	void ApplyCameraPanOffset(USpringArmComponent* SpringArm);
	void TickHeldCursorFollow(float DeltaTime);

	void HandleInteractPressed();
	void RefreshCurrentInteractable();
	void SetCurrentInteractable(AJargonInteractableActor* NewInteractable);
	void RefreshInteractionPromptData();
	virtual bool ShouldShowInteractionPrompt() const;
	void CreateInteractionPromptWidget();
	void UpdateInteractionPromptWidget();
	void UpdateInteractionPromptWidgetPosition();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Prompt")
	void BP_OnInteractionPromptChanged(const FJargonInteractionPromptData& PromptData);
};
