#include "World/Interactions/JargonInteractableActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "GameFramework/Pawn.h"

AJargonInteractableActor::AJargonInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(150.f, 150.f, 120.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionBox->SetGenerateOverlapEvents(true);

	InteractionPromptText = FText::FromString(TEXT("Press E to Interact"));
	InteractionVerbText = FText::FromString(TEXT("Interact"));

	InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AJargonInteractableActor::HandleInteractionBoxBeginOverlap);
	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AJargonInteractableActor::HandleInteractionBoxEndOverlap);
}

void AJargonInteractableActor::Interact_Implementation(AJargonExplorationPlayerController* InteractingController)
{
}

void AJargonInteractableActor::NotifyInteractionPromptShown(AJargonExplorationPlayerController* InteractingController)
{
	BP_OnInteractionPromptShown(InteractingController);
}

void AJargonInteractableActor::NotifyInteractionPromptHidden(AJargonExplorationPlayerController* InteractingController)
{
	BP_OnInteractionPromptHidden(InteractingController);
}

void AJargonInteractableActor::HandleInteractionBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	AJargonExplorationPlayerController* InteractingController = ResolveInteractingController(OtherActor);
	if (!InteractingController)
	{
		return;
	}

	OverlappingControllers.AddUnique(InteractingController);
	bPlayerInRange = OverlappingControllers.Num() > 0;

	InteractingController->NotifyInteractableEnteredRange(this);
	BP_OnPlayerEnteredRange(InteractingController);
	HandlePlayerEnteredRange(InteractingController);
}

void AJargonInteractableActor::HandleInteractionBoxEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	AJargonExplorationPlayerController* InteractingController = ResolveInteractingController(OtherActor);
	if (!InteractingController)
	{
		return;
	}

	OverlappingControllers.Remove(InteractingController);
	OverlappingControllers.RemoveAll([](const TWeakObjectPtr<AJargonExplorationPlayerController>& Controller)
	{
		return !Controller.IsValid();
	});
	bPlayerInRange = OverlappingControllers.Num() > 0;

	InteractingController->NotifyInteractableExitedRange(this);
	BP_OnPlayerExitedRange(InteractingController);
	HandlePlayerExitedRange(InteractingController);
}

AJargonExplorationPlayerController* AJargonInteractableActor::ResolveInteractingController(AActor* OtherActor) const
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	return OtherPawn ? Cast<AJargonExplorationPlayerController>(OtherPawn->GetController()) : nullptr;
}

void AJargonInteractableActor::HandlePlayerEnteredRange(AJargonExplorationPlayerController* InteractingController)
{
}

void AJargonInteractableActor::HandlePlayerExitedRange(AJargonExplorationPlayerController* InteractingController)
{
}
