// EncounterTrigger.cpp

#include "Exploration/EncounterTrigger.h"

#include "Components/BoxComponent.h"
#include "Core/JargonGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AEncounterTrigger::AEncounterTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
	bIsEditorOnlyActor = false;
#endif
}

void AEncounterTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (ensure(TriggerBox))
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEncounterTrigger::OnTriggerBoxBeginOverlap);
	}

	if (EncounterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger '%s' has no EncounterId set."), *GetName());
	}

	if (CombatMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger '%s' has no CombatMapName set."), *GetName());
	}

	if (UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>())
	{
		if (GameInstance->IsEncounterCleared(EncounterId))
		{
			DisableTrigger();
			SetActorHiddenInGame(true);
		}
	}
}

void AEncounterTrigger::OnTriggerBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bEncounterStarting)
	{
		return;
	}

	if (!IsValidTriggeringActor(OtherActor))
	{
		return;
	}

	if (EncounterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger '%s' cannot start encounter because EncounterId is None."), *GetName());
		return;
	}

	if (CombatMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger '%s' cannot start encounter because CombatMapName is None."), *GetName());
		return;
	}

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterTrigger '%s' could not find UJargonGameInstance."), *GetName());
		return;
	}

	if (GameInstance->IsEncounterCleared(EncounterId))
	{
		DisableTrigger();
		SetActorHiddenInGame(true);
		return;
	}

	const APawn* TriggeringPawn = Cast<APawn>(OtherActor);
	if (!TriggeringPawn)
	{
		return;
	}

	bEncounterStarting = true;
	DisableTrigger();

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	const FTransform SourceTransform = TriggeringPawn->GetActorTransform();

	GameInstance->StartEncounter(EncounterId, FName(*CurrentLevelName), SourceTransform);

	UGameplayStatics::OpenLevel(this, CombatMapName);
}

bool AEncounterTrigger::IsValidTriggeringActor(AActor* OtherActor) const
{
	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return false;
	}

	return Pawn->IsPlayerControlled();
}

void AEncounterTrigger::DisableTrigger()
{
	if (!TriggerBox)
	{
		return;
	}

	TriggerBox->SetGenerateOverlapEvents(false);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

#if WITH_EDITOR
void AEncounterTrigger::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
}
#endif