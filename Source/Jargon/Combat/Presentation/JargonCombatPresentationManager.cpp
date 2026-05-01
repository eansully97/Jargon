#include "Combat/Presentation/JargonCombatPresentationManager.h"

#include "Blueprint/UserWidget.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Presentation/JargonCombatPresentationSettings.h"
#include "Combat/Presentation/JargonFloatingCombatTextWidget.h"
#include "Combat/Units/BattleUnit.h"
#include "Components/SceneComponent.h"
#include "Data/CardDefinition.h"
#include "Data/JargonRelicDefinition.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "DrawDebugHelpers.h"
#endif

namespace
{
bool TryGetObjectWorldLocation(const UObject* Object, FVector& OutLocation)
{
	if (const AActor* Actor = Cast<AActor>(Object))
	{
		OutLocation = Actor->GetActorLocation();
		return true;
	}

	if (const USceneComponent* Component = Cast<USceneComponent>(Object))
	{
		OutLocation = Component->GetComponentLocation();
		return true;
	}

	return false;
}

template <typename AssetType>
bool HasAnyAssignedCueAsset(const TMap<EJargonCombatCueType, TObjectPtr<AssetType>>& CueAssetMap)
{
	for (const TPair<EJargonCombatCueType, TObjectPtr<AssetType>>& Pair : CueAssetMap)
	{
		if (Pair.Value)
		{
			return true;
		}
	}

	return false;
}

bool IsUsableNiagaraParameterName(FName ParameterName)
{
	return !ParameterName.IsNone();
}
}

AJargonCombatPresentationManager::AJargonCombatPresentationManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AJargonCombatPresentationManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageFloatingTextAggregationTimerHandle);
	}

	PendingDamageFloatingTextByTarget.Reset();

	Super::EndPlay(EndPlayReason);
}

void AJargonCombatPresentationManager::InitializePresentation(
	UJargonCombatPresentationSettings* InSettings,
	AJargonCombatGameMode* InCombatGameMode)
{
	PresentationSettings = InSettings;
	CombatGameMode = InCombatGameMode;

	if (!PresentationSettings)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat presentation manager initialized without PresentationSettings. Blueprint cue hooks still fire, but configured VFX/SFX/floating text are disabled."));
		return;
	}

	if (PresentationSettings->bEnableDebugValidation)
	{
		ValidatePresentationSetup(true);
	}
}

void AJargonCombatPresentationManager::HandleCombatCue(const FJargonCombatCueEvent& Cue)
{
	FVector CueLocation = GetActorLocation();
	GetBestCueLocation(Cue, CueLocation);

	PlayConfiguredVFX(Cue, CueLocation);
	PlayConfiguredSFX(Cue, CueLocation);
	SpawnFloatingText(Cue, CueLocation);
	DrawDebugCue(Cue, CueLocation);
	DispatchBlueprintCueEvents(Cue);
}

bool AJargonCombatPresentationManager::ValidatePresentationSetup(bool bLogWarnings) const
{
	bool bValid = true;

	if (!PresentationSettings)
	{
		if (bLogWarnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat presentation setup has no PresentationSettings asset assigned."));
		}
		return false;
	}

	if (PresentationSettings->bEnableFloatingText && !PresentationSettings->FloatingTextWidgetClass)
	{
		bValid = false;
		if (bLogWarnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat presentation floating text is enabled, but FloatingTextWidgetClass is not assigned."));
		}
	}

	if (PresentationSettings->bEnableVFX && !HasAnyAssignedCueAsset(PresentationSettings->DefaultNiagaraByCue))
	{
		if (bLogWarnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat presentation VFX are enabled, but no Niagara systems are assigned in DefaultNiagaraByCue."));
		}
	}

	if (PresentationSettings->bEnableSFX && !HasAnyAssignedCueAsset(PresentationSettings->DefaultSoundByCue))
	{
		if (bLogWarnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat presentation SFX are enabled, but no sounds are assigned in DefaultSoundByCue."));
		}
	}

	return bValid;
}

bool AJargonCombatPresentationManager::GetBestCueLocation(const FJargonCombatCueEvent& Cue, FVector& OutLocation) const
{
	if (Cue.bHasWorldLocation)
	{
		OutLocation = Cue.WorldLocation;
		return true;
	}

	if (GetCueTargetLocation(Cue, OutLocation))
	{
		return true;
	}

	if (GetCueSourceLocation(Cue, OutLocation))
	{
		return true;
	}

	if (Cue.OwningTileEffect && TryGetObjectWorldLocation(Cue.OwningTileEffect, OutLocation))
	{
		return true;
	}

	OutLocation = GetActorLocation();
	return false;
}

bool AJargonCombatPresentationManager::GetCueSourceLocation(const FJargonCombatCueEvent& Cue, FVector& OutLocation) const
{
	if (Cue.SourceUnit)
	{
		OutLocation = Cue.SourceUnit->GetActorLocation();
		return true;
	}

	if (Cue.SourceTile)
	{
		OutLocation = Cue.SourceTile->GetActorLocation();
		return true;
	}

	return TryGetObjectWorldLocation(Cue.SourceObject.Get(), OutLocation);
}

bool AJargonCombatPresentationManager::GetCueTargetLocation(const FJargonCombatCueEvent& Cue, FVector& OutLocation) const
{
	if (Cue.TargetUnit)
	{
		OutLocation = Cue.TargetUnit->GetActorLocation();
		return true;
	}

	if (Cue.TargetTile)
	{
		OutLocation = Cue.TargetTile->GetActorLocation();
		return true;
	}

	return false;
}

FText AJargonCombatPresentationManager::GetCueDisplayText(const FJargonCombatCueEvent& Cue) const
{
	if (!Cue.TextOverride.IsEmpty())
	{
		return Cue.TextOverride;
	}

	switch (Cue.CueType)
	{
	case EJargonCombatCueType::CardPlayed:
		return Cue.SourceCard && !Cue.SourceCard->DisplayName.IsEmpty()
			? Cue.SourceCard->DisplayName
			: FText::FromString(TEXT("Card Played"));

	case EJargonCombatCueType::Damage:
	case EJargonCombatCueType::PushCollision:
		return FText::FromString(FString::Printf(TEXT("-%d"), FMath::Max(0, Cue.Value)));

	case EJargonCombatCueType::Heal:
		return FText::FromString(FString::Printf(TEXT("+%d"), FMath::Max(0, Cue.Value)));

	case EJargonCombatCueType::ShieldGained:
		return FText::FromString(FString::Printf(TEXT("+%d Shield"), FMath::Max(0, Cue.Value)));

	case EJargonCombatCueType::ShieldBroken:
		return FText::FromString(TEXT("Shield Break"));

	case EJargonCombatCueType::StunApplied:
		return FText::FromString(TEXT("Stunned"));

	case EJargonCombatCueType::FreezeApplied:
		return FText::FromString(TEXT("Frozen"));

	case EJargonCombatCueType::FreezeConsumed:
		return FText::FromString(TEXT("Thawing"));

	case EJargonCombatCueType::UnitSummoned:
		return FText::FromString(TEXT("Summoned"));

	case EJargonCombatCueType::RelicTriggered:
		if (Cue.SourceRelic && !Cue.SourceRelic->DisplayName.IsEmpty())
		{
			return FText::Format(FText::FromString(TEXT("{0} triggered")), Cue.SourceRelic->DisplayName);
		}
		return FText::FromString(TEXT("Relic triggered"));

	case EJargonCombatCueType::ClassPassiveTriggered:
		if (Cue.HeroClass != EJargonHeroClass::None)
		{
			const UEnum* HeroClassEnum = StaticEnum<EJargonHeroClass>();
			const FText HeroClassName = HeroClassEnum
				? HeroClassEnum->GetDisplayNameTextByValue(static_cast<int64>(Cue.HeroClass))
				: FText::FromString(TEXT("Hero"));
			return FText::Format(FText::FromString(TEXT("{0} Passive")), HeroClassName);
		}
		return FText::FromString(TEXT("Class Passive"));

	case EJargonCombatCueType::HeroAspectTriggered:
		if (Cue.HeroAspect != EJargonHeroAspect::None)
		{
			const UEnum* HeroAspectEnum = StaticEnum<EJargonHeroAspect>();
			return HeroAspectEnum
				? HeroAspectEnum->GetDisplayNameTextByValue(static_cast<int64>(Cue.HeroAspect))
				: FText::FromString(TEXT("Hero Aspect"));
		}
		return FText::FromString(TEXT("Hero Aspect"));

	case EJargonCombatCueType::HeroAspectActivated:
		if (Cue.HeroAspect != EJargonHeroAspect::None)
		{
			const UEnum* HeroAspectEnum = StaticEnum<EJargonHeroAspect>();
			const FText AspectName = HeroAspectEnum
				? HeroAspectEnum->GetDisplayNameTextByValue(static_cast<int64>(Cue.HeroAspect))
				: FText::FromString(TEXT("Hero Aspect"));
			return FText::Format(FText::FromString(TEXT("{0} Awakened")), AspectName);
		}
		return FText::FromString(TEXT("Hero Aspect Awakened"));

	case EJargonCombatCueType::ElementalBonusTriggered:
		if (Cue.SourceCard && !Cue.SourceCard->DisplayName.IsEmpty())
		{
			const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
			const FText ElementName = ElementEnum
				? ElementEnum->GetDisplayNameTextByValue(static_cast<int64>(Cue.ElementType))
				: FText::FromString(TEXT("Element"));
			return FText::Format(FText::FromString(TEXT("{0}: {1} Bonus")), Cue.SourceCard->DisplayName, ElementName);
		}
		return FText::FromString(TEXT("Elemental Bonus"));

	default:
		return FText::GetEmpty();
	}
}

FLinearColor AJargonCombatPresentationManager::GetCueDisplayColor(const FJargonCombatCueEvent& Cue) const
{
	if (Cue.bHasColorOverride)
	{
		return Cue.ColorOverride;
	}

	return PresentationSettings
		? PresentationSettings->GetColorForCue(Cue.CueType)
		: FLinearColor::White;
}

bool AJargonCombatPresentationManager::IsUnitCue(const FJargonCombatCueEvent& Cue) const
{
	switch (Cue.CueType)
	{
	case EJargonCombatCueType::Damage:
	case EJargonCombatCueType::Heal:
	case EJargonCombatCueType::ShieldGained:
	case EJargonCombatCueType::ShieldBroken:
	case EJargonCombatCueType::StunApplied:
	case EJargonCombatCueType::StunConsumed:
	case EJargonCombatCueType::FreezeApplied:
	case EJargonCombatCueType::FreezeConsumed:
	case EJargonCombatCueType::UnitSummoned:
	case EJargonCombatCueType::UnitDied:
	case EJargonCombatCueType::ChainJump:
	case EJargonCombatCueType::Push:
	case EJargonCombatCueType::PushCollision:
		return true;

	default:
		return Cue.SourceUnit != nullptr || Cue.TargetUnit != nullptr;
	}
}

bool AJargonCombatPresentationManager::IsTileCue(const FJargonCombatCueEvent& Cue) const
{
	switch (Cue.CueType)
	{
	case EJargonCombatCueType::AoEPulse:
	case EJargonCombatCueType::TileEffectPlaced:
	case EJargonCombatCueType::TileEffectTriggered:
	case EJargonCombatCueType::TileEffectExpired:
		return true;

	default:
		return Cue.SourceTile != nullptr || Cue.TargetTile != nullptr || Cue.OwningTileEffect != nullptr;
	}
}

bool AJargonCombatPresentationManager::IsRelicCue(const FJargonCombatCueEvent& Cue) const
{
	return Cue.CueType == EJargonCombatCueType::RelicTriggered || Cue.SourceRelic != nullptr;
}

int32 AJargonCombatPresentationManager::GetCueTypeId(const FJargonCombatCueEvent& Cue) const
{
	return static_cast<int32>(Cue.CueType);
}

float AJargonCombatPresentationManager::GetCueDurationScale(const FJargonCombatCueEvent& Cue) const
{
	(void)Cue;

	return PresentationSettings
		? FMath::Max(0.01f, PresentationSettings->DefaultDurationScale)
		: 1.f;
}

float AJargonCombatPresentationManager::GetCueRadius(const FJargonCombatCueEvent& Cue) const
{
	return static_cast<float>(FMath::Max(0, Cue.Radius));
}

float AJargonCombatPresentationManager::GetCueValueAsFloat(const FJargonCombatCueEvent& Cue) const
{
	return static_cast<float>(Cue.Value);
}

bool AJargonCombatPresentationManager::HasValidSourceLocation(const FJargonCombatCueEvent& Cue) const
{
	FVector UnusedLocation;
	return GetCueSourceLocation(Cue, UnusedLocation);
}

bool AJargonCombatPresentationManager::HasValidTargetLocation(const FJargonCombatCueEvent& Cue) const
{
	FVector UnusedLocation;
	return GetCueTargetLocation(Cue, UnusedLocation);
}

void AJargonCombatPresentationManager::PlayConfiguredVFX(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const
{
	if (!PresentationSettings || !PresentationSettings->bEnableVFX)
	{
		return;
	}

	const TObjectPtr<UNiagaraSystem>* NiagaraSystemPtr = PresentationSettings->DefaultNiagaraByCue.Find(Cue.CueType);
	UNiagaraSystem* NiagaraSystem = NiagaraSystemPtr ? NiagaraSystemPtr->Get() : nullptr;
	if (!NiagaraSystem)
	{
		return;
	}

	FRotator VFXRotation = FRotator::ZeroRotator;
	FVector VFXScale = FVector(1.f);

	if (PresentationSettings->bUseSimpleVFXTransformMode)
	{
		if (PresentationSettings->bOrientVFXToCueDirection)
		{
			FVector CueDirection = FVector::UpVector;
			float SourceToTargetDistance = 0.f;
			const bool bHasCueDirection = ResolveCueDirection(Cue, CueDirection, SourceToTargetDistance);
			if (bHasCueDirection)
			{
				if (PresentationSettings->bInvertVFXDirection)
				{
					CueDirection *= -1.f;
				}

				VFXRotation = CueDirection.Rotation();
			}
			else if (PresentationSettings->bEnableDebugValidation && Cue.CueType == EJargonCombatCueType::Damage)
			{
				FVector SourceLocation = FVector::ZeroVector;
				FVector TargetLocation = FVector::ZeroVector;
				const bool bHasSourceLocation = GetCueSourceLocation(Cue, SourceLocation);
				const bool bHasTargetLocation = GetCueTargetLocation(Cue, TargetLocation);
				UE_LOG(LogTemp, Warning, TEXT("Damage cue has no valid source-to-target direction for VFX orientation. SourceValid=%s TargetValid=%s SourceUnit=%s TargetUnit=%s SourceObject=%s"),
					bHasSourceLocation ? TEXT("true") : TEXT("false"),
					bHasTargetLocation ? TEXT("true") : TEXT("false"),
					*GetNameSafe(Cue.SourceUnit.Get()),
					*GetNameSafe(Cue.TargetUnit.Get()),
					*GetNameSafe(Cue.SourceObject.Get()));
			}
		}

		const float ComponentScale = ResolveSimpleVFXComponentScale(Cue);
		VFXScale = FVector(ComponentScale);
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraSystem,
		CueLocation,
		VFXRotation,
		VFXScale,
		true,
		false);

	ApplyCueParametersToNiagaraComponent(NiagaraComponent, Cue, CueLocation);

	if (NiagaraComponent)
	{
		NiagaraComponent->Activate(true);
	}
}

void AJargonCombatPresentationManager::ApplyCueParametersToNiagaraComponent(
	UNiagaraComponent* NiagaraComponent,
	const FJargonCombatCueEvent& Cue,
	const FVector& CueLocation) const
{
	if (!NiagaraComponent || !PresentationSettings || !PresentationSettings->bApplyNiagaraCueParameters)
	{
		return;
	}

	const FLinearColor CueColor = GetCueDisplayColor(Cue);
	const float CueValue = GetCueValueAsFloat(Cue);
	const float CueRadius = GetCueRadius(Cue);
	const float DurationScale = GetCueDurationScale(Cue);
	const int32 CueTypeId = GetCueTypeId(Cue);

	FVector SourceLocation = FVector::ZeroVector;
	const bool bHasSourceLocation = GetCueSourceLocation(Cue, SourceLocation);

	FVector TargetLocation = FVector::ZeroVector;
	const bool bHasTargetLocation = GetCueTargetLocation(Cue, TargetLocation);

	float SourceToTargetDistance = 0.f;
	FVector CueDirection = FVector::UpVector;
	const bool bHasCueDirection = ResolveCueDirection(Cue, CueDirection, SourceToTargetDistance);

	const float RawSpriteSize =
		PresentationSettings->BaseSpriteSize +
		FMath::Max(0.f, CueValue) * PresentationSettings->ValueSpriteSizeScale;
	const float MaxSpriteSize = FMath::Max(1.f, PresentationSettings->MaxSpriteSize);
	const float SpriteSize = FMath::Clamp(RawSpriteSize, 1.f, MaxSpriteSize);

	const float RawSpriteScale =
		PresentationSettings->BaseSpriteScale +
		FMath::Max(0.f, CueValue) * PresentationSettings->ValueSpriteScaleAmount +
		CueRadius * PresentationSettings->RadiusSpriteScaleAmount;
	const float MinSpriteScale = FMath::Max(0.01f, PresentationSettings->MinSpriteScale);
	const float MaxSpriteScale = FMath::Max(MinSpriteScale, PresentationSettings->MaxSpriteScale);
	const float CueSpriteScale = FMath::Clamp(RawSpriteScale, MinSpriteScale, MaxSpriteScale);

	if (IsUsableNiagaraParameterName(PresentationSettings->CueColorParameterName))
	{
		NiagaraComponent->SetVariableLinearColor(PresentationSettings->CueColorParameterName, CueColor);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->ValueParameterName))
	{
		NiagaraComponent->SetVariableFloat(PresentationSettings->ValueParameterName, CueValue);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->RadiusParameterName))
	{
		NiagaraComponent->SetVariableFloat(PresentationSettings->RadiusParameterName, CueRadius);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->CueLocationParameterName))
	{
		NiagaraComponent->SetVariablePosition(PresentationSettings->CueLocationParameterName, CueLocation);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->SourceLocationParameterName))
	{
		NiagaraComponent->SetVariablePosition(PresentationSettings->SourceLocationParameterName, SourceLocation);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->TargetLocationParameterName))
	{
		NiagaraComponent->SetVariablePosition(PresentationSettings->TargetLocationParameterName, TargetLocation);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->DurationScaleParameterName))
	{
		NiagaraComponent->SetVariableFloat(PresentationSettings->DurationScaleParameterName, DurationScale);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->CueTypeIdParameterName))
	{
		NiagaraComponent->SetVariableInt(PresentationSettings->CueTypeIdParameterName, CueTypeId);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->HasSourceLocationParameterName))
	{
		NiagaraComponent->SetVariableBool(PresentationSettings->HasSourceLocationParameterName, bHasSourceLocation);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->HasTargetLocationParameterName))
	{
		NiagaraComponent->SetVariableBool(PresentationSettings->HasTargetLocationParameterName, bHasTargetLocation);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->CueDirectionParameterName))
	{
		NiagaraComponent->SetVariableVec3(PresentationSettings->CueDirectionParameterName, CueDirection);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->HasCueDirectionParameterName))
	{
		NiagaraComponent->SetVariableBool(PresentationSettings->HasCueDirectionParameterName, bHasCueDirection);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->SourceToTargetDistanceParameterName))
	{
		NiagaraComponent->SetVariableFloat(PresentationSettings->SourceToTargetDistanceParameterName, SourceToTargetDistance);
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->SpriteSizeParameterName))
	{
		NiagaraComponent->SetVariableVec2(PresentationSettings->SpriteSizeParameterName, FVector2D(SpriteSize, SpriteSize));
	}

	if (IsUsableNiagaraParameterName(PresentationSettings->SpriteScaleParameterName))
	{
		NiagaraComponent->SetVariableFloat(PresentationSettings->SpriteScaleParameterName, CueSpriteScale);
	}
}

bool AJargonCombatPresentationManager::ResolveCueDirection(
	const FJargonCombatCueEvent& Cue,
	FVector& OutDirection,
	float& OutSourceToTargetDistance) const
{
	OutDirection = FVector::UpVector;
	OutSourceToTargetDistance = 0.f;

	FVector SourceLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	if (!GetCueSourceLocation(Cue, SourceLocation) || !GetCueTargetLocation(Cue, TargetLocation))
	{
		return false;
	}

	const FVector SourceToTarget = TargetLocation - SourceLocation;
	OutSourceToTargetDistance = SourceToTarget.Size();
	if (OutSourceToTargetDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutDirection = SourceToTarget / OutSourceToTargetDistance;
	return true;
}

float AJargonCombatPresentationManager::ResolveSimpleVFXComponentScale(const FJargonCombatCueEvent& Cue) const
{
	if (!PresentationSettings)
	{
		return 1.f;
	}

	float RawScale = PresentationSettings->BaseVFXComponentScale;
	if (PresentationSettings->bScaleVFXComponentByCueValue)
	{
		RawScale += FMath::Max(0.f, GetCueValueAsFloat(Cue)) * PresentationSettings->ValueVFXComponentScaleAmount;
		RawScale += GetCueRadius(Cue) * PresentationSettings->RadiusVFXComponentScaleAmount;
	}

	const float MinScale = FMath::Max(0.01f, PresentationSettings->MinVFXComponentScale);
	const float MaxScale = FMath::Max(MinScale, PresentationSettings->MaxVFXComponentScale);
	return FMath::Clamp(RawScale, MinScale, MaxScale);
}

void AJargonCombatPresentationManager::PlayConfiguredSFX(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const
{
	if (!PresentationSettings || !PresentationSettings->bEnableSFX)
	{
		return;
	}

	const TObjectPtr<USoundBase>* SoundPtr = PresentationSettings->DefaultSoundByCue.Find(Cue.CueType);
	USoundBase* Sound = SoundPtr ? SoundPtr->Get() : nullptr;
	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, CueLocation);
}

bool AJargonCombatPresentationManager::ShouldSpawnFloatingTextForCue(const FJargonCombatCueEvent& Cue) const
{
	if (!PresentationSettings)
	{
		return false;
	}

	if (PresentationSettings->bShowOnlyNumericFloatingText)
	{
		switch (Cue.CueType)
		{
		case EJargonCombatCueType::Damage:
		case EJargonCombatCueType::PushCollision:
		case EJargonCombatCueType::Heal:
		case EJargonCombatCueType::ShieldGained:
			return Cue.Value > 0;

		case EJargonCombatCueType::ClassPassiveTriggered:
		case EJargonCombatCueType::HeroAspectActivated:
		case EJargonCombatCueType::HeroAspectTriggered:
		case EJargonCombatCueType::ElementalBonusTriggered:
			return true;

		default:
			return !Cue.TextOverride.IsEmpty();
		}
	}

	return !GetCueDisplayText(Cue).IsEmpty();
}

void AJargonCombatPresentationManager::SpawnFloatingText(const FJargonCombatCueEvent& Cue, const FVector& CueLocation)
{
	if (!PresentationSettings ||
		!PresentationSettings->bEnableFloatingText ||
		!PresentationSettings->FloatingTextWidgetClass)
	{
		return;
	}

	if (!ShouldSpawnFloatingTextForCue(Cue))
	{
		return;
	}

	const bool bDamageCue =
		Cue.CueType == EJargonCombatCueType::Damage ||
		Cue.CueType == EJargonCombatCueType::PushCollision;
	if (bDamageCue &&
		PresentationSettings->bAggregateDamageFloatingText &&
		PresentationSettings->DamageFloatingTextAggregationWindow > 0.f)
	{
		QueueAggregatedDamageFloatingText(Cue, CueLocation);
		return;
	}

	SpawnFloatingTextImmediate(Cue, CueLocation);
}

void AJargonCombatPresentationManager::SpawnFloatingTextImmediate(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const
{
	const FText DisplayText = GetCueDisplayText(Cue);
	if (DisplayText.IsEmpty())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		if (PresentationSettings->bEnableDebugValidation)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat presentation could not spawn floating text because PlayerController 0 was not found."));
		}
		return;
	}

	FVector2D ScreenPosition;
	const FVector FloatingTextWorldLocation = CueLocation + PresentationSettings->FloatingTextWorldOffset;
	if (!PlayerController->ProjectWorldLocationToScreen(FloatingTextWorldLocation, ScreenPosition, true))
	{
		if (PresentationSettings->bEnableDebugValidation)
		{
			UE_LOG(LogTemp, Warning, TEXT("Combat presentation could not project floating text cue '%s' to screen."),
				*StaticEnum<EJargonCombatCueType>()->GetNameStringByValue(static_cast<int64>(Cue.CueType)));
		}
		return;
	}

	UJargonFloatingCombatTextWidget* FloatingTextWidget = CreateWidget<UJargonFloatingCombatTextWidget>(
		PlayerController,
		PresentationSettings->FloatingTextWidgetClass);
	if (!FloatingTextWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat presentation failed to create floating text widget from class '%s'."),
			*GetNameSafe(PresentationSettings->FloatingTextWidgetClass.Get()));
		return;
	}

	FJargonCombatCueEvent WidgetCue = Cue;
	WidgetCue.bHasWorldLocation = true;
	WidgetCue.WorldLocation = FloatingTextWorldLocation;
	WidgetCue.TextOverride = DisplayText;
	FloatingTextWidget->InitializeFromCue(WidgetCue, GetCueDisplayColor(Cue));
	FloatingTextWidget->AddToViewport(PresentationSettings->FloatingTextZOrder);
	FloatingTextWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	FloatingTextWidget->SetPositionInViewport(ScreenPosition + PresentationSettings->FloatingTextScreenOffset, true);
}

void AJargonCombatPresentationManager::QueueAggregatedDamageFloatingText(const FJargonCombatCueEvent& Cue, const FVector& CueLocation)
{
	if (!PresentationSettings || Cue.Value <= 0 || !Cue.TargetUnit)
	{
		SpawnFloatingTextImmediate(Cue, CueLocation);
		return;
	}

	FPendingFloatingDamageCue& PendingCue = PendingDamageFloatingTextByTarget.FindOrAdd(Cue.TargetUnit);
	if (PendingCue.TotalValue <= 0)
	{
		PendingCue.Cue = Cue;
		PendingCue.Cue.CueType = EJargonCombatCueType::Damage;
		PendingCue.Cue.TextOverride = FText::GetEmpty();
		PendingCue.CueLocation = CueLocation;
		PendingCue.TotalValue = 0;
	}

	PendingCue.TotalValue += FMath::Max(0, Cue.Value);
	PendingCue.Cue.Value = PendingCue.TotalValue;
	PendingCue.Cue.TargetUnit = Cue.TargetUnit;
	PendingCue.Cue.TargetTile = Cue.TargetTile;
	PendingCue.Cue.WorldLocation = Cue.WorldLocation;
	PendingCue.Cue.bHasWorldLocation = Cue.bHasWorldLocation;
	PendingCue.CueLocation = CueLocation;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageFloatingTextAggregationTimerHandle);
		World->GetTimerManager().SetTimer(
			DamageFloatingTextAggregationTimerHandle,
			this,
			&AJargonCombatPresentationManager::FlushAggregatedDamageFloatingText,
			PresentationSettings->DamageFloatingTextAggregationWindow,
			false);
	}
}

void AJargonCombatPresentationManager::FlushAggregatedDamageFloatingText()
{
	TMap<TWeakObjectPtr<ABattleUnit>, FPendingFloatingDamageCue> PendingCues;
	Swap(PendingCues, PendingDamageFloatingTextByTarget);

	for (const TPair<TWeakObjectPtr<ABattleUnit>, FPendingFloatingDamageCue>& Pair : PendingCues)
	{
		if (Pair.Value.TotalValue <= 0)
		{
			continue;
		}

		FVector CueLocation = Pair.Value.CueLocation;
		GetBestCueLocation(Pair.Value.Cue, CueLocation);
		SpawnFloatingTextImmediate(Pair.Value.Cue, CueLocation);
	}
}

void AJargonCombatPresentationManager::DrawDebugCue(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!PresentationSettings || !PresentationSettings->bEnableDebugCueDraw)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FColor DebugColor = GetCueDisplayColor(Cue).ToFColor(true);
	const float Duration = FMath::Max(0.01f, PresentationSettings->DebugCueDuration);
	const float Scale = FMath::Max(1.f, PresentationSettings->DebugCueScale);

	DrawDebugSphere(World, CueLocation, Scale, 12, DebugColor, false, Duration, 0, 2.f);

	if (Cue.CueType == EJargonCombatCueType::ChainJump)
	{
		FVector SourceLocation;
		FVector TargetLocation;
		if (GetCueSourceLocation(Cue, SourceLocation) && GetCueTargetLocation(Cue, TargetLocation))
		{
			DrawDebugLine(World, SourceLocation, TargetLocation, DebugColor, false, Duration, 0, 3.f);
		}
	}
	else if (Cue.CueType == EJargonCombatCueType::AoEPulse && Cue.Radius > 0)
	{
		DrawDebugCircle(
			World,
			CueLocation,
			Scale * Cue.Radius,
			32,
			DebugColor,
			false,
			Duration,
			0,
			2.f,
			FVector::ForwardVector,
			FVector::RightVector,
			false);
	}
#endif
}

void AJargonCombatPresentationManager::DispatchBlueprintCueEvents(const FJargonCombatCueEvent& Cue)
{
	BP_OnCombatCue(Cue);

	switch (Cue.CueType)
	{
	case EJargonCombatCueType::Damage:
	case EJargonCombatCueType::PushCollision:
		BP_OnDamageCue(Cue);
		break;

	case EJargonCombatCueType::Heal:
		BP_OnHealCue(Cue);
		break;

	case EJargonCombatCueType::ShieldGained:
	case EJargonCombatCueType::ShieldBroken:
		BP_OnShieldCue(Cue);
		break;

	case EJargonCombatCueType::StunApplied:
	case EJargonCombatCueType::StunConsumed:
		BP_OnStunCue(Cue);
		break;

	case EJargonCombatCueType::FreezeApplied:
	case EJargonCombatCueType::FreezeConsumed:
		BP_OnFreezeCue(Cue);
		break;

	case EJargonCombatCueType::UnitSummoned:
	case EJargonCombatCueType::UnitDied:
		BP_OnUnitCue(Cue);
		break;

	case EJargonCombatCueType::TileEffectPlaced:
	case EJargonCombatCueType::TileEffectTriggered:
	case EJargonCombatCueType::TileEffectExpired:
		BP_OnTileEffectCue(Cue);
		break;

	case EJargonCombatCueType::RelicTriggered:
		BP_OnRelicTriggeredCue(Cue);
		break;

	case EJargonCombatCueType::ClassPassiveTriggered:
		BP_OnHeroClassPassiveTriggeredCue(Cue);
		break;

	case EJargonCombatCueType::HeroAspectTriggered:
	case EJargonCombatCueType::HeroAspectActivated:
		BP_OnHeroAspectTriggeredCue(Cue);
		break;

	case EJargonCombatCueType::ElementalBonusTriggered:
		BP_OnElementalBonusTriggeredCue(Cue);
		break;

	default:
		break;
	}
}
