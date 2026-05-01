#include "Exploration/JargonExplorationPlayerController.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Jargon.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Interactions/JargonInteractableActor.h"
#include "World/Interactions/JargonInteractionPromptWidget.h"

namespace
{
template <typename TAssetType>
TAssetType* LoadOptionalAsset(const TCHAR* AssetPath)
{
	ConstructorHelpers::FObjectFinder<TAssetType> AssetFinder(AssetPath);
	return AssetFinder.Succeeded() ? AssetFinder.Object.Get() : nullptr;
}
}

AJargonExplorationPlayerController::AJargonExplorationPlayerController()
{
	bIsTouch = false;
	bMoveToMouseCursor = false;
	bHasCachedDestination = false;
	bHasIssuedHoldMove = false;
	bCameraPanning = false;
	bCameraRotating = false;
	bHasRotatedCameraDuringHold = false;

	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("Path Following Component"));

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
	FollowMoveUpdateTime = 0.f;
	ShortPressThreshold = 0.2f;

	DefaultMappingContext = LoadOptionalAsset<UInputMappingContext>(TEXT("/Game/TopDown/Input/IMC_Default.IMC_Default"));
	SetDestinationClickAction = LoadOptionalAsset<UInputAction>(TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Click.IA_SetDestination_Click"));
	SetDestinationTouchAction = LoadOptionalAsset<UInputAction>(TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Touch.IA_SetDestination_Touch"));
	FXCursor = LoadOptionalAsset<UNiagaraSystem>(TEXT("/Game/TopDown/Cursor/FX_Cursor_Success.FX_Cursor_Success"));
}

void AJargonExplorationPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CreateInteractionPromptWidget();
	UpdateInteractionPromptWidget();
}

void AJargonExplorationPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	TickHeldCursorFollow(DeltaTime);
	TickEdgeScreenCameraPan(DeltaTime);

	if (CurrentInteractionPromptData.bShouldShowPrompt)
	{
		UpdateInteractionPromptWidgetPosition();
	}
}

void AJargonExplorationPlayerController::TickHeldCursorFollow(float DeltaTime)
{
	if (!bMoveToMouseCursor)
	{
		return;
	}

	if (!ShouldHandleWorldClickMovement())
	{
		ResetClickMoveState();
		return;
	}

	if (!IsInputKeyDown(EKeys::LeftMouseButton))
	{
		return;
	}

	FollowTime += DeltaTime;
	FollowMoveUpdateTime += DeltaTime;

	if (FollowTime <= ShortPressThreshold)
	{
		return;
	}

	if (FollowMoveUpdateTime < HoldMoveUpdateInterval)
	{
		return;
	}

	FollowMoveUpdateTime = 0.f;

	if (TryUpdateCachedDestination())
	{
		MoveToCachedDestination(false);
		bHasIssuedHoldMove = true;
	}
}

void AJargonExplorationPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SetDestinationClickAction)
		{
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &AJargonExplorationPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &AJargonExplorationPlayerController::OnSetDestinationTriggered);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &AJargonExplorationPlayerController::OnSetDestinationReleased);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &AJargonExplorationPlayerController::OnSetDestinationReleased);
		}

		if (SetDestinationTouchAction)
		{
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &AJargonExplorationPlayerController::OnTouchStarted);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &AJargonExplorationPlayerController::OnTouchTriggered);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &AJargonExplorationPlayerController::OnTouchReleased);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &AJargonExplorationPlayerController::OnTouchReleased);
		}

	}
	else
	{
		UE_LOG(LogJargon, Error, TEXT("'%s' failed to find an Enhanced Input Component for exploration input."), *GetNameSafe(this));
	}

	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AJargonExplorationPlayerController::HandleInteractPressed);
	}
}

bool AJargonExplorationPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	const bool bHandledCameraInput = HandleRawCameraInputKey(Params);
	const bool bHandledBySuper = Super::InputKey(Params);
	return bHandledCameraInput || bHandledBySuper;
}


void AJargonExplorationPlayerController::OnInputStarted()
{
	if (!ShouldHandleWorldClickMovement())
	{
		ResetClickMoveState();
		return;
	}

	ResetClickMoveState();

	bIsTouch = false;
	bMoveToMouseCursor = true;

	// Prime this so once the hold threshold is crossed, the first follow update can happen immediately.
	FollowMoveUpdateTime = HoldMoveUpdateInterval;

	if (TryUpdateCachedDestination())
	{
		StopMovement();
		MoveToCachedDestination(false);
	}
}

void AJargonExplorationPlayerController::OnTouchStarted()
{
	if (!ShouldHandleWorldClickMovement())
	{
		ResetClickMoveState();
		return;
	}

	ResetClickMoveState();
	bIsTouch = true;
	FollowMoveUpdateTime = HoldMoveUpdateInterval;

	if (TryUpdateCachedDestination())
	{
		StopMovement();
		MoveToCachedDestination(false);
	}
}

void AJargonExplorationPlayerController::SetWorldClickMovementEnabled(bool bEnabled)
{
	if (bWorldClickMovementEnabled == bEnabled)
	{
		return;
	}

	bWorldClickMovementEnabled = bEnabled;
	ResetClickMoveState();
	EndCameraPan();
	FlushPressedKeys();

	if (!bWorldClickMovementEnabled)
	{
		StopMovement();
	}

	RefreshInteractionPromptData();
}

void AJargonExplorationPlayerController::NotifyInteractableEnteredRange(AJargonInteractableActor* Interactable)
{
	if (!IsValid(Interactable))
	{
		return;
	}

	NearbyInteractables.RemoveAll([](const TWeakObjectPtr<AJargonInteractableActor>& Candidate)
	{
		return !Candidate.IsValid();
	});

	NearbyInteractables.Remove(Interactable);
	NearbyInteractables.Add(Interactable);
	SetCurrentInteractable(Interactable);
}

void AJargonExplorationPlayerController::NotifyInteractableExitedRange(AJargonInteractableActor* Interactable)
{
	if (!Interactable)
	{
		return;
	}

	NearbyInteractables.Remove(Interactable);

	if (CurrentInteractable.Get() == Interactable)
	{
		SetCurrentInteractable(nullptr);
		RefreshCurrentInteractable();
	}
	else
	{
		RefreshInteractionPromptData();
	}
}

void AJargonExplorationPlayerController::InteractWithCurrentInteractable()
{
	if (!ShouldHandleWorldClickMovement())
	{
		UE_CLOG(
			bLogInteractionPromptDebug,
			LogJargon,
			Log,
			TEXT("Interaction ignored for '%s' because world interaction input is disabled."),
			*GetNameSafe(this)
		);
		return;
	}

	if (!CurrentInteractable.IsValid())
	{
		RefreshCurrentInteractable();
	}

	AJargonInteractableActor* Interactable = CurrentInteractable.Get();
	if (!IsValid(Interactable))
	{
		return;
	}

	Interactable->Interact(this);
}

void AJargonExplorationPlayerController::RefreshInteractionPrompt()
{
	if (!CurrentInteractable.IsValid())
	{
		RefreshCurrentInteractable();
		return;
	}

	RefreshInteractionPromptData();
}

void AJargonExplorationPlayerController::ResetClickMoveState()
{
	FollowTime = 0.f;
	FollowMoveUpdateTime = 0.f;
	bIsTouch = false;
	bMoveToMouseCursor = false;
	bHasCachedDestination = false;
	bHasIssuedHoldMove = false;
	bCameraRotating = false;
	bHasRotatedCameraDuringHold = false;
	CachedDestination = FVector::ZeroVector;
}

bool AJargonExplorationPlayerController::ShouldHandleWorldClickMovement() const
{
	return bWorldClickMovementEnabled;
}

bool AJargonExplorationPlayerController::ShouldHandleAttachedCameraInput() const
{
	return bEnableAttachedCameraControls && ShouldHandleWorldClickMovement();
}

void AJargonExplorationPlayerController::OnSetDestinationTriggered()
{
	// Hold-follow is handled by PlayerTick so it does not depend on Enhanced Input
	// repeatedly firing Triggered events while the mouse is held.
}

void AJargonExplorationPlayerController::OnSetDestinationReleased()
{
	if (!ShouldHandleWorldClickMovement())
	{
		ResetClickMoveState();
		return;
	}

	if (FollowTime <= ShortPressThreshold && !bHasRotatedCameraDuringHold)
	{
		if (!bHasCachedDestination)
		{
			TryUpdateCachedDestination();
		}

		MoveToCachedDestination(true);
	}

	ResetClickMoveState();
}

void AJargonExplorationPlayerController::OnTouchTriggered()
{
	if (!ShouldHandleWorldClickMovement())
	{
		ResetClickMoveState();
		return;
	}

	bIsTouch = true;
	OnSetDestinationTriggered();
}

void AJargonExplorationPlayerController::OnTouchReleased()
{
	if (!ShouldHandleWorldClickMovement())
	{
		ResetClickMoveState();
		return;
	}

	OnSetDestinationReleased();
}

bool AJargonExplorationPlayerController::TryUpdateCachedDestination()
{
	if (!ShouldHandleWorldClickMovement())
	{
		return false;
	}

	FHitResult Hit;
	bool bHitSuccessful = false;

	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
		bHasCachedDestination = true;
	}
	else
	{
		UE_CLOG(
			bLogExplorationMovementDebug,
			LogJargon,
			Log,
			TEXT("Click movement hit test failed. Touch=%s Gate=%s"),
			bIsTouch ? TEXT("true") : TEXT("false"),
			ShouldHandleWorldClickMovement() ? TEXT("true") : TEXT("false")
		);
	}

	return bHitSuccessful;
}

bool AJargonExplorationPlayerController::ProjectCachedDestinationToNavigation()
{
	if (!bHasCachedDestination)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogJargon, Warning, TEXT("Click movement failed because controller world is unavailable."));
		return false;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSystem)
	{
		UE_LOG(LogJargon, Warning, TEXT("Click movement failed because no navigation system is available."));
		return false;
	}

	FNavLocation ProjectedDestination;
	const bool bProjected = NavSystem->ProjectPointToNavigation(
		CachedDestination,
		ProjectedDestination,
		FVector(150.f, 150.f, 500.f)
	);

	if (!bProjected)
	{
		UE_CLOG(
			bLogExplorationMovementDebug,
			LogJargon,
			Log,
			TEXT("Click movement destination could not project to navmesh: %s"),
			*CachedDestination.ToCompactString()
		);
		return false;
	}

	CachedDestination = ProjectedDestination.Location;
	return true;
}

bool AJargonExplorationPlayerController::MoveToCachedDestination(bool bSpawnCursorFX)
{
	if (!ShouldHandleWorldClickMovement() || !bHasCachedDestination)
	{
		return false;
	}

	if (!GetPawn())
	{
		UE_LOG(LogJargon, Warning, TEXT("Click movement failed because '%s' has no controlled pawn."), *GetNameSafe(this));
		return false;
	}

	if (!ProjectCachedDestinationToNavigation())
	{
		return false;
	}

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);

	UE_CLOG(
		bLogExplorationMovementDebug,
		LogJargon,
		Log,
		TEXT("Click movement issued SimpleMoveToLocation: %s"),
		*CachedDestination.ToCompactString()
	);

	if (bSpawnCursorFX)
	{
		SpawnCursorFXAtCachedDestination();
	}

	return true;
}

void AJargonExplorationPlayerController::SpawnCursorFXAtCachedDestination() const
{
	if (!FXCursor || !bHasCachedDestination)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		FXCursor,
		CachedDestination,
		FRotator::ZeroRotator,
		FVector(1.f, 1.f, 1.f),
		true,
		true,
		ENCPoolMethod::None,
		true
	);
}

void AJargonExplorationPlayerController::HandleCameraZoom(float AxisValue)
{
	if (!ShouldHandleAttachedCameraInput() || FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	USpringArmComponent* SpringArm = GetControlledPawnSpringArm();
	if (!SpringArm)
	{
		return;
	}

	CacheCameraDefaultsIfNeeded(SpringArm);

	const float MinArmLength = FMath::Min(MinCameraArmLength, MaxCameraArmLength);
	const float MaxArmLength = FMath::Max(MinCameraArmLength, MaxCameraArmLength);
	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - (AxisValue * CameraZoomStep),
		MinArmLength,
		MaxArmLength
	);
}

void AJargonExplorationPlayerController::BeginCameraPan()
{
	if (!ShouldHandleAttachedCameraInput())
	{
		bCameraPanning = false;
		return;
	}

	bCameraPanning = GetControlledPawnSpringArm() != nullptr;
}

void AJargonExplorationPlayerController::EndCameraPan()
{
	bCameraPanning = false;
}

void AJargonExplorationPlayerController::BeginCameraRotate()
{
	if (!ShouldHandleAttachedCameraInput() || !GetControlledPawnSpringArm())
	{
		bCameraRotating = false;
		return;
	}

	bCameraRotating = true;
}

void AJargonExplorationPlayerController::EndCameraRotate()
{
	bCameraRotating = false;
}

void AJargonExplorationPlayerController::HandleCameraRotate(float AxisValue)
{
	if (!bCameraRotating || !ShouldHandleAttachedCameraInput() || FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	USpringArmComponent* SpringArm = GetControlledPawnSpringArm();
	if (!SpringArm)
	{
		EndCameraRotate();
		return;
	}

	CacheCameraDefaultsIfNeeded(SpringArm);

	const float SignedAxisValue = bInvertCameraRotation ? -AxisValue : AxisValue;
	FRotator NewRotation = SpringArm->GetComponentRotation();
	NewRotation.Yaw += SignedAxisValue * CameraRotationSpeed;
	SpringArm->SetWorldRotation(NewRotation);
	bHasRotatedCameraDuringHold = true;
}

void AJargonExplorationPlayerController::HandleCameraPanX(float AxisValue)
{
	ApplyCameraPanInput(AxisValue*.3f, 0.f);
}

void AJargonExplorationPlayerController::HandleCameraPanY(float AxisValue)
{
	ApplyCameraPanInput(0.f, -AxisValue*.3f);
}

bool AJargonExplorationPlayerController::HandleRawCameraInputKey(const FInputKeyEventArgs& Params)
{
	if (!bEnableAttachedCameraControls)
	{
		return false;
	}

	if (Params.Key == EKeys::RightMouseButton)
	{
		if (Params.Event == IE_Pressed)
		{
			if (bEnableRightClickHoldCameraRotation)
			{
				BeginCameraRotate();
			}
			return ShouldHandleAttachedCameraInput();
		}

		if (Params.Event == IE_Released)
		{
			EndCameraRotate();
			return true;
		}
	}

	if (!ShouldHandleAttachedCameraInput())
	{
		return false;
	}

	if (Params.Key == EKeys::MouseWheelAxis && Params.Event == IE_Axis && !FMath::IsNearlyZero(Params.AmountDepressed))
	{
		HandleCameraZoom(Params.AmountDepressed);
		return true;
	}

	if (Params.Key == EKeys::MouseScrollUp && Params.Event == IE_Pressed)
	{
		HandleCameraZoom(1.f);
		return true;
	}

	if (Params.Key == EKeys::MouseScrollDown && Params.Event == IE_Pressed)
	{
		HandleCameraZoom(-1.f);
		return true;
	}

	if (bCameraPanning && Params.Event == IE_Axis)
	{
		if (Params.Key == EKeys::MouseX)
		{
			HandleCameraPanX(Params.AmountDepressed);
			return true;
		}

		if (Params.Key == EKeys::MouseY)
		{
			HandleCameraPanY(Params.AmountDepressed);
			return true;
		}
	}

	if (bCameraRotating && Params.Event == IE_Axis && Params.Key == EKeys::MouseX)
	{
		HandleCameraRotate(Params.AmountDepressed);
		return true;
	}

	return false;
}

void AJargonExplorationPlayerController::ApplyCameraPanInput(float AxisX, float AxisY)
{
	if (!bCameraPanning || !ShouldHandleAttachedCameraInput())
	{
		return;
	}

	if (FMath::IsNearlyZero(AxisX) && FMath::IsNearlyZero(AxisY))
	{
		return;
	}

	USpringArmComponent* SpringArm = GetControlledPawnSpringArm();
	if (!SpringArm)
	{
		EndCameraPan();
		return;
	}

	CacheCameraDefaultsIfNeeded(SpringArm);

	const FRotator CameraYawRotation(0.f, SpringArm->GetComponentRotation().Yaw, 0.f);
	FVector ForwardVector = FRotationMatrix(CameraYawRotation).GetUnitAxis(EAxis::X);
	FVector RightVector = FRotationMatrix(CameraYawRotation).GetUnitAxis(EAxis::Y);

	const float SignedAxisX = bInvertCameraPanX ? -AxisX : AxisX;
	const float SignedAxisY = bInvertCameraPanY ? AxisY : -AxisY;

	ApplyCameraPanOffsetDelta(((RightVector * SignedAxisX) + (ForwardVector * SignedAxisY)) * CameraPanSpeed);
}

void AJargonExplorationPlayerController::ApplyCameraPanOffsetDelta(const FVector& OffsetDelta)
{
	if (!ShouldHandleAttachedCameraInput() || OffsetDelta.IsNearlyZero())
	{
		return;
	}

	USpringArmComponent* SpringArm = GetControlledPawnSpringArm();
	if (!SpringArm)
	{
		return;
	}

	CacheCameraDefaultsIfNeeded(SpringArm);

	CameraPanOffset += OffsetDelta;
	CameraPanOffset.Z = 0.f;

	const float PanDistance = CameraPanOffset.Size2D();
	if (PanDistance > MaxCameraPanOffset && PanDistance > KINDA_SMALL_NUMBER)
	{
		CameraPanOffset *= MaxCameraPanOffset / PanDistance;
	}

	ApplyCameraPanOffset(SpringArm);
}

void AJargonExplorationPlayerController::TickEdgeScreenCameraPan(float DeltaTime)
{
	if (!bEnableEdgeScreenCameraPan || !ShouldHandleAttachedCameraInput() || EdgePanCameraMoveSpeed <= 0.f || EdgePanBorderSize <= 0.f)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	float AxisX = 0.f;
	float AxisY = 0.f;
	if (MouseX <= EdgePanBorderSize)
	{
		AxisX = -1.f;
	}
	else if (MouseX >= static_cast<float>(ViewportSizeX) - EdgePanBorderSize)
	{
		AxisX = 1.f;
	}

	if (MouseY <= EdgePanBorderSize)
	{
		AxisY = 1.f;
	}
	else if (MouseY >= static_cast<float>(ViewportSizeY) - EdgePanBorderSize)
	{
		AxisY = -1.f;
	}

	if (FMath::IsNearlyZero(AxisX) && FMath::IsNearlyZero(AxisY))
	{
		return;
	}

	USpringArmComponent* SpringArm = GetControlledPawnSpringArm();
	if (!SpringArm)
	{
		return;
	}

	const FRotator CameraYawRotation(0.f, SpringArm->GetComponentRotation().Yaw, 0.f);
	FVector ForwardVector = FRotationMatrix(CameraYawRotation).GetUnitAxis(EAxis::X);
	FVector RightVector = FRotationMatrix(CameraYawRotation).GetUnitAxis(EAxis::Y);

	const float SignedAxisX = bInvertCameraPanX ? -AxisX : AxisX;
	const float SignedAxisY = bInvertCameraPanY ? -AxisY : AxisY;
	FVector PanDirection = (RightVector * SignedAxisX) + (ForwardVector * SignedAxisY);
	PanDirection.Z = 0.f;
	if (!PanDirection.Normalize())
	{
		return;
	}

	ApplyCameraPanOffsetDelta(PanDirection * EdgePanCameraMoveSpeed * DeltaTime);
}

USpringArmComponent* AJargonExplorationPlayerController::GetControlledPawnSpringArm()
{
	APawn* ControlledPawn = GetPawn();
	return ControlledPawn ? ControlledPawn->FindComponentByClass<USpringArmComponent>() : nullptr;
}

void AJargonExplorationPlayerController::CacheCameraDefaultsIfNeeded(USpringArmComponent* SpringArm)
{
	if (!SpringArm || CachedCameraSpringArm.Get() == SpringArm)
	{
		return;
	}

	CachedCameraSpringArm = SpringArm;
	BaseCameraTargetOffset = SpringArm->TargetOffset;
	CameraPanOffset = FVector::ZeroVector;
}

void AJargonExplorationPlayerController::ApplyCameraPanOffset(USpringArmComponent* SpringArm)
{
	if (!SpringArm)
	{
		return;
	}

	SpringArm->TargetOffset = BaseCameraTargetOffset + CameraPanOffset;
}

void AJargonExplorationPlayerController::HandleInteractPressed()
{
	InteractWithCurrentInteractable();
}

void AJargonExplorationPlayerController::RefreshCurrentInteractable()
{
	NearbyInteractables.RemoveAll([](const TWeakObjectPtr<AJargonInteractableActor>& Candidate)
	{
		return !Candidate.IsValid();
	});

	AJargonInteractableActor* NewInteractable = NearbyInteractables.Num() > 0
		? NearbyInteractables.Last().Get()
		: nullptr;

	SetCurrentInteractable(NewInteractable);
}

void AJargonExplorationPlayerController::SetCurrentInteractable(AJargonInteractableActor* NewInteractable)
{
	AJargonInteractableActor* PreviousInteractable = CurrentInteractable.Get();
	if (PreviousInteractable == NewInteractable)
	{
		RefreshInteractionPromptData();
		return;
	}

	UE_CLOG(
		bLogInteractionPromptDebug,
		LogJargon,
		Log,
		TEXT("Interaction prompt active interactable changed on '%s': %s -> %s"),
		*GetNameSafe(this),
		*GetNameSafe(PreviousInteractable),
		*GetNameSafe(NewInteractable)
	);

	if (IsValid(PreviousInteractable))
	{
		PreviousInteractable->NotifyInteractionPromptHidden(this);
	}

	CurrentInteractable = NewInteractable;

	if (IsValid(NewInteractable))
	{
		NewInteractable->NotifyInteractionPromptShown(this);
	}

	RefreshInteractionPromptData();
}

void AJargonExplorationPlayerController::RefreshInteractionPromptData()
{
	FJargonInteractionPromptData NewPromptData;

	AJargonInteractableActor* Interactable = CurrentInteractable.Get();
	if (ShouldShowInteractionPrompt() && IsValid(Interactable))
	{
		NewPromptData = Interactable->BuildInteractionPromptData(this);
		NewPromptData.Interactable = Interactable;
		NewPromptData.bShouldShowPrompt = NewPromptData.bShouldShowPrompt && NewPromptData.bCanInteract;
	}
	else
	{
		UE_CLOG(
			bLogInteractionPromptDebug && IsValid(Interactable),
			LogJargon,
			Log,
			TEXT("Interaction prompt suppressed for '%s'. Interactable=%s WorldInputEnabled=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Interactable),
			bWorldClickMovementEnabled ? TEXT("true") : TEXT("false")
		);
		NewPromptData.Reset();
	}

	CurrentInteractionPromptData = NewPromptData;
	OnInteractionPromptChanged.Broadcast(CurrentInteractionPromptData);
	BP_OnInteractionPromptChanged(CurrentInteractionPromptData);
	UpdateInteractionPromptWidget();
}

bool AJargonExplorationPlayerController::ShouldShowInteractionPrompt() const
{
	return ShouldHandleWorldClickMovement();
}

void AJargonExplorationPlayerController::CreateInteractionPromptWidget()
{
	if (!IsLocalPlayerController() || InteractionPromptWidget)
	{
		return;
	}

	if (!InteractionPromptWidgetClass)
	{
		UE_CLOG(
			bLogInteractionPromptDebug,
			LogJargon,
			Log,
			TEXT("No interaction prompt widget class assigned on '%s'. OnInteractionPromptChanged remains available for Blueprint UI."),
			*GetNameSafe(this)
		);
		return;
	}

	InteractionPromptWidget = CreateWidget<UJargonInteractionPromptWidget>(this, InteractionPromptWidgetClass);
	if (!InteractionPromptWidget)
	{
		UE_LOG(LogJargon, Warning, TEXT("Failed to create interaction prompt widget for '%s'."), *GetNameSafe(this));
		return;
	}

	InteractionPromptWidget->AddToViewport(InteractionPromptZOrder);
	InteractionPromptWidget->SetAlignmentInViewport(InteractionPromptViewportAlignment);
	InteractionPromptWidget->ClearPromptData();
	UpdateInteractionPromptWidgetPosition();

	UE_CLOG(
		bLogInteractionPromptDebug,
		LogJargon,
		Log,
		TEXT("Created interaction prompt widget '%s' for '%s'."),
		*GetNameSafe(InteractionPromptWidget),
		*GetNameSafe(this)
	);
}

void AJargonExplorationPlayerController::UpdateInteractionPromptWidget()
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	if (CurrentInteractionPromptData.bShouldShowPrompt)
	{
		InteractionPromptWidget->ApplyPromptData(CurrentInteractionPromptData);
		UpdateInteractionPromptWidgetPosition();
		UE_CLOG(
			bLogInteractionPromptDebug,
			LogJargon,
			Log,
			TEXT("Applied interaction prompt widget data for '%s'. Interactable=%s Prompt='%s'"),
			*GetNameSafe(this),
			*GetNameSafe(CurrentInteractionPromptData.Interactable),
			*CurrentInteractionPromptData.PromptText.ToString()
		);
	}
	else
	{
		InteractionPromptWidget->ClearPromptData();
		UE_CLOG(
			bLogInteractionPromptDebug,
			LogJargon,
			Log,
			TEXT("Cleared interaction prompt widget for '%s'."),
			*GetNameSafe(this)
		);
	}
}

void AJargonExplorationPlayerController::UpdateInteractionPromptWidgetPosition()
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	FVector2D PromptScreenPosition(
		static_cast<float>(ViewportSizeX) * InteractionPromptFallbackViewportPosition.X,
		static_cast<float>(ViewportSizeY) * InteractionPromptFallbackViewportPosition.Y
	);

	bool bUsedWorldLocation = false;
	if (bPositionInteractionPromptAtWorldLocation && CurrentInteractionPromptData.bHasWorldLocation)
	{
		const FVector ProjectedWorldLocation = CurrentInteractionPromptData.WorldLocation + InteractionPromptWorldOffset;
		bUsedWorldLocation = ProjectWorldLocationToScreen(ProjectedWorldLocation, PromptScreenPosition, true);
	}

	PromptScreenPosition += InteractionPromptScreenOffset;
	InteractionPromptWidget->SetAlignmentInViewport(InteractionPromptViewportAlignment);
	InteractionPromptWidget->SetPositionInViewport(PromptScreenPosition, false);

	UE_CLOG(
		bLogInteractionPromptDebug && CurrentInteractionPromptData.bShouldShowPrompt,
		LogJargon,
		VeryVerbose,
		TEXT("Positioned interaction prompt for '%s'. UsedWorldLocation=%s ScreenPosition=%s"),
		*GetNameSafe(this),
		bUsedWorldLocation ? TEXT("true") : TEXT("false"),
		*PromptScreenPosition.ToString()
	);
}
