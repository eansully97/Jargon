#include "Exploration/JargonExplorationPlayerController.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Jargon.h"
#include "Navigation/PathFollowingComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

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

	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("Path Following Component"));

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
	ShortPressThreshold = 0.2f;

	DefaultMappingContext = LoadOptionalAsset<UInputMappingContext>(TEXT("/Game/TopDown/Input/IMC_Default.IMC_Default"));
	SetDestinationClickAction = LoadOptionalAsset<UInputAction>(TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Click.IA_SetDestination_Click"));
	SetDestinationTouchAction = LoadOptionalAsset<UInputAction>(TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Touch.IA_SetDestination_Touch"));
	FXCursor = LoadOptionalAsset<UNiagaraSystem>(TEXT("/Game/TopDown/Cursor/FX_Cursor_Success.FX_Cursor_Success"));
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
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &AJargonExplorationPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &AJargonExplorationPlayerController::OnTouchTriggered);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &AJargonExplorationPlayerController::OnTouchReleased);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &AJargonExplorationPlayerController::OnTouchReleased);
		}
	}
	else
	{
		UE_LOG(LogJargon, Error, TEXT("'%s' failed to find an Enhanced Input Component for exploration input."), *GetNameSafe(this));
	}
}

void AJargonExplorationPlayerController::OnInputStarted()
{
	if (!ShouldHandleWorldClickMovement())
	{
		return;
	}

	StopMovement();
	UpdateCachedDestination();
}

void AJargonExplorationPlayerController::SetWorldClickMovementEnabled(bool bEnabled)
{
	bWorldClickMovementEnabled = bEnabled;

	if (!bWorldClickMovementEnabled)
	{
		StopMovement();
		FollowTime = 0.f;
	}
}

bool AJargonExplorationPlayerController::ShouldHandleWorldClickMovement() const
{
	return bWorldClickMovementEnabled;
}

void AJargonExplorationPlayerController::OnSetDestinationTriggered()
{
	if (!ShouldHandleWorldClickMovement())
	{
		return;
	}

	FollowTime += GetWorld()->GetDeltaSeconds();
	UpdateCachedDestination();

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0f, false);
	}
}

void AJargonExplorationPlayerController::OnSetDestinationReleased()
{
	if (!ShouldHandleWorldClickMovement())
	{
		FollowTime = 0.f;
		return;
	}

	if (FollowTime <= ShortPressThreshold)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);

		if (FXCursor)
		{
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
	}

	FollowTime = 0.f;
}

void AJargonExplorationPlayerController::OnTouchTriggered()
{
	if (!ShouldHandleWorldClickMovement())
	{
		return;
	}

	bIsTouch = true;
	OnSetDestinationTriggered();
}

void AJargonExplorationPlayerController::OnTouchReleased()
{
	if (!ShouldHandleWorldClickMovement())
	{
		bIsTouch = false;
		FollowTime = 0.f;
		return;
	}

	bIsTouch = false;
	OnSetDestinationReleased();
}

void AJargonExplorationPlayerController::UpdateCachedDestination()
{
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
	}
}
