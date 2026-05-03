// BattleUnit.cpp

#include "BattleUnit.h"

#include "Combat/JargonCombatGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/Grid/GridBoard.h"
#include "Combat/Grid/GridTile.h"
#include "Components/WidgetComponent.h"
#include "Combat/Widgets/BattleUnitStatusWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/JargonRelicDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABattleUnit::ABattleUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

	static ConstructorHelpers::FClassFinder<UBattleUnitStatusWidget> DefaultStatusWidgetClassFinder(TEXT("/Game/Jargon/Blueprints/Widgets/WBP_BattleUnitStatus"));
	if (DefaultStatusWidgetClassFinder.Succeeded())
	{
		StatusWidgetClass = DefaultStatusWidgetClassFinder.Class;
		StatusWidgetComponent->SetWidgetClass(StatusWidgetClass);
	}
}

void ABattleUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsMovingAlongPath || ActiveMovePath.Num() < 2 || ActiveMoveSegmentIndex < 0)
	{
		return;
	}

	ActiveMoveSegmentElapsed += DeltaSeconds;

	const float SegmentDuration = FMath::Max(KINDA_SMALL_NUMBER, ActiveMoveSegmentDuration);
	const float LerpAlpha = FMath::Clamp(ActiveMoveSegmentElapsed / SegmentDuration, 0.f, 1.f);
	const FVector NewLocation = FMath::Lerp(ActiveMoveSegmentStart, ActiveMoveSegmentEnd, LerpAlpha);
	SetActorLocation(NewLocation);

	if (LerpAlpha >= 1.f)
	{
		AGridTile* ReachedTile = ActiveMovePath.IsValidIndex(ActiveMoveSegmentIndex)
			? ActiveMovePath[ActiveMoveSegmentIndex].Get()
			: nullptr;
		const bool bIsFinalTile = ActiveMoveSegmentIndex >= ActiveMovePath.Num() - 1;
		HandlePathSegmentArrival(ReachedTile, bIsFinalTile);
	}
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
	PlayIdleAnimation();
}

void ABattleUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPathMovement();

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

	if (!StatusWidgetClass)
	{
		StatusWidgetClass = LoadClass<UBattleUnitStatusWidget>(nullptr, TEXT("/Game/Jargon/Blueprints/Widgets/WBP_BattleUnitStatus.WBP_BattleUnitStatus_C"));
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

	BP_OnStatusChanged();
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
	bMovementBlockedByRootThisTurn = false;
	RefreshStatusWidget();
}

bool ABattleUnit::ConsumeMoveAction()
{
	if (bIsDead || bMoveActionUsedThisTurn || IsRooted() || bMovementBlockedByRootThisTurn)
	{
		return false;
	}

	bMoveActionUsedThisTurn = true;
	RefreshStatusWidget();
	return true;
}

bool ABattleUnit::ConsumeAttackAction()
{
	if (bIsDead || bAttackActionUsedThisTurn)
	{
		return false;
	}

	bAttackActionUsedThisTurn = true;
	RefreshStatusWidget();
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

bool ABattleUnit::MoveAlongPath(const TArray<AGridTile*>& Path)
{
	if (bIsDead || IsRooted() || bMovementBlockedByRootThisTurn || bIsMovingAlongPath || Path.Num() < 2)
	{
		return false;
	}

	if (CurrentTile && Path[0] != CurrentTile)
	{
		return false;
	}

	AGridTile* DestinationTile = Path.Last();
	if (!DestinationTile || !DestinationTile->IsWalkable())
	{
		return false;
	}

	ActiveMovePath.Reset();
	ActiveMovePath.Reserve(Path.Num());

	for (AGridTile* PathTile : Path)
	{
		if (!PathTile)
		{
			StopPathMovement();
			return false;
		}

		ActiveMovePath.Add(PathTile);
	}

	ClearCurrentTileOccupancy();
	CurrentTile = nullptr;
	bIsMovingAlongPath = true;
	ActiveMoveSegmentIndex = 1;
	ActiveMoveSegmentElapsed = 0.f;
	ActiveMoveSegmentDuration = 0.f;

	SetActorTickEnabled(true);
	AdvanceMovementSegment();
	return true;
}

void ABattleUnit::ApplyDamage(int32 Amount)
{
	ApplyDamageInternal(Amount, nullptr);
}

void ABattleUnit::ApplyDamageFromSource(int32 Amount, ABattleUnit* DamageSourceUnit)
{
	FJargonCombatCueEvent DamageCueSource;
	if (DamageSourceUnit)
	{
		DamageCueSource.SourceObject = DamageSourceUnit;
		DamageCueSource.SourceUnit = DamageSourceUnit;
		DamageCueSource.SourceTile = DamageSourceUnit->GetCurrentTile();
	}

	ApplyDamageInternal(Amount, DamageSourceUnit ? &DamageCueSource : nullptr);
}

void ABattleUnit::ApplyDamageFromEffectContext(int32 Amount, const FJargonEffectContext& EffectContext)
{
	FJargonCombatCueEvent DamageCueSource;
	DamageCueSource.Operation = EJargonEffectOperation::DealDamage;
	DamageCueSource.Trigger = EffectContext.Trigger;
	DamageCueSource.SourceObject = EffectContext.SourceObject;
	DamageCueSource.SourceUnit = EffectContext.SourceUnit;
	DamageCueSource.SourceTile = EffectContext.SourceTile;
	DamageCueSource.OwningTileEffect = EffectContext.OwningTileEffect;
	DamageCueSource.SourceCard = EffectContext.SourceCard;
	DamageCueSource.SourceRelic = Cast<UJargonRelicDefinition>(EffectContext.SourceObject.Get());

	ApplyDamageInternal(Amount, &DamageCueSource);
}

void ABattleUnit::ApplyDamageInternal(int32 Amount, const FJargonCombatCueEvent* DamageCueSource)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	int32 RemainingDamage = Amount;
	if (VulnerableDamageBonus > 0)
	{
		const int32 ConsumedVulnerableBonus = VulnerableDamageBonus;
		VulnerableDamageBonus = 0;
		RemainingDamage += ConsumedVulnerableBonus;
		RefreshStatusWidget();
		BP_OnVulnerableChanged(VulnerableDamageBonus);
		EmitUnitCue(EJargonCombatCueType::VulnerableConsumed, ConsumedVulnerableBonus, CurrentTile);
	}

	const int32 IncomingDamage = RemainingDamage;
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
			BP_OnShieldChanged(TemporaryShield);
			if (TemporaryShield <= 0)
			{
				EmitUnitCue(EJargonCombatCueType::ShieldBroken, IncomingDamage, CurrentTile);
			}
		}
		return;
	}

	PlayHitFlash();
	
	CurrentHP = FMath::Max(0, CurrentHP - RemainingDamage);
	RefreshStatusWidget();

	if (AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr)
	{
		CombatGameMode->RecordCombatDamageApplied(
			DamageCueSource ? DamageCueSource->SourceUnit.Get() : nullptr,
			this,
			RemainingDamage);
	}

	EmitDamageCue(RemainingDamage, CurrentTile, DamageCueSource);

	if (bShieldChanged)
	{
		BP_OnShieldChanged(TemporaryShield);
		if (TemporaryShield <= 0)
		{
			EmitUnitCue(EJargonCombatCueType::ShieldBroken, IncomingDamage - RemainingDamage, CurrentTile);
		}
	}

	if (CurrentHP > 0)
	{
		return;
	}

	bIsDead = true;
	AGridTile* DeathTile = CurrentTile;
	EmitUnitCue(EJargonCombatCueType::UnitDied, 0, DeathTile);
	StopPathMovement();
	ClearCurrentTileOccupancy();
	CurrentTile = nullptr;

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (CombatGameMode)
	{
		CombatGameMode->HandleUnitDied(this, DeathTile);
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

	const int32 OldHP = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0, MaxHP);
	const int32 ActualHeal = CurrentHP - OldHP;
	RefreshStatusWidget();
	if (ActualHeal > 0)
	{
		if (AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr)
		{
			CombatGameMode->RecordCombatHealingApplied(this, ActualHeal);
		}

		EmitUnitCue(EJargonCombatCueType::Heal, ActualHeal, CurrentTile);
	}
}

void ABattleUnit::IncreaseAttack(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	AttackDamage += Amount;
	RefreshStatusWidget();
}

void ABattleUnit::IncreaseMaxHealth(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	MaxHP += Amount;
	RefreshStatusWidget();
}

void ABattleUnit::SetBaseCombatStats(
	int32 NewMaxHP,
	int32 NewMoveRange,
	int32 NewAttackRange,
	int32 NewAttackDamage,
	bool bRestoreToFullHealth)
{
	MaxHP = FMath::Max(1, NewMaxHP);
	MoveRange = FMath::Max(0, NewMoveRange);
	AttackRange = FMath::Max(1, NewAttackRange);
	AttackDamage = FMath::Max(0, NewAttackDamage);

	if (bRestoreToFullHealth)
	{
		CurrentHP = MaxHP;
	}
	else
	{
		CurrentHP = FMath::Clamp(CurrentHP, 0, MaxHP);
	}

	RefreshStatusWidget();
}

void ABattleUnit::ApplySummonedUnitDefinition(UJargonSummonedUnitDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	AppliedSummonedUnitDefinition = Definition;
	Team = Definition->Team;
	SetBaseCombatStats(
		Definition->MaxHP,
		Definition->MoveRange,
		Definition->AttackRange,
		Definition->AttackDamage,
		true);

	IdleAnimation = Definition->IdleAnimationOverride;
	BasicAttackAnimation = Definition->BasicAttackAnimationOverride;
	DeathAnimation = Definition->DeathAnimationOverride;
	PlayIdleAnimation();

	OnSummonedEffects.Append(Definition->OnSummonedEffects);
	OnTurnStartEffects.Append(Definition->OnTurnStartEffects);
	OnDeathEffects.Append(Definition->OnDeathEffects);
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

	AGridBoard* GridBoard = CurrentTile->GetOwningGridBoard();
	return GridBoard && GridBoard->AreTilesWithinRange(CurrentTile, Target->GetCurrentTile(), AttackRange);
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

	Target->ApplyDamageFromSource(AttackDamage, this);
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

float ABattleUnit::GetBasicAttackPresentationDuration() const
{
	return BasicAttackAnimation ? FMath::Max(0.f, BasicAttackAnimation->GetPlayLength()) : 0.f;
}

void ABattleUnit::ReturnToIdleAfterBasicAttack()
{
	PlayIdleAnimation();
}

void ABattleUnit::PlayIdleAnimation()
{
	if (bIsDead || !UnitMesh || !IdleAnimation)
	{
		return;
	}

	UnitMesh->PlayAnimation(IdleAnimation, true);
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

void ABattleUnit::EmitUnitCue(EJargonCombatCueType CueType, int32 Value, AGridTile* CueTile)
{
	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	FJargonCombatCueEvent Cue;
	Cue.CueType = CueType;
	Cue.TargetUnit = this;
	Cue.SourceUnit = this;
	Cue.TargetTile = CueTile ? CueTile : CurrentTile.Get();
	Cue.SourceTile = CurrentTile;
	Cue.Value = Value;
	Cue.WorldLocation = GetActorLocation();
	Cue.bHasWorldLocation = true;

	CombatGameMode->EmitCombatCue(Cue);
}

void ABattleUnit::AdvanceMovementSegment()
{
	if (!bIsMovingAlongPath)
	{
		return;
	}

	if (!ActiveMovePath.IsValidIndex(ActiveMoveSegmentIndex))
	{
		FinishPathMovement();
		return;
	}

	AGridTile* NextTile = ActiveMovePath[ActiveMoveSegmentIndex].Get();
	if (!NextTile)
	{
		StopPathMovement();
		MovementCompletedDelegate.Broadcast(this);
		return;
	}

	ActiveMoveSegmentStart = GetActorLocation();
	ActiveMoveSegmentEnd = NextTile->GetUnitStandLocation();
	ActiveMoveSegmentElapsed = 0.f;
	ActiveMoveSegmentDuration = FMath::Max(0.01f, MovementSecondsPerTile);
	FaceLocation(ActiveMoveSegmentEnd);
}

void ABattleUnit::HandlePathSegmentArrival(AGridTile* ReachedTile, bool bIsFinalTile)
{
	if (!ReachedTile)
	{
		StopPathMovement();
		MovementCompletedDelegate.Broadcast(this);
		return;
	}

	EnterTileDuringPathMovement(ReachedTile, bIsFinalTile);

	if (bIsDead)
	{
		return;
	}

	if (!bIsMovingAlongPath)
	{
		MovementCompletedDelegate.Broadcast(this);
		return;
	}

	if (bIsFinalTile)
	{
		FinishPathMovement();
		return;
	}

	++ActiveMoveSegmentIndex;
	AdvanceMovementSegment();
}

void ABattleUnit::EnterTileDuringPathMovement(AGridTile* Tile, bool bKeepOccupancyAfterEntry)
{
	if (!Tile || bIsDead)
	{
		return;
	}

	if ((Tile->IsOccupied() && Tile->GetOccupyingUnit() != this) || Tile->IsBlocked())
	{
		StopPathMovement();
		return;
	}

	SetActorLocation(Tile->GetUnitStandLocation());
	ClearCurrentTileOccupancy();
	SetCurrentTile(Tile);

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (CombatGameMode)
	{
		CombatGameMode->NotifyTileEffectsUnitEntered(this, Tile);
	}

	if (bIsDead || !bIsMovingAlongPath)
	{
		return;
	}

	if (!bKeepOccupancyAfterEntry && CurrentTile == Tile)
	{
		ClearCurrentTileOccupancy();
		SetCurrentTile(nullptr);
	}
}

void ABattleUnit::FinishPathMovement()
{
	StopPathMovement();
	MovementCompletedDelegate.Broadcast(this);
}

void ABattleUnit::StopPathMovement()
{
	bIsMovingAlongPath = false;
	ActiveMovePath.Reset();
	ActiveMoveSegmentIndex = INDEX_NONE;
	ActiveMoveSegmentElapsed = 0.f;
	ActiveMoveSegmentDuration = 0.f;
	ActiveMoveSegmentStart = FVector::ZeroVector;
	ActiveMoveSegmentEnd = FVector::ZeroVector;
	SetActorTickEnabled(false);
}

void ABattleUnit::FinalizeDeathAndDestroy()
{
	Destroy();
}

void ABattleUnit::EmitDamageCue(int32 Value, AGridTile* CueTile, const FJargonCombatCueEvent* DamageCueSource)
{
	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	FJargonCombatCueEvent Cue;
	Cue.CueType = EJargonCombatCueType::Damage;
	Cue.Operation = DamageCueSource ? DamageCueSource->Operation : EJargonEffectOperation::None;
	Cue.Trigger = DamageCueSource ? DamageCueSource->Trigger : EJargonEffectTrigger::OnPlayed;
	Cue.SourceObject = DamageCueSource ? DamageCueSource->SourceObject : nullptr;
	Cue.SourceUnit = DamageCueSource ? DamageCueSource->SourceUnit : nullptr;
	Cue.SourceTile = DamageCueSource ? DamageCueSource->SourceTile : nullptr;
	Cue.OwningTileEffect = DamageCueSource ? DamageCueSource->OwningTileEffect : nullptr;
	Cue.SourceCard = DamageCueSource ? DamageCueSource->SourceCard : nullptr;
	Cue.SourceRelic = DamageCueSource ? DamageCueSource->SourceRelic : nullptr;
	Cue.TargetUnit = this;
	Cue.TargetTile = CueTile ? CueTile : CurrentTile.Get();
	Cue.Value = Value;
	Cue.WorldLocation = GetActorLocation();
	Cue.bHasWorldLocation = true;

	if (!Cue.SourceObject && !Cue.SourceUnit && !Cue.SourceTile)
	{
		Cue.SourceObject = this;
		Cue.SourceUnit = this;
		Cue.SourceTile = CurrentTile;
	}

	CombatGameMode->EmitCombatCue(Cue);
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
	BP_OnShieldChanged(TemporaryShield);

	if (AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr)
	{
		CombatGameMode->RecordCombatShieldGained(this, Amount);
	}

	EmitUnitCue(EJargonCombatCueType::ShieldGained, Amount, CurrentTile);
}

void ABattleUnit::ClearTemporaryShield()
{
	TemporaryShield = 0;
	RefreshStatusWidget();
	BP_OnShieldChanged(TemporaryShield);
}

void ABattleUnit::ApplyStun(int32 Turns)
{
	const int32 SafeTurns = FMath::Max(0, Turns);
	if (SafeTurns <= 0)
	{
		return;
	}

	// Use max instead of += so repeated 1-turn stuns refresh duration
	// without stacking into huge lockouts.
	StunTurnsRemaining = FMath::Max(StunTurnsRemaining, SafeTurns);

	// Optional, but useful if your status widget refreshes from unit state.
	RefreshStatusWidget();
	BP_OnStunChanged(StunTurnsRemaining);
	EmitUnitCue(EJargonCombatCueType::StunApplied, SafeTurns, CurrentTile);
}

bool ABattleUnit::ConsumeStunTurn()
{
	if (StunTurnsRemaining <= 0)
	{
		return false;
	}

	StunTurnsRemaining = FMath::Max(0, StunTurnsRemaining - 1);

	// Optional, but useful if your status widget refreshes from unit state.
	RefreshStatusWidget();
	BP_OnStunChanged(StunTurnsRemaining);
	EmitUnitCue(EJargonCombatCueType::StunConsumed, 1, CurrentTile);

	return true;
}

void ABattleUnit::ApplyFreeze(int32 Turns)
{
	const int32 SafeTurns = FMath::Max(0, Turns);
	if (SafeTurns <= 0)
	{
		return;
	}

	// Freeze is stasis-flavored: repeated applications refresh duration
	// without stacking into long lockouts.
	FreezeTurnsRemaining = FMath::Max(FreezeTurnsRemaining, SafeTurns);

	RefreshStatusWidget();
	BP_OnFreezeChanged(FreezeTurnsRemaining);
	EmitUnitCue(EJargonCombatCueType::FreezeApplied, SafeTurns, CurrentTile);
}

bool ABattleUnit::ConsumeFreezeTurn()
{
	if (FreezeTurnsRemaining <= 0)
	{
		return false;
	}

	FreezeTurnsRemaining = FMath::Max(0, FreezeTurnsRemaining - 1);

	RefreshStatusWidget();
	BP_OnFreezeChanged(FreezeTurnsRemaining);
	EmitUnitCue(EJargonCombatCueType::FreezeConsumed, 1, CurrentTile);

	return true;
}

void ABattleUnit::ApplyBurn(int32 Stacks)
{
	const int32 SafeStacks = FMath::Max(0, Stacks);
	if (SafeStacks <= 0 || bIsDead)
	{
		return;
	}

	BurnStacks += SafeStacks;
	RefreshStatusWidget();
	BP_OnBurnChanged(BurnStacks);
	EmitUnitCue(EJargonCombatCueType::BurnApplied, SafeStacks, CurrentTile);
}

bool ABattleUnit::ConsumeBurnTurn()
{
	if (BurnStacks <= 0 || bIsDead)
	{
		return false;
	}

	const int32 BurnDamage = BurnStacks;
	EmitUnitCue(EJargonCombatCueType::BurnTick, BurnDamage, CurrentTile);
	ApplyDamage(BurnDamage);

	if (!bIsDead)
	{
		BurnStacks = FMath::Max(0, BurnStacks - 1);
		RefreshStatusWidget();
		BP_OnBurnChanged(BurnStacks);
	}

	return true;
}

void ABattleUnit::ApplyRoot(int32 Turns)
{
	const int32 SafeTurns = FMath::Max(0, Turns);
	if (SafeTurns <= 0 || bIsDead)
	{
		return;
	}

	RootTurnsRemaining = FMath::Max(RootTurnsRemaining, SafeTurns);
	RefreshStatusWidget();
	BP_OnRootChanged(RootTurnsRemaining);
	EmitUnitCue(EJargonCombatCueType::RootApplied, SafeTurns, CurrentTile);
}

bool ABattleUnit::ConsumeRootTurn()
{
	if (RootTurnsRemaining <= 0 || bIsDead)
	{
		return false;
	}

	RootTurnsRemaining = FMath::Max(0, RootTurnsRemaining - 1);
	bMoveActionUsedThisTurn = true;
	bMovementBlockedByRootThisTurn = true;
	RefreshStatusWidget();
	BP_OnRootChanged(RootTurnsRemaining);
	EmitUnitCue(EJargonCombatCueType::RootConsumed, 1, CurrentTile);
	return true;
}

void ABattleUnit::ApplyVulnerable(int32 BonusDamage)
{
	const int32 SafeBonusDamage = FMath::Max(0, BonusDamage);
	if (SafeBonusDamage <= 0 || bIsDead)
	{
		return;
	}

	VulnerableDamageBonus += SafeBonusDamage;
	RefreshStatusWidget();
	BP_OnVulnerableChanged(VulnerableDamageBonus);
	EmitUnitCue(EJargonCombatCueType::VulnerableApplied, SafeBonusDamage, CurrentTile);
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
		SetHighlightColor(HighlightColor);
		SetHighlightEnabled(true);
	}
	else
	{
		SetHighlightEnabled(false);
	}
}
