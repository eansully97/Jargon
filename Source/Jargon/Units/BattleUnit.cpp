// BattleUnit.cpp

#include "Units/BattleUnit.h"

#include "Combat/JargonCombatGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Grid/GridTile.h"
#include "Components/WidgetComponent.h"
#include "Widgets/BattleUnitStatusWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

ABattleUnit::ABattleUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UnitMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("UnitMesh"));
	UnitMesh->SetupAttachment(SceneRoot);
	UnitMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UnitMesh->SetCollisionObjectType(ECC_Pawn);
	UnitMesh->SetCollisionResponseToAllChannels(ECR_Block);

	StatusWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusWidgetComponent"));
	StatusWidgetComponent->SetupAttachment(SceneRoot);
	StatusWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusWidgetComponent->SetDrawAtDesiredSize(true);
	StatusWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
	StatusWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABattleUnit::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHP = MaxHP;
	TemporaryShield = 0;
	ResetTurnActions();
	
	InitializeStatusWidget();
	RefreshStatusWidget();

	InitializeDynamicMaterials();
	RefreshMaterialFeedback();
}

void ABattleUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ABattleUnit::InitializeStatusWidget()
{
	if (!StatusWidgetComponent)
	{
		return;
	}

	if (StatusWidgetClass)
	{
		StatusWidgetComponent->SetWidgetClass(StatusWidgetClass);
	}

	UUserWidget* UserWidget = StatusWidgetComponent->GetUserWidgetObject();
	UBattleUnitStatusWidget* StatusWidget = Cast<UBattleUnitStatusWidget>(UserWidget);
	if (StatusWidget)
	{
		StatusWidget->SetObservedUnit(this);
	}
}

void ABattleUnit::RefreshStatusWidget()
{
	if (!StatusWidgetComponent)
	{
		return;
	}

	UUserWidget* UserWidget = StatusWidgetComponent->GetUserWidgetObject();
	UBattleUnitStatusWidget* StatusWidget = Cast<UBattleUnitStatusWidget>(UserWidget);
	if (StatusWidget)
	{
		StatusWidget->RefreshFromObservedUnit();
	}
}

void ABattleUnit::InitializeDynamicMaterials()
{
	DynamicMaterialInstances.Reset();

	if (!UnitMesh)
	{
		return;
	}

	const int32 MaterialCount = UnitMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* BaseMaterial = UnitMesh->GetMaterial(MaterialIndex);
		if (!BaseMaterial)
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = UnitMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
		if (DynamicMaterial)
		{
			DynamicMaterialInstances.Add(DynamicMaterial);
		}
	}
}

void ABattleUnit::RefreshMaterialFeedback()
{
	const float HighlightStrength = bHighlightEnabled ? HighlightStrengthWhenEnabled : 0.f;
	const float FlashStrength = bHitFlashActive ? HitFlashStrengthWhenActive : 0.f;

	for (UMaterialInstanceDynamic* DynamicMaterial : DynamicMaterialInstances)
	{
		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetScalarParameterValue(HighlightStrengthParameterName, HighlightStrength);
		DynamicMaterial->SetVectorParameterValue(HighlightColorParameterName, HighlightColor);

		DynamicMaterial->SetScalarParameterValue(FlashStrengthParameterName, FlashStrength);
		DynamicMaterial->SetVectorParameterValue(FlashColorParameterName, HitFlashColor);
	}
}

void ABattleUnit::SetFlashColor(const FLinearColor& InColor)
{
	HitFlashColor = InColor;
	RefreshMaterialFeedback();
}

void ABattleUnit::SetHighlightEnabled(bool bEnabled)
{
	if (bHighlightEnabled == bEnabled)
	{
		return;
	}

	bHighlightEnabled = bEnabled;
	RefreshMaterialFeedback();
}

void ABattleUnit::SetHighlightColor(const FLinearColor& InColor)
{
	HighlightColor = InColor;
	RefreshMaterialFeedback();
}

void ABattleUnit::PlayHitFlash()
{
	bHitFlashActive = true;
	RefreshMaterialFeedback();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
		World->GetTimerManager().SetTimer(
			HitFlashTimerHandle,
			this,
			&ABattleUnit::HandleHitFlashTimerElapsed,
			HitFlashDuration,
			false
		);
	}
}

void ABattleUnit::ClearHitFlash()
{
	if (!bHitFlashActive)
	{
		return;
	}

	bHitFlashActive = false;
	RefreshMaterialFeedback();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
	}
}

void ABattleUnit::HandleHitFlashTimerElapsed()
{
	ClearHitFlash();
}

void ABattleUnit::ResetTurnActions()
{
	bMoveActionUsedThisTurn = false;
	bAttackActionUsedThisTurn = false;
}

bool ABattleUnit::ConsumeMoveAction()
{
	if (bIsDead || bMoveActionUsedThisTurn)
	{
		return false;
	}

	bMoveActionUsedThisTurn = true;
	return true;
}

bool ABattleUnit::ConsumeAttackAction()
{
	if (bIsDead || bAttackActionUsedThisTurn)
	{
		return false;
	}

	bAttackActionUsedThisTurn = true;
	return true;
}

void ABattleUnit::PlaceOnTile(AGridTile* Tile)
{
	if (!Tile || bIsDead)
	{
		return;
	}

	if (CurrentTile == Tile)
	{
		SetActorLocation(Tile->GetUnitStandLocation());
		return;
	}

	if (Tile->IsOccupied() && Tile->GetOccupyingUnit() != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleUnit '%s' cannot be placed on occupied tile."), *GetName());
		return;
	}

	if (Tile->IsBlocked())
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleUnit '%s' cannot be placed on blocked tile."), *GetName());
		return;
	}

	ClearCurrentTileOccupancy();
	SetCurrentTile(Tile);
	SetActorLocation(Tile->GetUnitStandLocation());

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (CombatGameMode)
	{
		CombatGameMode->NotifyTileEffectsUnitEntered(this, Tile);
	}
}

void ABattleUnit::ClearCurrentTileOccupancy()
{
	if (CurrentTile && CurrentTile->GetOccupyingUnit() == this)
	{
		CurrentTile->SetOccupyingUnit(nullptr);
	}
}

void ABattleUnit::SetCurrentTile(AGridTile* Tile)
{
	CurrentTile = Tile;

	if (CurrentTile)
	{
		CurrentTile->SetOccupyingUnit(this);
	}
}

void ABattleUnit::MoveAlongPath(const TArray<AGridTile*>& Path)
{
	if (bIsDead || Path.Num() == 0)
	{
		return;
	}

	AGridTile* DestinationTile = Path.Last();
	if (!DestinationTile)
	{
		return;
	}

	FaceLocation(DestinationTile->GetUnitStandLocation());
	PlaceOnTile(DestinationTile);
}

void ABattleUnit::ApplyDamage(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	int32 RemainingDamage = Amount;
	bool bShieldChanged = false;

	if (TemporaryShield > 0)
	{
		const int32 Absorbed = FMath::Min(TemporaryShield, RemainingDamage);
		TemporaryShield -= Absorbed;
		RemainingDamage -= Absorbed;
		bShieldChanged = Absorbed > 0;
	}

	if (RemainingDamage <= 0)
	{
		if (bShieldChanged)
		{
			RefreshStatusWidget();
		}
		return;
	}

	PlayHitFlash();
	
	CurrentHP = FMath::Max(0, CurrentHP - RemainingDamage);
	RefreshStatusWidget();

	if (CurrentHP > 0)
	{
		return;
	}

	bIsDead = true;
	ClearCurrentTileOccupancy();
	CurrentTile = nullptr;

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (CombatGameMode)
	{
		CombatGameMode->HandleUnitDied(this);
	}

	PlayDeathPresentation();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
		World->GetTimerManager().ClearTimer(DeathTimerHandle);
		World->GetTimerManager().SetTimer(
			DeathTimerHandle,
			this,
			&ABattleUnit::HandleDeathTimerElapsed,
			DeathDestroyDelay,
			false
		);
	}
}

void ABattleUnit::ApplyHeal(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0, MaxHP);
	RefreshStatusWidget();
}

bool ABattleUnit::CanAttackTarget(const ABattleUnit* Target) const
{
	if (!Target || bIsDead || Target->IsDead())
	{
		return false;
	}

	if (Target == this)
	{
		return false;
	}

	if (Target->GetTeam() == Team)
	{
		return false;
	}

	if (!CurrentTile || !Target->GetCurrentTile())
	{
		return false;
	}

	const FIntPoint MyCoord = CurrentTile->GetCoord();
	const FIntPoint TargetCoord = Target->GetCurrentTile()->GetCoord();

	const int32 ManhattanDistance =
		FMath::Abs(MyCoord.X - TargetCoord.X) +
		FMath::Abs(MyCoord.Y - TargetCoord.Y);

	return ManhattanDistance <= AttackRange;
}

void ABattleUnit::FaceDirection(const FVector& WorldDirection)
{
	FVector FlatDirection = WorldDirection;
	FlatDirection.Z = 0.f;

	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	FRotator DesiredRotation = FlatDirection.Rotation();
	DesiredRotation.Pitch = 0.f;
	DesiredRotation.Roll = 0.f;

	SetActorRotation(DesiredRotation);
}

void ABattleUnit::FaceLocation(const FVector& WorldLocation)
{
	const FVector DirectionToLocation = WorldLocation - GetActorLocation();
	FaceDirection(DirectionToLocation);
}

bool ABattleUnit::PerformBasicAttack(ABattleUnit* Target)
{
	if (!CanAttackTarget(Target))
	{
		return false;
	}

	Target->ApplyDamage(AttackDamage);
	return true;
}

void ABattleUnit::PlayBasicAttackPresentation(ABattleUnit* Target)
{
	if (bIsDead)
	{
		return;
	}

	if (bFaceTargetOnBasicAttack && Target)
	{
		FaceLocation(Target->GetActorLocation());
	}

	if (!UnitMesh || !BasicAttackAnimation)
	{
		return;
	}

	// Clear any previous pending return-to-idle timer
	GetWorldTimerManager().ClearTimer(BasicAttackTimerHandle);

	UnitMesh->PlayAnimation(BasicAttackAnimation, false);

	const float AnimLength = BasicAttackAnimation->GetPlayLength();
	if (AnimLength > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			BasicAttackTimerHandle,
			this,
			&ABattleUnit::ReturnToIdleAfterBasicAttack,
			AnimLength,
			false
		);
	}
}

void ABattleUnit::ReturnToIdleAfterBasicAttack()
{
	if (bIsDead)
	{
		return;
	}

	if (UnitMesh && IdleAnimation)
	{
		UnitMesh->PlayAnimation(IdleAnimation, true);
	}
}

void ABattleUnit::PlayDeathPresentation()
{
	SetActorEnableCollision(false);

	ClearHitFlash();
	SetHighlightEnabled(false);

	if (UnitMesh)
	{
		UnitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (DeathAnimation)
		{
			UnitMesh->PlayAnimation(DeathAnimation, false);
		}
	}
	
	if (StatusWidgetComponent)
	{
		StatusWidgetComponent->SetVisibility(false);
	}
}

void ABattleUnit::FinalizeDeathAndDestroy()
{
	Destroy();
}

void ABattleUnit::HandleDeathTimerElapsed()
{
	FinalizeDeathAndDestroy();
}

void ABattleUnit::AddTemporaryShield(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	TemporaryShield += Amount;
	RefreshStatusWidget();
}

void ABattleUnit::ClearTemporaryShield()
{
	TemporaryShield = 0;
	RefreshStatusWidget();
}

void ABattleUnit::SetActingHighlight(bool bInActingHighlight)
{
	if (bActingHighlight == bInActingHighlight)
	{
		return;
	}

	bActingHighlight = bInActingHighlight;

	if (bActingHighlight)
	{
		SetHighlightColor(ActingHighlightColor);
		SetHighlightEnabled(true);
	}
	else
	{
		SetHighlightEnabled(false);
	}
}
