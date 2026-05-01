// Copyright Epic Games, Inc. All Rights Reserved.

#include "JargonCharacter.h"
#include "Data/JargonHeroDefinition.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

AJargonCharacter::AJargonCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;
	
	PrimaryActorTick.bCanEverTick = false;
}

void AJargonCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AJargonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AJargonCharacter::InitializeFromHeroDefinition(UJargonHeroDefinition* HeroDefinition)
{
	if (AppliedHeroDefinition == HeroDefinition)
	{
		return;
	}

	AppliedHeroDefinition = HeroDefinition;
	if (HeroDefinition && HeroDefinition->HeroSkeletalMesh && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(HeroDefinition->HeroSkeletalMesh);
	}
	BP_OnHeroDefinitionApplied(HeroDefinition);
}
