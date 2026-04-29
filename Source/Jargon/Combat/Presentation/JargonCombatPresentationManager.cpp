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
}

AJargonCombatPresentationManager::AJargonCombatPresentationManager()
{
	PrimaryActorTick.bCanEverTick = false;
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
		return FText::FromString(TEXT("Stun"));

	case EJargonCombatCueType::StunConsumed:
		return FText::FromString(TEXT("Stunned"));

	case EJargonCombatCueType::UnitSummoned:
		return FText::FromString(TEXT("Summoned"));

	case EJargonCombatCueType::UnitDied:
		return FText::FromString(TEXT("Defeated"));

	case EJargonCombatCueType::RelicTriggered:
		if (Cue.SourceRelic && !Cue.SourceRelic->DisplayName.IsEmpty())
		{
			return FText::Format(FText::FromString(TEXT("{0} triggered")), Cue.SourceRelic->DisplayName);
		}
		return FText::FromString(TEXT("Relic triggered"));

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

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraSystem,
		CueLocation,
		FRotator::ZeroRotator);
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

void AJargonCombatPresentationManager::SpawnFloatingText(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const
{
	if (!PresentationSettings ||
		!PresentationSettings->bEnableFloatingText ||
		!PresentationSettings->FloatingTextWidgetClass)
	{
		return;
	}

	const bool bStatusCue =
		Cue.CueType == EJargonCombatCueType::ShieldBroken ||
		Cue.CueType == EJargonCombatCueType::StunApplied ||
		Cue.CueType == EJargonCombatCueType::StunConsumed ||
		Cue.CueType == EJargonCombatCueType::UnitSummoned ||
		Cue.CueType == EJargonCombatCueType::UnitDied;
	if (Cue.Value <= 0 && Cue.TextOverride.IsEmpty() && !bStatusCue)
	{
		return;
	}

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

	default:
		break;
	}
}
