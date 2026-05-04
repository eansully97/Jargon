// JargonCombatGameMode.cpp

#include "Combat/JargonCombatGameMode.h"

#include "Combat/CardResolver.h"
#include "Combat/Effects/JargonEffectContextBuilder.h"
#include "Combat/Effects/JargonEffectExecutor.h"
#include "Combat/JargonCombatPlayerController.h"
#include "Combat/Presentation/JargonCombatPresentationManager.h"
#include "Combat/TacticsCameraPawn.h"
#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonDeckDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonRelicDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"
#include "Exploration/Encounters/EncounterTypes.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Grid/Effects/BattleTileEffect.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"
#include "Kismet/GameplayStatics.h"
#include "Units/BattleUnit.h"
#include "Units/EnemyBattleUnit.h"
#include "Units/PlayerBattleUnit.h"

namespace
{
TArray<FJargonEffectSpec> BuildAbilityEffectPreview(const UJargonAbilityDefinition* AbilityDefinition)
{
	TArray<FJargonEffectSpec> BuiltEffects;
	if (AbilityDefinition)
	{
		AbilityDefinition->BuildEffectSpecs(BuiltEffects);
	}
	return BuiltEffects;
}

void ResolveRunRelicEffects(
	AJargonCombatGameMode* CombatGameMode,
	UJargonRelicDefinition* RelicDefinition,
	const UJargonAbilityDefinition* AbilityDefinition,
	const FJargonEffectContext& Context,
	const TCHAR* HookName,
	EJargonAbilityHookContextType HookContextType)
{
	if (!CombatGameMode || !RelicDefinition || !AbilityDefinition)
	{
		return;
	}

	FJargonCombatCueEvent RelicCue;
	RelicCue.CueType = EJargonCombatCueType::RelicTriggered;
	RelicCue.Trigger = Context.Trigger;
	RelicCue.SourceObject = RelicDefinition;
	RelicCue.SourceRelic = RelicDefinition;
	RelicCue.SourceUnit = Context.SourceUnit;
	RelicCue.TargetUnit = Context.PrimaryUnitTarget;
	RelicCue.SourceTile = Context.SourceTile;
	RelicCue.TargetTile = Context.PrimaryTileTarget;
	RelicCue.WorldLocation = Context.PrimaryTileTarget
		? Context.PrimaryTileTarget->GetActorLocation()
		: (Context.SourceUnit ? Context.SourceUnit->GetActorLocation() : FVector::ZeroVector);
	RelicCue.bHasWorldLocation = Context.PrimaryTileTarget.Get() != nullptr || Context.SourceUnit.Get() != nullptr;
	RelicCue.TextOverride = RelicDefinition->DisplayName.IsEmpty()
		? FText::FromString(TEXT("Relic triggered"))
		: FText::Format(FText::FromString(TEXT("{0} triggered")), RelicDefinition->DisplayName);
	CombatGameMode->EmitCombatCue(RelicCue);

	const FString SourceLabel = FString::Printf(TEXT("%s %s"), *GetNameSafe(RelicDefinition), HookName ? HookName : TEXT("RunRelic"));
	FJargonEffectExecutor::ExecuteAbility(
		AbilityDefinition,
		Context,
		SourceLabel,
		nullptr,
		HookContextType);
}

FText GetHeroClassDisplayText(EJargonHeroClass HeroClass)
{
	const UEnum* HeroClassEnum = StaticEnum<EJargonHeroClass>();
	return HeroClassEnum
		? HeroClassEnum->GetDisplayNameTextByValue(static_cast<int64>(HeroClass))
		: FText::FromString(TEXT("Hero Class"));
}

FText GetHeroDisplayText(const UJargonHeroDefinition* HeroDefinition)
{
	if (!HeroDefinition)
	{
		return FText::GetEmpty();
	}

	return !HeroDefinition->DisplayName.IsEmpty()
		? HeroDefinition->DisplayName
		: GetHeroClassDisplayText(HeroDefinition->HeroClass);
}

FText GetCombatGameModeElementDisplayText(EJargonElementType ElementType)
{
	const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
	return ElementEnum
		? ElementEnum->GetDisplayNameTextByValue(static_cast<int64>(ElementType))
		: FText::FromString(TEXT("Element"));
}

FText GetHeroAspectEnumDisplayText(EJargonHeroAspect HeroAspect)
{
	const UEnum* HeroAspectEnum = StaticEnum<EJargonHeroAspect>();
	return HeroAspectEnum
		? HeroAspectEnum->GetDisplayNameTextByValue(static_cast<int64>(HeroAspect))
		: FText::FromString(TEXT("Hero Aspect"));
}

FText GetHeroAspectDisplayText(const FJargonHeroAspectDefinition* AspectDefinition, EJargonHeroAspect FallbackAspect)
{
	if (AspectDefinition && !AspectDefinition->DisplayName.IsEmpty())
	{
		return AspectDefinition->DisplayName;
	}

	const EJargonHeroAspect Aspect = AspectDefinition ? AspectDefinition->Aspect : FallbackAspect;
	return GetHeroAspectEnumDisplayText(Aspect);
}

FJargonHeroClassInfo MakeHeroClassInfo(const UJargonHeroDefinition& HeroDefinition)
{
	FJargonHeroClassInfo Info;
	Info.HeroClass = HeroDefinition.HeroClass;
	Info.DisplayName = GetHeroDisplayText(&HeroDefinition);
	Info.Description = HeroDefinition.Description;
	Info.bHasCombatStartPassive = HeroDefinition.CombatStartPassive.HasEffects();
	Info.CombatStartPassiveName = HeroDefinition.CombatStartPassive.PassiveName;
	Info.bHasPlayerTurnStartPassive = HeroDefinition.PlayerTurnStartPassive.HasEffects();
	Info.PlayerTurnStartPassiveName = HeroDefinition.PlayerTurnStartPassive.PassiveName;
	Info.bIsActive = true;
	return Info;
}

FJargonHeroAspectInfo MakeHeroAspectInfo(
	const AJargonCombatGameMode* CombatGameMode,
	const UJargonHeroDefinition& HeroDefinition,
	const FJargonHeroAspectDefinition& AspectDefinition)
{
	const FJargonHeroRuntimeState RuntimeState = CombatGameMode ? CombatGameMode->GetHeroRuntimeState() : FJargonHeroRuntimeState();
	const int32 CurrentCharges = CombatGameMode
		? CombatGameMode->GetElementCharges(AspectDefinition.RequiredElement)
		: 0;
	const int32 ActivationChargeThreshold = CombatGameMode
		? CombatGameMode->GetHeroAspectActivationChargeThreshold()
		: 0;

	FJargonHeroAspectInfo Info;
	Info.HeroClass = HeroDefinition.HeroClass;
	Info.ElementType = AspectDefinition.RequiredElement;
	Info.RequiredElement = AspectDefinition.RequiredElement;
	Info.RequiredElementCharges = FMath::Max(0, ActivationChargeThreshold);
	Info.RequiredCharges = Info.RequiredElementCharges;
	Info.CurrentElementCharges = FMath::Max(0, CurrentCharges);
	Info.CurrentCharges = Info.CurrentElementCharges;
	Info.Aspect = AspectDefinition.Aspect;
	Info.DisplayName = GetHeroAspectDisplayText(&AspectDefinition, AspectDefinition.Aspect);
	Info.Description = AspectDefinition.Description;
	Info.PassiveName = !AspectDefinition.TransformationName.IsEmpty()
		? AspectDefinition.TransformationName
		: (!AspectDefinition.TurnStartPassiveName.IsEmpty()
			? AspectDefinition.TurnStartPassiveName
			: AspectDefinition.EnemyDeathPassiveName);
	if (!AspectDefinition.TurnStartPassiveName.IsEmpty() && !AspectDefinition.EnemyDeathPassiveName.IsEmpty())
	{
		Info.PassiveDescription = FText::Format(
			FText::FromString(TEXT("Turn start: {0}\nEnemy death: {1}")),
			AspectDefinition.TurnStartPassiveName,
			AspectDefinition.EnemyDeathPassiveName);
	}
	else if (!AspectDefinition.TurnStartPassiveName.IsEmpty())
	{
		Info.PassiveDescription = FText::Format(
			FText::FromString(TEXT("Turn start: {0}")),
			AspectDefinition.TurnStartPassiveName);
	}
	else if (!AspectDefinition.EnemyDeathPassiveName.IsEmpty())
	{
		Info.PassiveDescription = FText::Format(
			FText::FromString(TEXT("Enemy death: {0}")),
			AspectDefinition.EnemyDeathPassiveName);
	}
	Info.bHasRequiredCharges = Info.RequiredElementCharges > 0 && Info.CurrentElementCharges >= Info.RequiredElementCharges;
	Info.bRequirementMet = Info.bHasRequiredCharges;
	Info.bIsActive =
		RuntimeState.bHasTransformedAspect &&
		RuntimeState.TransformedAspect == AspectDefinition.Aspect;
	Info.bIsTransformed = Info.bIsActive;
	Info.bTransformationLocked = RuntimeState.bHasTransformedAspect;
	Info.bCanTransformNow =
		!RuntimeState.bHasTransformedAspect &&
		Info.bHasRequiredCharges &&
		AspectDefinition.Aspect != EJargonHeroAspect::None;
	const FText ElementName = GetCombatGameModeElementDisplayText(AspectDefinition.RequiredElement);
	Info.ProgressText = FText::Format(
		FText::FromString(TEXT("{0} Transformation: {1}/{2}")),
		ElementName,
		FText::AsNumber(Info.CurrentElementCharges),
		FText::AsNumber(Info.RequiredElementCharges));
	if (Info.bIsActive)
	{
		Info.StatusText = FText::Format(FText::FromString(TEXT("{0} Transformed")), Info.DisplayName);
	}
	else if (RuntimeState.bHasTransformedAspect)
	{
		Info.StatusText = NSLOCTEXT("JargonHero", "AspectTransformationLocked", "Transformation locked");
	}
	else
	{
		Info.StatusText = Info.ProgressText;
	}
	return Info;
}

const FJargonEffectSpec* FindFirstPresentationEffect(const TArray<FJargonEffectSpec>& Effects, bool bPreferNonChargeEffect)
{
	if (bPreferNonChargeEffect)
	{
		for (const FJargonEffectSpec& Effect : Effects)
		{
			if (Effect.Operation != EJargonEffectOperation::GainElementCharge)
			{
				return &Effect;
			}
		}
	}

	return Effects.Num() > 0 ? &Effects[0] : nullptr;
}

const FJargonEffectSpec* FindFirstElementChargeEffect(const TArray<FJargonEffectSpec>& Effects)
{
	for (const FJargonEffectSpec& Effect : Effects)
	{
		if (Effect.Operation == EJargonEffectOperation::GainElementCharge &&
			Effect.ElementType != EJargonElementType::None &&
			Effect.Value > 0)
		{
			return &Effect;
		}
	}

	return nullptr;
}

void ApplyPassiveCueEffectMetadata(FJargonCombatCueEvent& Cue, const TArray<FJargonEffectSpec>& Effects, bool bPreferNonChargeEffect)
{
	const FJargonEffectSpec* PrimaryEffect = FindFirstPresentationEffect(Effects, bPreferNonChargeEffect);
	if (PrimaryEffect)
	{
		Cue.Operation = PrimaryEffect->Operation;
		Cue.Value = FMath::Max(0, PrimaryEffect->Value);
		Cue.Radius = FMath::Max(0, PrimaryEffect->Radius);

		if (PrimaryEffect->Operation == EJargonEffectOperation::GainElementCharge)
		{
			Cue.ElementType = PrimaryEffect->ElementType;
			Cue.ElementChargeCount = FMath::Max(0, PrimaryEffect->Value);
		}
	}

	if (const FJargonEffectSpec* ChargeEffect = FindFirstElementChargeEffect(Effects))
	{
		Cue.ElementType = ChargeEffect->ElementType;
		Cue.ElementChargeCount = FMath::Max(0, ChargeEffect->Value);
	}
}

FText BuildElementChargeCueText(const FJargonEffectSpec& Effect)
{
	const FText ElementName = GetCombatGameModeElementDisplayText(Effect.ElementType);
	const int32 ChargeAmount = FMath::Max(0, Effect.Value);
	return FText::Format(
		FText::FromString(TEXT("+{0} {1} {2}")),
		FText::AsNumber(ChargeAmount),
		ElementName,
		ChargeAmount == 1 ? FText::FromString(TEXT("Charge")) : FText::FromString(TEXT("Charges")));
}

FText BuildHeroClassPassiveCueText(
	const UJargonHeroDefinition* HeroDefinition,
	const FText& PassiveName,
	const TArray<FJargonEffectSpec>& Effects)
{
	if (const FJargonEffectSpec* PrimaryEffect = FindFirstPresentationEffect(Effects, false))
	{
		const FText ClassName = GetHeroDisplayText(HeroDefinition);
		switch (PrimaryEffect->Operation)
		{
		case EJargonEffectOperation::GainElementCharge:
			if (PrimaryEffect->ElementType != EJargonElementType::None && PrimaryEffect->Value > 0)
			{
				return BuildElementChargeCueText(*PrimaryEffect);
			}
			break;

		case EJargonEffectOperation::ApplyShield:
			return FText::Format(FText::FromString(TEXT("{0} Shield")), ClassName);

		case EJargonEffectOperation::Heal:
			return FText::Format(FText::FromString(TEXT("{0} Heal")), ClassName);

		default:
			break;
		}
	}

	if (!PassiveName.IsEmpty())
	{
		return PassiveName;
	}

	return FText::Format(FText::FromString(TEXT("{0} Passive")), GetHeroDisplayText(HeroDefinition));
}

FText BuildHeroAspectPassiveCueText(
	const FJargonHeroAspectDefinition* AspectDefinition,
	EJargonHeroAspect FallbackAspect,
	const FText& PassiveName,
	const TArray<FJargonEffectSpec>& Effects)
{
	const FText AspectName = GetHeroAspectDisplayText(AspectDefinition, FallbackAspect);
	const FJargonEffectSpec* PrimaryEffect = FindFirstPresentationEffect(Effects, true);
	if (PrimaryEffect)
	{
		switch (PrimaryEffect->Operation)
		{
		case EJargonEffectOperation::ApplyShield:
			return FText::Format(FText::FromString(TEXT("{0} Shield")), AspectName);

		case EJargonEffectOperation::Heal:
			return FText::Format(FText::FromString(TEXT("{0} Heal")), AspectName);

		case EJargonEffectOperation::DealDamage:
			return FText::Format(FText::FromString(TEXT("{0} Damage")), AspectName);

		case EJargonEffectOperation::ApplyStun:
			return FText::Format(FText::FromString(TEXT("{0} Stun")), AspectName);

		case EJargonEffectOperation::ApplyFreeze:
			return FText::Format(FText::FromString(TEXT("{0} Freeze")), AspectName);

		case EJargonEffectOperation::GainElementCharge:
			return PassiveName.IsEmpty()
				? FText::Format(FText::FromString(TEXT("{0} Charge")), AspectName)
				: PassiveName;

		default:
			break;
		}
	}

	if (!PassiveName.IsEmpty())
	{
		return PassiveName;
	}

	if (!AspectName.IsEmpty())
	{
		return AspectName;
	}

	return PassiveName.IsEmpty() ? FText::FromString(TEXT("Hero Aspect")) : PassiveName;
}

FText BuildHeroAspectActivatedCueText(const FJargonHeroAspectDefinition* AspectDefinition, EJargonHeroAspect FallbackAspect)
{
	const FText AspectName = AspectDefinition && !AspectDefinition->TransformationName.IsEmpty()
		? AspectDefinition->TransformationName
		: GetHeroAspectDisplayText(AspectDefinition, FallbackAspect);
	return FText::Format(FText::FromString(TEXT("{0} Transformed")), AspectName);
}

TArray<FJargonEffectSpec> BuildHeroClassPassiveEffects(
	const UJargonHeroDefinition* HeroDefinition,
	EJargonEffectTrigger Trigger,
	FText& OutPassiveName)
{
	OutPassiveName = FText::GetEmpty();

	if (!HeroDefinition)
	{
		return TArray<FJargonEffectSpec>();
	}

	const FJargonHeroClassPassiveDefinition* PassiveDefinition = nullptr;
	if (Trigger == EJargonEffectTrigger::OnCombatStart)
	{
		PassiveDefinition = &HeroDefinition->CombatStartPassive;
	}
	else if (Trigger == EJargonEffectTrigger::OnTurnStart)
	{
		PassiveDefinition = &HeroDefinition->PlayerTurnStartPassive;
	}

	if (!PassiveDefinition)
	{
		return TArray<FJargonEffectSpec>();
	}

	OutPassiveName = PassiveDefinition->PassiveName.IsEmpty()
		? GetHeroDisplayText(HeroDefinition)
		: PassiveDefinition->PassiveName;
	if (PassiveDefinition->Ability && PassiveDefinition->PassiveName.IsEmpty())
	{
		OutPassiveName = PassiveDefinition->Ability->GetExecutionLabel();
	}

	return BuildAbilityEffectPreview(PassiveDefinition->Ability);
}

const UJargonAbilityDefinition* FindHeroClassPassiveAbility(
	const UJargonHeroDefinition* HeroDefinition,
	EJargonEffectTrigger Trigger)
{
	if (!HeroDefinition)
	{
		return nullptr;
	}

	if (Trigger == EJargonEffectTrigger::OnCombatStart)
	{
		return HeroDefinition->CombatStartPassive.Ability;
	}

	if (Trigger == EJargonEffectTrigger::OnTurnStart)
	{
		return HeroDefinition->PlayerTurnStartPassive.Ability;
	}

	return nullptr;
}

TArray<FJargonEffectSpec> BuildHeroAspectPassiveEffects(
	const UJargonHeroDefinition* HeroDefinition,
	EJargonHeroAspect HeroAspect,
	EJargonEffectTrigger Trigger,
	FText& OutPassiveName)
{
	OutPassiveName = FText::GetEmpty();

	const FJargonHeroAspectDefinition* AspectDefinition = HeroDefinition
		? HeroDefinition->FindAspectDefinitionByAspect(HeroAspect)
		: nullptr;
	if (!AspectDefinition)
	{
		return TArray<FJargonEffectSpec>();
	}

	if (Trigger == EJargonEffectTrigger::OnTurnStart)
	{
		OutPassiveName = AspectDefinition->TurnStartPassiveName.IsEmpty()
			? GetHeroAspectDisplayText(AspectDefinition, HeroAspect)
			: AspectDefinition->TurnStartPassiveName;
		if (AspectDefinition->TurnStartAbility && AspectDefinition->TurnStartPassiveName.IsEmpty())
		{
			OutPassiveName = AspectDefinition->TurnStartAbility->GetExecutionLabel();
		}
		return BuildAbilityEffectPreview(AspectDefinition->TurnStartAbility);
	}

	if (Trigger == EJargonEffectTrigger::OnEnemyDeath)
	{
		OutPassiveName = AspectDefinition->EnemyDeathPassiveName.IsEmpty()
			? GetHeroAspectDisplayText(AspectDefinition, HeroAspect)
			: AspectDefinition->EnemyDeathPassiveName;
		if (AspectDefinition->EnemyDeathAbility && AspectDefinition->EnemyDeathPassiveName.IsEmpty())
		{
			OutPassiveName = AspectDefinition->EnemyDeathAbility->GetExecutionLabel();
		}
		return BuildAbilityEffectPreview(AspectDefinition->EnemyDeathAbility);
	}

	return TArray<FJargonEffectSpec>();
}

const UJargonAbilityDefinition* FindHeroAspectPassiveAbility(
	const UJargonHeroDefinition* HeroDefinition,
	EJargonHeroAspect HeroAspect,
	EJargonEffectTrigger Trigger)
{
	const FJargonHeroAspectDefinition* AspectDefinition = HeroDefinition
		? HeroDefinition->FindAspectDefinitionByAspect(HeroAspect)
		: nullptr;
	if (!AspectDefinition)
	{
		return nullptr;
	}

	if (Trigger == EJargonEffectTrigger::OnTurnStart)
	{
		return AspectDefinition->TurnStartAbility;
	}

	if (Trigger == EJargonEffectTrigger::OnEnemyDeath)
	{
		return AspectDefinition->EnemyDeathAbility;
	}

	return nullptr;
}

bool TryBuildHeroAspectInfo(
	const AJargonCombatGameMode* CombatGameMode,
	const UJargonHeroDefinition& HeroDefinition,
	const FJargonHeroAspectDefinition& AspectDefinition,
	FJargonHeroAspectInfo& OutAspectInfo)
{
	if (!CombatGameMode)
	{
		return false;
	}

	OutAspectInfo = MakeHeroAspectInfo(CombatGameMode, HeroDefinition, AspectDefinition);
	return true;
}

}

AJargonCombatGameMode::AJargonCombatGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AJargonCombatPlayerController::StaticClass();
	CombatPhase = ECombatPhase::BattleStart;
	CurrentRound = 0;
	CurrentEnergy = 0;
	CurrentMaxEnergy = 0;
	CurrentActingEnemy = nullptr;
	ActiveHeroDefinition = nullptr;
	HeroRuntimeState = FJargonHeroRuntimeState();
	DefeatedEnemyCount = 0;
	AccumulatedEnemyKillCurrency = FJargonCurrencyAmount();
	EnergyPerTurn = 1;
	MaxEnergyIncreasePerRound = 1;
	MaxEnergyCap = 3;
	CardsDrawnPerTurn = 1;
	EnemyTurnActionIndex = 0;
	EnemyTurnStartDelay = 0.35f;
	EnemyActionDelay = 0.5f;
	EnemyTurnEndDelay = 0.35f;
	bLogCombatPacingSummary = true;
	bHasLoggedCombatPacingSummary = false;
}

TArray<UCardDefinition*> AJargonCombatGameMode::GetEmergencyStartingDeckCards() const
{
	if (!EmergencyStartingDeckDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatGameMode has no EmergencyStartingDeckDefinition assigned for direct combat seeding."));
		return TArray<UCardDefinition*>();
	}

	if (!EmergencyStartingDeckDefinition->IsValidDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat emergency starting deck definition '%s' failed validation. Runtime will preserve resolved non-null cards."),
			*GetNameSafe(EmergencyStartingDeckDefinition));
	}

	TArray<UCardDefinition*> ResolvedCards = EmergencyStartingDeckDefinition->GetResolvedCards();
	if (ResolvedCards.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Combat emergency starting deck definition '%s' resolved no cards."),
			*GetNameSafe(EmergencyStartingDeckDefinition));
	}

	return ResolvedCards;
}

void AJargonCombatGameMode::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombat();
}

void AJargonCombatGameMode::InitializeCombat()
{
	SetCombatPhase(ECombatPhase::BattleStart);
	SetCurrentEnergy(0);
	ActiveHeroDefinition = nullptr;
	HeroRuntimeState = FJargonHeroRuntimeState();
	ClearElementCharges();
	ResetCombatRewardState();
	ResetCombatPacingSummary();

	UE_LOG(LogTemp, Log, TEXT("Combat GameMode initialized for world '%s'."), *GetNameSafe(GetWorld()));

	InitializePresentationManager();
	InitializeCameraPawn();
	FindGridBoard();
	SpawnCombatants();
	InitializeCombatPacingSummary();
	InitializeCombatantFacing();
	StartBattleFlow();
}

void AJargonCombatGameMode::InitializeCombatantFacing()
{
	if (!PlayerUnit)
	{
		return;
	}

	ABattleUnit* FirstValidEnemy = nullptr;

	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			FirstValidEnemy = EnemyUnit;
			break;
		}
	}

	if (FirstValidEnemy)
	{
		PlayerUnit->FaceLocation(FirstValidEnemy->GetActorLocation());
	}

	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
		{
			continue;
		}

		if (ABattleUnit* TargetUnit = FindPreferredEnemyTarget(EnemyUnit))
		{
			EnemyUnit->FaceLocation(TargetUnit->GetActorLocation());
		}
		else
		{
			EnemyUnit->FaceLocation(PlayerUnit->GetActorLocation());
		}
	}
}

void AJargonCombatGameMode::SetCombatPhase(ECombatPhase NewPhase)
{
	if (CombatPhase == NewPhase)
	{
		return;
	}

	CombatPhase = NewPhase;
	RefreshSelectedFriendlyUnitPresentation();
	OnPhaseChanged.Broadcast(CombatPhase);
	BroadcastPlayerActionAvailabilityChanged();
}

void AJargonCombatGameMode::SetCurrentEnergy(int32 NewEnergy)
{
	NewEnergy = FMath::Max(0, NewEnergy);

	if (CurrentEnergy == NewEnergy)
	{
		return;
	}

	CurrentEnergy = NewEnergy;
	OnEnergyChanged.Broadcast(CurrentEnergy);
}

void AJargonCombatGameMode::AddCurrentEnergy(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	SetCurrentEnergy(FMath::Min(CurrentEnergy + Amount, CurrentMaxEnergy));
}

int32 AJargonCombatGameMode::GetElementCharges(EJargonElementType Element) const
{
	if (Element == EJargonElementType::None)
	{
		return 0;
	}

	const int32* FoundCharges = ElementCharges.Find(Element);
	return FoundCharges ? FMath::Max(0, *FoundCharges) : 0;
}

void AJargonCombatGameMode::GainElementCharges(EJargonElementType Element, int32 Amount)
{
	if (Element == EJargonElementType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GainElementCharges ignored because ElementType was None."));
		return;
	}

	if (Amount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GainElementCharges ignored because Amount was not positive."));
		return;
	}

	const int32 ChargeCap = GetElementChargeCap();
	const int32 OldAmount = GetElementCharges(Element);
	const int32 NewAmount = FMath::Clamp(OldAmount + Amount, 0, ChargeCap);
	if (NewAmount == OldAmount)
	{
		return;
	}

	ElementCharges.Add(Element, NewAmount);
	const bool bLockedTransformation = TryLockHeroAspectTransformation(Element, OldAmount, NewAmount);
	OnElementChargesChanged.Broadcast();
	RefreshHeroRuntimeStateFromElements();

	if (bLockedTransformation && ActiveHeroDefinition)
	{
		const FJargonHeroAspectDefinition* AspectDefinition =
			ActiveHeroDefinition->FindAspectDefinitionByAspect(HeroRuntimeState.TransformedAspect);
		EmitHeroAspectTransformationCue(AspectDefinition);
		if (AspectDefinition)
		{
			ResolveHeroAspectTransformationEffects(*AspectDefinition);
		}
	}
}

bool AJargonCombatGameMode::HasElementCharges(EJargonElementType Element, int32 Amount) const
{
	if (Element == EJargonElementType::None || Amount <= 0)
	{
		return false;
	}

	return GetElementCharges(Element) >= Amount;
}

bool AJargonCombatGameMode::TrySpendElementCharges(EJargonElementType Element, int32 Amount)
{
	if (!HasElementCharges(Element, Amount))
	{
		UE_LOG(LogTemp, Log, TEXT("TrySpendElementCharges failed for %s. Requested=%d Current=%d."),
			*GetCombatGameModeElementDisplayText(Element).ToString(),
			Amount,
			GetElementCharges(Element));
		return false;
	}

	const int32 OldAmount = GetElementCharges(Element);
	const int32 NewAmount = GetElementCharges(Element) - Amount;
	if (NewAmount > 0)
	{
		ElementCharges.Add(Element, NewAmount);
	}
	else
	{
		ElementCharges.Remove(Element);
	}

	UE_LOG(LogTemp, Log, TEXT("Spent %d %s charges. %d -> %d."),
		Amount,
		*GetCombatGameModeElementDisplayText(Element).ToString(),
		OldAmount,
		FMath::Max(0, NewAmount));
	OnElementChargesChanged.Broadcast();
	RefreshHeroRuntimeStateFromElements();
	return true;
}

void AJargonCombatGameMode::ClearElementCharges()
{
	if (ElementCharges.Num() <= 0)
	{
		return;
	}

	ElementCharges.Reset();
	OnElementChargesChanged.Broadcast();
	RefreshHeroRuntimeStateFromElements();
}

bool AJargonCombatGameMode::GetActiveHeroClassInfo(FJargonHeroClassInfo& OutClassInfo) const
{
	if (!ActiveHeroDefinition)
	{
		OutClassInfo = FJargonHeroClassInfo();
		return false;
	}

	return GetHeroClassInfo(ActiveHeroDefinition->HeroClass, OutClassInfo);
}

bool AJargonCombatGameMode::GetHeroClassInfo(EJargonHeroClass HeroClass, FJargonHeroClassInfo& OutClassInfo) const
{
	if (!ActiveHeroDefinition || ActiveHeroDefinition->HeroClass != HeroClass)
	{
		OutClassInfo = FJargonHeroClassInfo();
		return false;
	}

	OutClassInfo = MakeHeroClassInfo(*ActiveHeroDefinition);
	return true;
}

TArray<FJargonHeroClassInfo> AJargonCombatGameMode::GetConfiguredHeroClassInfos() const
{
	TArray<FJargonHeroClassInfo> Infos;
	if (ActiveHeroDefinition)
	{
		Infos.Add(MakeHeroClassInfo(*ActiveHeroDefinition));
	}

	return Infos;
}

FText AJargonCombatGameMode::GetActiveHeroClassDisplayName() const
{
	FJargonHeroClassInfo Info;
	return GetActiveHeroClassInfo(Info) ? Info.DisplayName : FText::GetEmpty();
}

FText AJargonCombatGameMode::GetActiveHeroClassDescription() const
{
	FJargonHeroClassInfo Info;
	return GetActiveHeroClassInfo(Info) ? Info.Description : FText::GetEmpty();
}

bool AJargonCombatGameMode::GetActiveHeroAspectInfo(FJargonHeroAspectInfo& OutAspectInfo) const
{
	if (!ActiveHeroDefinition || HeroRuntimeState.ActiveAspect == EJargonHeroAspect::None)
	{
		OutAspectInfo = FJargonHeroAspectInfo();
		return false;
	}

	const FJargonHeroAspectDefinition* Definition = ActiveHeroDefinition->FindAspectDefinitionByAspect(HeroRuntimeState.ActiveAspect);
	if (!Definition)
	{
		OutAspectInfo = FJargonHeroAspectInfo();
		return false;
	}

	return TryBuildHeroAspectInfo(this, *ActiveHeroDefinition, *Definition, OutAspectInfo);
}

bool AJargonCombatGameMode::GetHeroAspectInfo(
	EJargonHeroClass HeroClass,
	EJargonElementType ElementType,
	FJargonHeroAspectInfo& OutAspectInfo) const
{
	return GetHeroAspectInfoForElement(HeroClass, ElementType, OutAspectInfo);
}

bool AJargonCombatGameMode::GetHeroAspectInfoForElement(
	EJargonHeroClass HeroClass,
	EJargonElementType Element,
	FJargonHeroAspectInfo& OutAspectInfo) const
{
	if (!ActiveHeroDefinition || ActiveHeroDefinition->HeroClass != HeroClass)
	{
		OutAspectInfo = FJargonHeroAspectInfo();
		return false;
	}

	const FJargonHeroAspectDefinition* Definition = ActiveHeroDefinition->FindAspectDefinitionForElement(Element);
	if (!Definition)
	{
		OutAspectInfo = FJargonHeroAspectInfo();
		return false;
	}

	return TryBuildHeroAspectInfo(this, *ActiveHeroDefinition, *Definition, OutAspectInfo);
}

TArray<FJargonHeroAspectInfo> AJargonCombatGameMode::GetConfiguredHeroAspectInfos() const
{
	TArray<FJargonHeroAspectInfo> Infos;
	if (!ActiveHeroDefinition)
	{
		return Infos;
	}

	const TArray<FJargonHeroAspectDefinition>& Definitions = ActiveHeroDefinition->GetAuthoredAspectDefinitions();
	Infos.Reserve(Definitions.Num());

	for (const FJargonHeroAspectDefinition& Definition : Definitions)
	{
		FJargonHeroAspectInfo Info;
		if (TryBuildHeroAspectInfo(this, *ActiveHeroDefinition, Definition, Info))
		{
			Infos.Add(Info);
		}
	}

	return Infos;
}

FText AJargonCombatGameMode::GetActiveHeroAspectDisplayName() const
{
	FJargonHeroAspectInfo Info;
	return GetActiveHeroAspectInfo(Info) ? Info.DisplayName : FText::GetEmpty();
}

FText AJargonCombatGameMode::GetActiveHeroAspectDescription() const
{
	FJargonHeroAspectInfo Info;
	return GetActiveHeroAspectInfo(Info) ? Info.Description : FText::GetEmpty();
}

FText AJargonCombatGameMode::GetActiveHeroAspectPassiveName() const
{
	FJargonHeroAspectInfo Info;
	return GetActiveHeroAspectInfo(Info) ? Info.PassiveName : FText::GetEmpty();
}

FText AJargonCombatGameMode::GetActiveHeroDominantElementDisplayName() const
{
	return HeroRuntimeState.DominantElement != EJargonElementType::None
		? GetCombatGameModeElementDisplayText(HeroRuntimeState.DominantElement)
		: FText::GetEmpty();
}

void AJargonCombatGameMode::RefreshHeroRuntimeStateFromElements()
{
	const FJargonHeroRuntimeState PreviousState = HeroRuntimeState;

	FJargonHeroRuntimeState NewState;
	NewState.BaseHeroDefinition = ActiveHeroDefinition;
	NewState.FireCharges = GetElementCharges(EJargonElementType::Fire);
	NewState.FrostCharges = GetElementCharges(EJargonElementType::Frost);
	NewState.StormCharges = GetElementCharges(EJargonElementType::Storm);
	NewState.NatureCharges = GetElementCharges(EJargonElementType::Nature);
	NewState.RadianceCharges = GetElementCharges(EJargonElementType::Radiance);
	NewState.QuietusCharges = GetElementCharges(EJargonElementType::Quietus);
	NewState.bHasElementInfluence =
		NewState.FireCharges > 0 ||
		NewState.FrostCharges > 0 ||
		NewState.StormCharges > 0 ||
		NewState.NatureCharges > 0 ||
		NewState.RadianceCharges > 0 ||
		NewState.QuietusCharges > 0;
	NewState.DominantElement = NewState.bHasElementInfluence
		? ResolveDominantElementFromCharges(HeroRuntimeState.DominantElement)
		: EJargonElementType::None;
	const FJargonHeroAspectDefinition* DominantAspectDefinition = ActiveHeroDefinition
		? ActiveHeroDefinition->FindAspectDefinitionForElement(NewState.DominantElement)
		: nullptr;
	NewState.TransformedElement = PreviousState.TransformedElement;
	NewState.TransformedAspect = PreviousState.TransformedAspect;
	NewState.bHasTransformedAspect = PreviousState.bHasTransformedAspect;
	NewState.TransformationThreshold = PreviousState.TransformationThreshold;
	NewState.ActiveAspect = NewState.bHasTransformedAspect
		? NewState.TransformedAspect
		: EJargonHeroAspect::None;
	NewState.bHasActiveAspect = NewState.ActiveAspect != EJargonHeroAspect::None;
	NewState.AspectThreshold = NewState.bHasTransformedAspect
		? NewState.TransformationThreshold
		: (DominantAspectDefinition
		? GetHeroAspectActivationChargeThreshold()
		: 0);

	if (!IsHeroRuntimeStateDifferent(PreviousState, NewState))
	{
		return;
	}

	HeroRuntimeState = NewState;
	OnHeroRuntimeStateChanged.Broadcast(HeroRuntimeState);
	BP_OnHeroRuntimeStateChanged(HeroRuntimeState);
}

EJargonElementType AJargonCombatGameMode::ResolveDominantElementFromCharges(EJargonElementType PreviousDominantElement) const
{
	constexpr EJargonElementType OrderedElements[] =
	{
		EJargonElementType::Fire,
		EJargonElementType::Frost,
		EJargonElementType::Storm,
		EJargonElementType::Nature,
		EJargonElementType::Radiance,
		EJargonElementType::Quietus
	};

	int32 HighestChargeCount = 0;
	for (const EJargonElementType Element : OrderedElements)
	{
		HighestChargeCount = FMath::Max(HighestChargeCount, GetElementCharges(Element));
	}

	if (HighestChargeCount <= 0)
	{
		return EJargonElementType::None;
	}

	if (PreviousDominantElement != EJargonElementType::None &&
		GetElementCharges(PreviousDominantElement) == HighestChargeCount)
	{
		return PreviousDominantElement;
	}

	for (const EJargonElementType Element : OrderedElements)
	{
		if (GetElementCharges(Element) == HighestChargeCount)
		{
			return Element;
		}
	}

	return EJargonElementType::None;
}

bool AJargonCombatGameMode::TryLockHeroAspectTransformation(EJargonElementType Element, int32 OldCharges, int32 NewCharges)
{
	const int32 TransformationThreshold = GetHeroAspectActivationChargeThreshold();
	if (HeroRuntimeState.bHasTransformedAspect ||
		!ActiveHeroDefinition ||
		Element == EJargonElementType::None ||
		TransformationThreshold <= 0 ||
		OldCharges >= TransformationThreshold ||
		NewCharges < TransformationThreshold)
	{
		return false;
	}

	const FJargonHeroAspectDefinition* AspectDefinition = ActiveHeroDefinition->FindAspectDefinitionForElement(Element);
	if (!AspectDefinition ||
		AspectDefinition->Aspect == EJargonHeroAspect::None ||
		AspectDefinition->RequiredElement == EJargonElementType::None)
	{
		return false;
	}

	HeroRuntimeState.TransformedElement = Element;
	HeroRuntimeState.TransformedAspect = AspectDefinition->Aspect;
	HeroRuntimeState.bHasTransformedAspect = true;
	HeroRuntimeState.TransformationThreshold = TransformationThreshold;
	HeroRuntimeState.ActiveAspect = AspectDefinition->Aspect;
	HeroRuntimeState.bHasActiveAspect = true;
	HeroRuntimeState.AspectThreshold = TransformationThreshold;
	return true;
}

void AJargonCombatGameMode::EmitHeroAspectTransformationCue(const FJargonHeroAspectDefinition* AspectDefinition)
{
	if (!ActiveHeroDefinition || !PlayerUnit || !AspectDefinition)
	{
		return;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();

	FJargonCombatCueEvent AspectActivatedCue;
	AspectActivatedCue.CueType = EJargonCombatCueType::HeroAspectActivated;
	AspectActivatedCue.Trigger = EJargonEffectTrigger::Activated;
	AspectActivatedCue.SourceObject = ActiveHeroDefinition;
	AspectActivatedCue.SourceUnit = PlayerUnit;
	AspectActivatedCue.TargetUnit = PlayerUnit;
	AspectActivatedCue.SourceTile = PlayerTile;
	AspectActivatedCue.TargetTile = PlayerTile;
	AspectActivatedCue.HeroClass = ActiveHeroDefinition->HeroClass;
	AspectActivatedCue.HeroAspect = AspectDefinition->Aspect;
	AspectActivatedCue.ElementType = AspectDefinition->RequiredElement;
	AspectActivatedCue.ElementChargeCount = GetElementCharges(AspectDefinition->RequiredElement);
	AspectActivatedCue.Value = AspectActivatedCue.ElementChargeCount;
	const TArray<FJargonEffectSpec> TransformationPreviewEffects = BuildAbilityEffectPreview(AspectDefinition->TransformationAbility);
	ApplyPassiveCueEffectMetadata(AspectActivatedCue, TransformationPreviewEffects, true);
	AspectActivatedCue.ElementType = AspectDefinition->RequiredElement;
	AspectActivatedCue.ElementChargeCount = GetElementCharges(AspectDefinition->RequiredElement);
	AspectActivatedCue.TextOverride = BuildHeroAspectActivatedCueText(
		AspectDefinition,
		AspectDefinition->Aspect);
	AspectActivatedCue.WorldLocation = PlayerTile ? PlayerTile->GetActorLocation() : PlayerUnit->GetActorLocation();
	AspectActivatedCue.bHasWorldLocation = true;
	EmitCombatCue(AspectActivatedCue);
}

void AJargonCombatGameMode::ResolveHeroAspectTransformationEffects(const FJargonHeroAspectDefinition& AspectDefinition)
{
	if (!ActiveHeroDefinition ||
		!PlayerUnit ||
		!AspectDefinition.TransformationAbility)
	{
		return;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
	FJargonEffectContext Context = FJargonEffectContextBuilder::BuildForUnit(
		this,
		PlayerUnit,
		EJargonEffectTrigger::Activated,
		PlayerUnit,
		PlayerTile,
		ActiveHeroDefinition);
	Context.TriggeringUnit = PlayerUnit;

	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		AspectDefinition.TransformationAbility,
		Context,
		GetNameSafe(ActiveHeroDefinition),
		nullptr,
		EJargonAbilityHookContextType::HeroAspectTransformed);
	if (!ExecutionReport.bResolverSucceeded)
	{
		return;
	}
}

bool AJargonCombatGameMode::IsHeroRuntimeStateDifferent(
	const FJargonHeroRuntimeState& First,
	const FJargonHeroRuntimeState& Second) const
{
	return First.BaseHeroDefinition != Second.BaseHeroDefinition ||
		First.DominantElement != Second.DominantElement ||
		First.bHasElementInfluence != Second.bHasElementInfluence ||
		First.ActiveAspect != Second.ActiveAspect ||
		First.bHasActiveAspect != Second.bHasActiveAspect ||
		First.AspectThreshold != Second.AspectThreshold ||
		First.TransformedElement != Second.TransformedElement ||
		First.TransformedAspect != Second.TransformedAspect ||
		First.bHasTransformedAspect != Second.bHasTransformedAspect ||
		First.TransformationThreshold != Second.TransformationThreshold ||
		First.FireCharges != Second.FireCharges ||
		First.FrostCharges != Second.FrostCharges ||
		First.StormCharges != Second.StormCharges ||
		First.NatureCharges != Second.NatureCharges ||
		First.RadianceCharges != Second.RadianceCharges ||
		First.QuietusCharges != Second.QuietusCharges;
}

void AJargonCombatGameMode::SetCurrentActingEnemy(ABattleUnit* NewActingEnemy)
{
	ABattleUnit* NormalizedNewActingEnemy =
		(IsValid(NewActingEnemy) && !NewActingEnemy->IsDead()) ? NewActingEnemy : nullptr;

	if (CurrentActingEnemy == NormalizedNewActingEnemy)
	{
		return;
	}

	if (IsValid(CurrentActingEnemy))
	{
		CurrentActingEnemy->SetActingHighlight(false);
	}

	CurrentActingEnemy = NormalizedNewActingEnemy;

	if (IsValid(CurrentActingEnemy))
	{
		CurrentActingEnemy->SetActingHighlight(true);
	}

	OnCurrentActingEnemyChanged.Broadcast(CurrentActingEnemy);
}

void AJargonCombatGameMode::BroadcastPlayerActionAvailabilityChanged()
{
	OnPlayerActionAvailabilityChanged.Broadcast(HasPlayerMoveRemaining(), HasPlayerAttackRemaining());
}

bool AJargonCombatGameMode::HasPlayerMoveRemaining() const
{
	ABattleUnit* SelectedUnit = SelectedFriendlyUnit.Get();
	return CombatPhase == ECombatPhase::PlayerTurn
		&& IsValid(SelectedUnit)
		&& !SelectedUnit->IsDead()
		&& SelectedUnit->HasMoveActionRemaining();
}

bool AJargonCombatGameMode::HasPlayerAttackRemaining() const
{
	ABattleUnit* SelectedUnit = SelectedFriendlyUnit.Get();
	return CombatPhase == ECombatPhase::PlayerTurn
		&& IsValid(SelectedUnit)
		&& !SelectedUnit->IsDead()
		&& SelectedUnit->HasAttackActionRemaining();
}

bool AJargonCombatGameMode::IsFriendlyUnitSelectable(const ABattleUnit* Unit) const
{
	if (!Unit || Unit->IsDead() || Unit->GetTeam() != ETeam::Player)
	{
		return false;
	}

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (FriendlyUnit == Unit)
		{
			return true;
		}
	}

	return false;
}

ABattleUnit* AJargonCombatGameMode::FindFallbackSelectedFriendlyUnit() const
{
	if (IsFriendlyUnitSelectable(PlayerUnit))
	{
		return PlayerUnit;
	}

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (IsFriendlyUnitSelectable(FriendlyUnit))
		{
			return FriendlyUnit.Get();
		}
	}

	return nullptr;
}

void AJargonCombatGameMode::RefreshSelectedFriendlyUnitPresentation()
{
	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead())
		{
			continue;
		}

		FriendlyUnit->SetHighlightEnabled(false);
	}

	if (CombatPhase != ECombatPhase::PlayerTurn || !IsFriendlyUnitSelectable(SelectedFriendlyUnit))
	{
		return;
	}

	SelectedFriendlyUnit->SetHighlightColor(SelectedFriendlyUnitHighlightColor);
	SelectedFriendlyUnit->SetHighlightEnabled(true);
}

void AJargonCombatGameMode::SetSelectedFriendlyUnit(ABattleUnit* NewSelectedFriendlyUnit)
{
	ABattleUnit* NormalizedSelectedUnit = IsFriendlyUnitSelectable(NewSelectedFriendlyUnit)
		? NewSelectedFriendlyUnit
		: FindFallbackSelectedFriendlyUnit();

	SelectedFriendlyUnit = NormalizedSelectedUnit;
	RefreshSelectedFriendlyUnitPresentation();
	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();
}

bool AJargonCombatGameMode::TrySelectFriendlyUnit(ABattleUnit* FriendlyUnit)
{
	if (CombatPhase != ECombatPhase::PlayerTurn || !IsFriendlyUnitSelectable(FriendlyUnit))
	{
		return false;
	}

	SetSelectedFriendlyUnit(FriendlyUnit);
	return true;
}

bool AJargonCombatGameMode::StartPresentedBasicAttack(
	ABattleUnit* Attacker,
	ABattleUnit* Target,
	bool bReturnToPlayerTurnAfterSequence,
	bool bContinueEnemyTurnAfterSequence)
{
	if (!Attacker || !Target || !Attacker->CanAttackTarget(Target))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ClearBasicAttackTimer();

	PendingAttackAttacker = Attacker;
	PendingAttackTarget = Target;
	bReturnToPlayerTurnAfterAttackSequence = bReturnToPlayerTurnAfterSequence;
	bContinueEnemyTurnAfterAttackSequence = bContinueEnemyTurnAfterSequence;

	Attacker->PlayBasicAttackPresentation(Target);

	const float DamageDelay = FMath::Max(0.f, Attacker->GetBasicAttackDamageDelay());
	const float PresentationDuration = FMath::Max(0.f, Attacker->GetBasicAttackPresentationDuration());
	const float CompletionDelay = FMath::Max(DamageDelay, PresentationDuration);

	if (DamageDelay <= KINDA_SMALL_NUMBER)
	{
		ApplyPresentedBasicAttackDamage();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			BasicAttackDamageTimerHandle,
			this,
			&AJargonCombatGameMode::ApplyPresentedBasicAttackDamage,
			DamageDelay,
			false
		);
	}

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return true;
	}

	if (CompletionDelay <= KINDA_SMALL_NUMBER)
	{
		HandlePresentedBasicAttackCompleted();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			BasicAttackCompletionTimerHandle,
			this,
			&AJargonCombatGameMode::HandlePresentedBasicAttackCompleted,
			CompletionDelay,
			false
		);
	}

	return true;
}

void AJargonCombatGameMode::ApplyPresentedBasicAttackDamage()
{
	ABattleUnit* Attacker = PendingAttackAttacker.Get();
	ABattleUnit* Target = PendingAttackTarget.Get();
	if (IsValid(Attacker) && IsValid(Target))
	{
		Attacker->PerformBasicAttack(Target);
	}
}

void AJargonCombatGameMode::HandlePresentedBasicAttackCompleted()
{
	const bool bShouldReturnToPlayerTurn = bReturnToPlayerTurnAfterAttackSequence;
	const bool bShouldContinueEnemyTurn = bContinueEnemyTurnAfterAttackSequence;
	ClearBasicAttackTimer();

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	if (bShouldReturnToPlayerTurn)
	{
		SetCombatPhase(ECombatPhase::PlayerTurn);
		RefreshPlayerMovementHighlights();
		BroadcastPlayerActionAvailabilityChanged();
		return;
	}

	if (bShouldContinueEnemyTurn && CombatPhase == ECombatPhase::EnemyTurn)
	{
		SetCurrentActingEnemy(nullptr);
		ScheduleNextEnemyAction(0.f);
	}
}

void AJargonCombatGameMode::ScheduleNextEnemyAction(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EnemyTurnTimerHandle);

	if (DelaySeconds <= KINDA_SMALL_NUMBER)
	{
		EnemyTurnTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &AJargonCombatGameMode::ResolveNextEnemyAction);
		return;
	}

	World->GetTimerManager().SetTimer(EnemyTurnTimerHandle, this, &AJargonCombatGameMode::ResolveNextEnemyAction, DelaySeconds, false);
}

void AJargonCombatGameMode::ScheduleEndEnemyTurn(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EnemyTurnTimerHandle);

	if (DelaySeconds <= KINDA_SMALL_NUMBER)
	{
		EnemyTurnTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &AJargonCombatGameMode::EndEnemyTurn);
		return;
	}

	World->GetTimerManager().SetTimer(EnemyTurnTimerHandle, this, &AJargonCombatGameMode::EndEnemyTurn, DelaySeconds, false);
}

void AJargonCombatGameMode::ClearEnemyTurnTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EnemyTurnTimerHandle);
}

void AJargonCombatGameMode::ClearBasicAttackTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(BasicAttackDamageTimerHandle);
	World->GetTimerManager().ClearTimer(BasicAttackCompletionTimerHandle);
	PendingAttackAttacker.Reset();
	PendingAttackTarget.Reset();
	bReturnToPlayerTurnAfterAttackSequence = false;
	bContinueEnemyTurnAfterAttackSequence = false;
}

void AJargonCombatGameMode::InitializePresentationManager()
{
	PresentationManager = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AJargonCombatPresentationManager> It(World); It; ++It)
	{
		PresentationManager = *It;
		break;
	}

	if (!PresentationManager)
	{
		TSubclassOf<AJargonCombatPresentationManager> ManagerClass = PresentationManagerClass;
		if (!ManagerClass)
		{
			ManagerClass = AJargonCombatPresentationManager::StaticClass();
		}

		PresentationManager = World->SpawnActor<AJargonCombatPresentationManager>(
			ManagerClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator);
	}

	if (PresentationManager)
	{
		PresentationManager->InitializePresentation(PresentationSettings, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat presentation manager failed to spawn. Combat cue events will be ignored."));
	}
}

void AJargonCombatGameMode::EmitCombatCue(const FJargonCombatCueEvent& Cue)
{
	if (PresentationManager)
	{
		PresentationManager->HandleCombatCue(Cue);
	}
}

void AJargonCombatGameMode::InitializeCameraPawn()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeCameraPawn could not find PlayerController 0."));
		return;
	}

	if (CameraPawnClass)
	{
		const FTransform CameraSpawnTransform(FRotator::ZeroRotator, FVector::ZeroVector);

		SpawnedCameraPawn = GetWorld()->SpawnActor<ATacticsCameraPawn>(
			CameraPawnClass,
			CameraSpawnTransform
		);

		if (!SpawnedCameraPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeCameraPawn failed to spawn CameraPawnClass."));
		}
	}
	else
	{
		for (TActorIterator<ATacticsCameraPawn> It(GetWorld()); It; ++It)
		{
			SpawnedCameraPawn = *It;
			break;
		}

		if (!SpawnedCameraPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeCameraPawn found no CameraPawnClass and no placed ATacticsCameraPawn."));
		}
	}

	if (SpawnedCameraPawn)
	{
		PlayerController->Possess(SpawnedCameraPawn);
	}
}

void AJargonCombatGameMode::FindGridBoard()
{
	GridBoard = nullptr;

	for (TActorIterator<AGridBoard> It(GetWorld()); It; ++It)
	{
		GridBoard = *It;
		break;
	}

	if (!GridBoard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat GameMode could not find an AGridBoard in the level."));
	}
}

void AJargonCombatGameMode::RegisterBattleUnitCallbacks(ABattleUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Unit->OnMovementCompleted().RemoveAll(this);
	Unit->OnMovementCompleted().AddUObject(this, &AJargonCombatGameMode::HandleBattleUnitMovementCompleted);
}

TSubclassOf<APlayerBattleUnit> AJargonCombatGameMode::ResolvePlayerUnitClass() const
{
	return PlayerUnitClass;
}

void AJargonCombatGameMode::ApplyActiveHeroToPlayerUnit()
{
	if (!PlayerUnit)
	{
		return;
	}

	const UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	ActiveHeroDefinition = GameInstance ? GameInstance->GetActiveHeroDefinition() : nullptr;
	if (!ActiveHeroDefinition)
	{
		return;
	}

	PlayerUnit->InitializeFromHeroDefinition(ActiveHeroDefinition);
}

void AJargonCombatGameMode::SpawnCombatants()
{
	EnemyUnits.Reset();
	FriendlyUnits.Reset();
	PlayerUnit = nullptr;
	SelectedFriendlyUnit = nullptr;
	ActiveHeroDefinition = nullptr;
	HeroRuntimeState = FJargonHeroRuntimeState();

	if (!GridBoard)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants aborted because GridBoard is null."));
		return;
	}

	const TSubclassOf<APlayerBattleUnit> ResolvedPlayerUnitClass = ResolvePlayerUnitClass();
	if (!ResolvedPlayerUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants aborted because no player unit class is assigned."));
		return;
	}

	AGridTile* PlayerSpawnTile = GridBoard->GetPlayerSpawnTile();
	if (!PlayerSpawnTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants could not find a valid player spawn tile."));
		return;
	}

	if (!PlayerSpawnTile->IsWalkable())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants aborted because player spawn tile is not walkable."));
		return;
	}

	PlayerUnit = GetWorld()->SpawnActor<APlayerBattleUnit>(
		ResolvedPlayerUnitClass,
		PlayerSpawnTile->GetUnitStandLocation(),
		FRotator::ZeroRotator
	);

	if (!PlayerUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants failed to spawn PlayerUnitClass."));
		return;
	}

	ApplyActiveHeroToPlayerUnit();
	RegisterBattleUnitCallbacks(PlayerUnit);
	PlayerUnit->PlaceOnTile(PlayerSpawnTile);
	FriendlyUnits.Add(PlayerUnit);
	RefreshHeroRuntimeStateFromElements();

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (GameInstance && GameInstance->HasPendingEncounterData())
	{
		SpawnEnemiesFromPendingEncounter();

		if (EnemyUnits.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Pending encounter produced no valid enemy spawns. Falling back to default enemy spawn."));
			SpawnFallbackEnemy();
		}
	}
	else
	{
		SpawnFallbackEnemy();
	}

	if (EnemyUnits.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnCombatants completed but no enemy units were spawned."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("SpawnCombatants succeeded. Spawned %d enemy unit(s)."), EnemyUnits.Num());
	}
}

void AJargonCombatGameMode::SpawnEnemiesFromPendingEncounter()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter failed because GameInstance was null."));
		return;
	}

	const FPendingEncounterRuntimeData& PendingEncounter = GameInstance->GetPendingEncounterData();
	if (!PendingEncounter.HasConfiguredCombatEncounter())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter found no configured pending encounter."));
		return;
	}

	for (const FEncounterEnemySpawn& SpawnEntry : PendingEncounter.EnemySpawns)
	{
		if (!SpawnEntry.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter skipped an invalid spawn entry."));
			continue;
		}

		AGridTile* SpawnTile = ResolveEnemySpawnTile(SpawnEntry.SpawnCoord);
		if (!SpawnTile)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter could not resolve a valid tile near coord %s."),
				*SpawnEntry.SpawnCoord.ToString());
			continue;
		}

		if (SpawnTile->GetCoord() != SpawnEntry.SpawnCoord)
		{
			UE_LOG(LogTemp, Log, TEXT("SpawnEnemiesFromPendingEncounter resolved fallback tile %s for preferred coord %s."),
				*SpawnTile->GetCoord().ToString(),
				*SpawnEntry.SpawnCoord.ToString());
		}

		ABattleUnit* SpawnedEnemy = SpawnEnemyUnitAtTile(SpawnEntry.UnitClass, SpawnTile);
		if (!SpawnedEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemiesFromPendingEncounter failed to spawn enemy unit near coord %s."),
				*SpawnEntry.SpawnCoord.ToString());
			continue;
		}

		EnemyUnits.Add(SpawnedEnemy);
	}
}

void AJargonCombatGameMode::SpawnFallbackEnemy()
{
	if (!EnemyUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnFallbackEnemy aborted because EnemyUnitClass is not assigned."));
		return;
	}

	AGridTile* EnemySpawnTile = GridBoard ? GridBoard->GetEnemySpawnTile() : nullptr;
	if (!EnemySpawnTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnFallbackEnemy could not find a valid fallback enemy spawn tile."));
		return;
	}

	ABattleUnit* SpawnedEnemy = SpawnEnemyUnitAtTile(EnemyUnitClass, EnemySpawnTile);
	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnFallbackEnemy failed to spawn EnemyUnitClass."));
		return;
	}

	EnemyUnits.Add(SpawnedEnemy);
}

AGridTile* AJargonCombatGameMode::ResolveEnemySpawnTile(const FHexCoord& PreferredCoord) const
{
	return GridBoard ? GridBoard->FindNearestWalkableTile(PreferredCoord) : nullptr;
}

ABattleUnit* AJargonCombatGameMode::SpawnEnemyUnitAtTile(TSubclassOf<ABattleUnit> UnitClass, AGridTile* SpawnTile)
{
	if (!UnitClass || !SpawnTile || !SpawnTile->IsWalkable())
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABattleUnit* SpawnedEnemy = World->SpawnActor<ABattleUnit>(
		UnitClass,
		SpawnTile->GetUnitStandLocation(),
		FRotator::ZeroRotator
	);

	if (!SpawnedEnemy)
	{
		return nullptr;
	}

	RegisterBattleUnitCallbacks(SpawnedEnemy);
	SpawnedEnemy->PlaceOnTile(SpawnTile);
	return SpawnedEnemy;
}

bool AJargonCombatGameMode::AreAllEnemiesDefeated() const
{
	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			return false;
		}
	}

	return true;
}

void AJargonCombatGameMode::ResetCombatRewardState()
{
	DefeatedEnemyCount = 0;
	AccumulatedEnemyKillCurrency = FJargonCurrencyAmount();
}

void AJargonCombatGameMode::AccumulateEnemyKillReward(ABattleUnit* DeadEnemy)
{
	if (!DeadEnemy)
	{
		return;
	}

	DefeatedEnemyCount++;

	const FJargonCurrencyAmount KillReward = GetEnemyKillCurrencyReward(DeadEnemy);
	AccumulatedEnemyKillCurrency = FJargonCurrencyAmount::FromTotalCopper(
		AccumulatedEnemyKillCurrency.GetTotalCopperValue() + KillReward.GetTotalCopperValue()
	);
}

FJargonCurrencyAmount AJargonCombatGameMode::GetEnemyKillCurrencyReward(const ABattleUnit* DeadEnemy) const
{
	if (const AEnemyBattleUnit* EnemyUnit = Cast<AEnemyBattleUnit>(DeadEnemy))
	{
		return EnemyUnit->GetKillCurrencyReward();
	}

	FJargonCurrencyAmount FallbackReward;
	FallbackReward.Copper = 1;
	return FallbackReward;
}

void AJargonCombatGameMode::ResetCombatPacingSummary()
{
	CombatPacingSummary = FJargonCombatPacingSummary();
	bHasLoggedCombatPacingSummary = false;
}

void AJargonCombatGameMode::InitializeCombatPacingSummary()
{
	int32 InitialEnemies = 0;
	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			++InitialEnemies;
		}
	}

	CombatPacingSummary.InitialEnemyCount = InitialEnemies;
	CombatPacingSummary.HeroStartingHP = PlayerUnit ? PlayerUnit->GetCurrentHP() : 0;
	CombatPacingSummary.HeroStartingMaxHP = PlayerUnit ? PlayerUnit->GetMaxHP() : 0;
}

void AJargonCombatGameMode::RecordCombatCardPlayed(const UCardDefinition* Card, int32 EnergyCost)
{
	if (!Card)
	{
		return;
	}

	CombatPacingSummary.CardsPlayed++;
	CombatPacingSummary.EnergySpentOnCards += FMath::Max(0, EnergyCost);
}

void AJargonCombatGameMode::RecordCombatDamageApplied(
	const ABattleUnit* SourceUnit,
	const ABattleUnit* TargetUnit,
	int32 DamageAmount)
{
	if (!TargetUnit || DamageAmount <= 0)
	{
		return;
	}

	const ETeam TargetTeam = TargetUnit->GetTeam();
	if (TargetTeam == ETeam::Player)
	{
		CombatPacingSummary.PlayerDamageTaken += DamageAmount;
	}
	else
	{
		CombatPacingSummary.EnemyDamageTaken += DamageAmount;
	}

	if (!SourceUnit)
	{
		return;
	}

	const ETeam SourceTeam = SourceUnit->GetTeam();
	if (SourceTeam == ETeam::Player && TargetTeam == ETeam::Enemy)
	{
		CombatPacingSummary.PlayerDamageDealt += DamageAmount;
	}
	else if (SourceTeam == ETeam::Enemy && TargetTeam == ETeam::Player)
	{
		CombatPacingSummary.EnemyDamageDealt += DamageAmount;
	}
}

void AJargonCombatGameMode::RecordCombatHealingApplied(const ABattleUnit* TargetUnit, int32 HealAmount)
{
	if (!TargetUnit || HealAmount <= 0)
	{
		return;
	}

	if (TargetUnit->GetTeam() == ETeam::Player)
	{
		CombatPacingSummary.PlayerHealingReceived += HealAmount;
	}
	else
	{
		CombatPacingSummary.EnemyHealingReceived += HealAmount;
	}
}

void AJargonCombatGameMode::RecordCombatShieldGained(const ABattleUnit* TargetUnit, int32 ShieldAmount)
{
	if (!TargetUnit || ShieldAmount <= 0)
	{
		return;
	}

	if (TargetUnit->GetTeam() == ETeam::Player)
	{
		CombatPacingSummary.PlayerShieldGained += ShieldAmount;
	}
	else
	{
		CombatPacingSummary.EnemyShieldGained += ShieldAmount;
	}
}

void AJargonCombatGameMode::RecordCombatUnitSummoned(const ABattleUnit* SummonedUnit)
{
	if (!SummonedUnit)
	{
		return;
	}

	if (SummonedUnit->GetTeam() == ETeam::Player)
	{
		CombatPacingSummary.PlayerSummonsCreated++;
	}
	else
	{
		CombatPacingSummary.EnemySummonsCreated++;
	}
}

void AJargonCombatGameMode::RecordCombatUnitDied(const ABattleUnit* DeadUnit)
{
	if (!DeadUnit)
	{
		return;
	}

	if (DeadUnit->GetTeam() == ETeam::Enemy)
	{
		CombatPacingSummary.EnemiesKilled++;
		return;
	}

	if (DeadUnit == PlayerUnit)
	{
		CombatPacingSummary.bHeroDied = true;
		return;
	}

	CombatPacingSummary.FriendlyUnitsLost++;
}

void AJargonCombatGameMode::LogCombatPacingSummary(bool bVictory)
{
	if (!bLogCombatPacingSummary || bHasLoggedCombatPacingSummary)
	{
		return;
	}

	bHasLoggedCombatPacingSummary = true;

	int32 RemainingEnemies = 0;
	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (IsValid(EnemyUnit) && !EnemyUnit->IsDead())
		{
			++RemainingEnemies;
		}
	}

	const int32 HeroFinalHP = IsValid(PlayerUnit) ? PlayerUnit->GetCurrentHP() : 0;
	const int32 HeroFinalMaxHP = IsValid(PlayerUnit) ? PlayerUnit->GetMaxHP() : CombatPacingSummary.HeroStartingMaxHP;

	UE_LOG(LogTemp, Display, TEXT("=== Combat Pacing Summary ==="));
	UE_LOG(LogTemp, Display, TEXT("Result: %s"), bVictory ? TEXT("Victory") : TEXT("Defeat"));
	UE_LOG(LogTemp, Display, TEXT("Initial Enemies: %d | Remaining Enemies: %d"), CombatPacingSummary.InitialEnemyCount, RemainingEnemies);
	UE_LOG(LogTemp, Display, TEXT("Player Turns: %d | Enemy Turns: %d"), CombatPacingSummary.PlayerTurnsTaken, CombatPacingSummary.EnemyTurnsTaken);
	UE_LOG(LogTemp, Display, TEXT("Cards Played: %d | Energy Spent: %d"), CombatPacingSummary.CardsPlayed, CombatPacingSummary.EnergySpentOnCards);
	UE_LOG(LogTemp, Display, TEXT("Player Damage Dealt: %d | Player Damage Taken: %d"), CombatPacingSummary.PlayerDamageDealt, CombatPacingSummary.PlayerDamageTaken);
	UE_LOG(LogTemp, Display, TEXT("Enemy Damage Dealt: %d | Enemy Damage Taken: %d"), CombatPacingSummary.EnemyDamageDealt, CombatPacingSummary.EnemyDamageTaken);
	UE_LOG(LogTemp, Display, TEXT("Player Healing Received: %d | Enemy Healing Received: %d"), CombatPacingSummary.PlayerHealingReceived, CombatPacingSummary.EnemyHealingReceived);
	UE_LOG(LogTemp, Display, TEXT("Player Shield Gained: %d | Enemy Shield Gained: %d"), CombatPacingSummary.PlayerShieldGained, CombatPacingSummary.EnemyShieldGained);
	UE_LOG(LogTemp, Display, TEXT("Player Summons: %d | Enemy Summons: %d"), CombatPacingSummary.PlayerSummonsCreated, CombatPacingSummary.EnemySummonsCreated);
	UE_LOG(LogTemp, Display, TEXT("Enemies Killed: %d | Friendly Units Lost: %d | Hero Died: %s"),
		CombatPacingSummary.EnemiesKilled,
		CombatPacingSummary.FriendlyUnitsLost,
		CombatPacingSummary.bHeroDied ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Display, TEXT("Hero HP: %d / %d (Started %d / %d)"),
		HeroFinalHP,
		HeroFinalMaxHP,
		CombatPacingSummary.HeroStartingHP,
		CombatPacingSummary.HeroStartingMaxHP);
}

void AJargonCombatGameMode::StartBattleFlow()
{
	if (!PlayerUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBattleFlow aborted because PlayerUnit is null."));
		return;
	}

	if (EnemyUnits.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBattleFlow aborted because no enemy units were spawned."));
		return;
	}

	CurrentRound = 1;

	StartPlayerTurn();
}

void AJargonCombatGameMode::ClearPendingMovementSequence()
{
	PendingMovementContext = EPendingMovementContext::None;
	PendingMovementUnit.Reset();
}

bool AJargonCombatGameMode::StartPlayerControlledMoveSequence(
	ABattleUnit* MovingUnit,
	const TArray<AGridTile*>& Path,
	bool bConsumeMoveAction)
{
	if (!MovingUnit || MovingUnit->IsDead() || Path.Num() < 2)
	{
		return false;
	}

	PendingMovementContext = EPendingMovementContext::PlayerControlled;
	PendingMovementUnit = MovingUnit;
	SetCombatPhase(ECombatPhase::Resolving);

	if (!MovingUnit->MoveAlongPath(Path))
	{
		ClearPendingMovementSequence();

		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
		}

		return false;
	}

	if (bConsumeMoveAction)
	{
		MovingUnit->ConsumeMoveAction();
	}

	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();
	return true;
}

bool AJargonCombatGameMode::StartEnemyMoveSequence(ABattleUnit* EnemyUnit, const TArray<AGridTile*>& Path)
{
	if (!EnemyUnit || EnemyUnit->IsDead() || Path.Num() < 2)
	{
		return false;
	}

	PendingMovementContext = EPendingMovementContext::Enemy;
	PendingMovementUnit = EnemyUnit;
	if (EnemyUnit->MoveAlongPath(Path))
	{
		return true;
	}

	ClearPendingMovementSequence();
	return false;
}

void AJargonCombatGameMode::HandleBattleUnitMovementCompleted(ABattleUnit* MovedUnit)
{
	if (!MovedUnit || PendingMovementUnit.Get() != MovedUnit)
	{
		return;
	}

	const EPendingMovementContext CompletedContext = PendingMovementContext;
	ClearPendingMovementSequence();

	switch (CompletedContext)
	{
	case EPendingMovementContext::PlayerControlled:
		HandlePlayerControlledMoveCompleted(MovedUnit);
		break;

	case EPendingMovementContext::Enemy:
		HandleEnemyMoveCompleted(MovedUnit);
		break;

	case EPendingMovementContext::None:
	default:
		break;
	}
}

void AJargonCombatGameMode::HandlePlayerControlledMoveCompleted(ABattleUnit* MovedUnit)
{
	if (!MovedUnit)
	{
		return;
	}

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCombatPhase(ECombatPhase::PlayerTurn);
	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();
}

void AJargonCombatGameMode::HandleEnemyMoveCompleted(ABattleUnit* MovedUnit)
{
	if (!MovedUnit)
	{
		return;
	}

	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	if (CombatPhase != ECombatPhase::EnemyTurn)
	{
		return;
	}

	if (MovedUnit->IsDead())
	{
		SetCurrentActingEnemy(nullptr);
		ScheduleNextEnemyAction(0.f);
		return;
	}

	ABattleUnit* PostMoveTarget = FindPreferredEnemyTarget(MovedUnit);
	if (PostMoveTarget && MovedUnit->CanAttackTarget(PostMoveTarget))
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' attacks '%s' after moving for %d damage."),
			*GetNameSafe(MovedUnit),
			*GetNameSafe(PostMoveTarget),
			MovedUnit->GetAttackDamage());

		if (StartPresentedBasicAttack(MovedUnit, PostMoveTarget, false, true))
		{
			return;
		}
	}

	SetCurrentActingEnemy(nullptr);
	ScheduleNextEnemyAction(0.f);
}

void AJargonCombatGameMode::StartPlayerTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	CombatPacingSummary.PlayerTurnsTaken++;

	FriendlyUnits.RemoveAllSwap([](const TObjectPtr<ABattleUnit>& FriendlyUnit)
	{
		return !IsValid(FriendlyUnit) || FriendlyUnit->IsDead();
	});

	bool bPlayerUnitFrozenInStasis = false;

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead())
		{
			continue;
		}

		FriendlyUnit->ClearTemporaryShield();
		FriendlyUnit->ResetTurnActions();

		FriendlyUnit->ConsumeBurnTurn();
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead())
		{
			continue;
		}

		FriendlyUnit->ConsumeRegenTurn();

		FriendlyUnit->ConsumeRootTurn();

		if (FriendlyUnit->ConsumeFreezeTurn())
		{
			UE_LOG(LogTemp, Log, TEXT("Friendly unit '%s' is frozen in stasis and loses its turn-start effects and actions this turn."),
				*GetNameSafe(FriendlyUnit));

			FriendlyUnit->ConsumeMoveAction();
			FriendlyUnit->ConsumeAttackAction();
			bPlayerUnitFrozenInStasis |= FriendlyUnit.Get() == PlayerUnit.Get();
			continue;
		}

		ExecuteOnTurnStartAbility(FriendlyUnit);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead())
		{
			continue;
		}

		if (FriendlyUnit->ConsumeStunTurn())
		{
			UE_LOG(LogTemp, Log, TEXT("Friendly unit '%s' is stunned and loses its actions this turn."),
				*GetNameSafe(FriendlyUnit));

			FriendlyUnit->ConsumeMoveAction();
			FriendlyUnit->ConsumeAttackAction();
		}
	}

	if (CurrentRound == 1)
	{
		ExecuteHeroClassCombatStartPassive();
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		ExecuteRunRelicOnCombatStartAbilities();
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}
	}

	if (!bPlayerUnitFrozenInStasis)
	{
		ExecuteHeroClassPlayerTurnStartPassive();
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		ExecuteHeroAspectPlayerTurnStartPassive();
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}
	}

	ExecuteRunRelicOnPlayerTurnStartAbilities();
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearBasicAttackTimer();
	ClearPendingMovementSequence();
	EnemyTurnActionIndex = 0;
	SetCombatPhase(ECombatPhase::PlayerTurn);
	SetSelectedFriendlyUnit(SelectedFriendlyUnit);
	CurrentMaxEnergy = CalculateMaxEnergyForRound(CurrentRound);
	SetCurrentEnergy(CurrentMaxEnergy);

	if (PlayerUnit)
	{
		FJargonCombatCueEvent TurnStartCue;
		TurnStartCue.CueType = EJargonCombatCueType::TurnStart;
		TurnStartCue.SourceUnit = PlayerUnit;
		TurnStartCue.TargetUnit = PlayerUnit;
		TurnStartCue.SourceTile = PlayerUnit->GetCurrentTile();
		TurnStartCue.TargetTile = PlayerUnit->GetCurrentTile();
		TurnStartCue.WorldLocation = PlayerUnit->GetActorLocation();
		TurnStartCue.bHasWorldLocation = true;
		EmitCombatCue(TurnStartCue);
	}

	NotifyPlayerTurnStartTileEffects();

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (CombatPC)
	{
		CombatPC->ClearSelectedCard();
		CombatPC->DrawCards(CardsDrawnPerTurn);
	}

	RefreshPlayerMovementHighlights();
	BroadcastPlayerActionAvailabilityChanged();

	UE_LOG(LogTemp, Log, TEXT("Player Turn Start - Round %d, Energy %d"), CurrentRound, CurrentEnergy);
}

void AJargonCombatGameMode::EndPlayerTurn()
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return;
	}

	SetCombatPhase(ECombatPhase::Resolving);
	RefreshPlayerMovementHighlights();

	StartEnemyTurn();
}

void AJargonCombatGameMode::StartEnemyTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	CombatPacingSummary.EnemyTurnsTaken++;
	SetCombatPhase(ECombatPhase::EnemyTurn);

	FJargonCombatCueEvent EnemyTurnCue;
	EnemyTurnCue.CueType = EJargonCombatCueType::EnemyTurnStart;
	EnemyTurnCue.WorldLocation = PlayerUnit ? PlayerUnit->GetActorLocation() : FVector::ZeroVector;
	EnemyTurnCue.bHasWorldLocation = PlayerUnit.Get() != nullptr;
	EmitCombatCue(EnemyTurnCue);

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (CombatPC)
	{
		CombatPC->ClearSelectedCard();
	}

	UE_LOG(LogTemp, Log, TEXT("Enemy Turn Start - Round %d"), CurrentRound);

	ResolveEnemyTurn();
}

void AJargonCombatGameMode::ResolveEnemyTurn()
{
	SetCurrentActingEnemy(nullptr);
	ClearPendingMovementSequence();
	EnemyTurnActionIndex = 0;
	ScheduleNextEnemyAction(EnemyTurnStartDelay);
}

void AJargonCombatGameMode::ResolveNextEnemyAction()
{
	if (CombatPhase != ECombatPhase::EnemyTurn)
	{
		return;
	}

	if (!PlayerUnit || PlayerUnit->IsDead())
	{
		SetCurrentActingEnemy(nullptr);
		ClearEnemyTurnTimer();
		HandleDefeat();
		return;
	}

	SetCurrentActingEnemy(nullptr);

	while (EnemyTurnActionIndex < EnemyUnits.Num())
	{
		ABattleUnit* EnemyUnit = EnemyUnits[EnemyTurnActionIndex].Get();
		EnemyTurnActionIndex++;

		if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
		{
			continue;
		}

		if (!PlayerUnit || PlayerUnit->IsDead())
		{
			HandleDefeat();
			return;
		}

		SetCurrentActingEnemy(EnemyUnit);
		const bool bStartedAction = ResolveSingleEnemyAction(EnemyUnit);

		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			ClearEnemyTurnTimer();
			return;
		}

		if (bStartedAction)
		{
			return;
		}

		SetCurrentActingEnemy(nullptr);
	}

	SetCurrentActingEnemy(nullptr);
	ScheduleEndEnemyTurn(EnemyTurnEndDelay);
}

bool AJargonCombatGameMode::ResolveSingleEnemyAction(ABattleUnit* EnemyUnit)
{
	if (!EnemyUnit || EnemyUnit->IsDead())
	{
		return false;
	}

	EnemyUnit->ResetTurnActions();

	EnemyUnit->ConsumeBurnTurn();
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return false;
	}

	if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
	{
		return false;
	}

	EnemyUnit->ConsumeRegenTurn();

	EnemyUnit->ConsumeRootTurn();

	if (EnemyUnit->ConsumeFreezeTurn())
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' is frozen in stasis and skips its turn-start effects and action."),
			*GetNameSafe(EnemyUnit));

		return false;
	}

		ExecuteOnTurnStartAbility(EnemyUnit);
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return false;
	}

	if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
	{
		return false;
	}

	if (EnemyUnit->ConsumeStunTurn())
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' is stunned and skips its action."),
			*GetNameSafe(EnemyUnit));

		return false;
	}

	ABattleUnit* TargetUnit = FindPreferredEnemyTarget(EnemyUnit);
	if (!TargetUnit)
	{
		return false;
	}

	if (EnemyUnit->CanAttackTarget(TargetUnit))
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy '%s' attacks '%s' for %d damage."),
			*GetNameSafe(EnemyUnit),
			*GetNameSafe(TargetUnit),
			EnemyUnit->GetAttackDamage());

		return StartPresentedBasicAttack(EnemyUnit, TargetUnit, false, true);
	}

	AGridTile* BestDestination = FindBestEnemyMoveDestination(EnemyUnit, TargetUnit);
	if (BestDestination && EnemyUnit->GetCurrentTile())
	{
		const TArray<AGridTile*> Path = GridBoard->BuildPath(EnemyUnit->GetCurrentTile(), BestDestination);
		if (Path.Num() >= 2)
		{
			if (StartEnemyMoveSequence(EnemyUnit, Path))
			{
				UE_LOG(LogTemp, Log, TEXT("Enemy '%s' moves to %s."),
					*GetNameSafe(EnemyUnit),
					*BestDestination->GetCoord().ToString());

				return true;
			}
		}
	}

	return false;
}

void AJargonCombatGameMode::EndEnemyTurn()
{
	if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearPendingMovementSequence();
	EnemyTurnActionIndex = 0;
	CurrentRound++;
	StartPlayerTurn();
}

void AJargonCombatGameMode::ExecuteHeroClassCombatStartPassive()
{
	if (!ActiveHeroDefinition || !PlayerUnit)
	{
		return;
	}

	FText PassiveName;
	const TArray<FJargonEffectSpec> Effects = BuildHeroClassPassiveEffects(
		ActiveHeroDefinition,
		EJargonEffectTrigger::OnCombatStart,
		PassiveName);
	ResolveHeroClassPassiveEffects(
		EJargonEffectTrigger::OnCombatStart,
		PassiveName,
		Effects,
		FindHeroClassPassiveAbility(ActiveHeroDefinition, EJargonEffectTrigger::OnCombatStart));
}

void AJargonCombatGameMode::ExecuteHeroClassPlayerTurnStartPassive()
{
	if (!ActiveHeroDefinition || !PlayerUnit)
	{
		return;
	}

	FText PassiveName;
	const TArray<FJargonEffectSpec> Effects = BuildHeroClassPassiveEffects(
		ActiveHeroDefinition,
		EJargonEffectTrigger::OnTurnStart,
		PassiveName);
	ResolveHeroClassPassiveEffects(
		EJargonEffectTrigger::OnTurnStart,
		PassiveName,
		Effects,
		FindHeroClassPassiveAbility(ActiveHeroDefinition, EJargonEffectTrigger::OnTurnStart));
}

void AJargonCombatGameMode::ResolveHeroClassPassiveEffects(
	EJargonEffectTrigger Trigger,
	const FText& PassiveName,
	const TArray<FJargonEffectSpec>& Effects,
	const UJargonAbilityDefinition* AbilityDefinition)
{
	if (!ActiveHeroDefinition || !PlayerUnit || (!AbilityDefinition && Effects.Num() <= 0))
	{
		return;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
	FJargonEffectContext Context = FJargonEffectContextBuilder::BuildForUnit(
		this,
		PlayerUnit,
		Trigger,
		PlayerUnit,
		PlayerTile,
		ActiveHeroDefinition);

	FJargonCombatCueEvent ClassPassiveCue;
	ClassPassiveCue.CueType = EJargonCombatCueType::ClassPassiveTriggered;
	ClassPassiveCue.Trigger = Trigger;
	ClassPassiveCue.SourceObject = ActiveHeroDefinition;
	ClassPassiveCue.SourceUnit = PlayerUnit;
	ClassPassiveCue.SourceTile = PlayerTile;
	ClassPassiveCue.TargetUnit = PlayerUnit;
	ClassPassiveCue.TargetTile = PlayerTile;
	ClassPassiveCue.HeroClass = ActiveHeroDefinition->HeroClass;
	ApplyPassiveCueEffectMetadata(ClassPassiveCue, Effects, false);
	ClassPassiveCue.TextOverride = BuildHeroClassPassiveCueText(
		ActiveHeroDefinition,
		PassiveName,
		Effects);
	ClassPassiveCue.WorldLocation = PlayerUnit->GetActorLocation();
	ClassPassiveCue.bHasWorldLocation = true;
	EmitCombatCue(ClassPassiveCue);
	BP_OnHeroClassPassiveTriggered(ActiveHeroDefinition->HeroClass, Trigger, ClassPassiveCue.TextOverride);

	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		AbilityDefinition,
		Context,
		GetNameSafe(ActiveHeroDefinition),
		nullptr,
		Trigger == EJargonEffectTrigger::OnCombatStart
			? EJargonAbilityHookContextType::HeroClassCombatStart
			: EJargonAbilityHookContextType::HeroClassTurnStart);
	if (!ExecutionReport.bResolverSucceeded)
	{
		return;
	}
}

void AJargonCombatGameMode::ExecuteHeroAspectPlayerTurnStartPassive()
{
	if (!ActiveHeroDefinition || !PlayerUnit || HeroRuntimeState.ActiveAspect == EJargonHeroAspect::None)
	{
		return;
	}

	FText PassiveName;
	const TArray<FJargonEffectSpec> Effects = BuildHeroAspectPassiveEffects(
		ActiveHeroDefinition,
		HeroRuntimeState.ActiveAspect,
		EJargonEffectTrigger::OnTurnStart,
		PassiveName);
	ResolveHeroAspectPassiveEffects(
		EJargonEffectTrigger::OnTurnStart,
		PassiveName,
		Effects,
		FindHeroAspectPassiveAbility(ActiveHeroDefinition, HeroRuntimeState.ActiveAspect, EJargonEffectTrigger::OnTurnStart),
		PlayerUnit,
		PlayerUnit->GetCurrentTile(),
		PlayerUnit);
}

void AJargonCombatGameMode::ExecuteHeroAspectEnemyDeathPassive(ABattleUnit* DeadEnemy, AGridTile* DeathTile)
{
	if (!ActiveHeroDefinition ||
		!PlayerUnit ||
		!DeadEnemy ||
		HeroRuntimeState.ActiveAspect == EJargonHeroAspect::None ||
		DeadEnemy->GetTeam() != ETeam::Enemy)
	{
		return;
	}

	FText PassiveName;
	const TArray<FJargonEffectSpec> Effects = BuildHeroAspectPassiveEffects(
		ActiveHeroDefinition,
		HeroRuntimeState.ActiveAspect,
		EJargonEffectTrigger::OnEnemyDeath,
		PassiveName);
	ResolveHeroAspectPassiveEffects(
		EJargonEffectTrigger::OnEnemyDeath,
		PassiveName,
		Effects,
		FindHeroAspectPassiveAbility(ActiveHeroDefinition, HeroRuntimeState.ActiveAspect, EJargonEffectTrigger::OnEnemyDeath),
		PlayerUnit,
		DeathTile ? DeathTile : PlayerUnit->GetCurrentTile(),
		DeadEnemy);
}

void AJargonCombatGameMode::ResolveHeroAspectPassiveEffects(
	EJargonEffectTrigger Trigger,
	const FText& PassiveName,
	const TArray<FJargonEffectSpec>& Effects,
	const UJargonAbilityDefinition* AbilityDefinition,
	ABattleUnit* PrimaryUnitTarget,
	AGridTile* PrimaryTileTarget,
	ABattleUnit* TriggeringUnit)
{
	if (!ActiveHeroDefinition ||
		!PlayerUnit ||
		HeroRuntimeState.ActiveAspect == EJargonHeroAspect::None ||
		(!AbilityDefinition && Effects.Num() <= 0))
	{
		return;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
	FJargonEffectContext Context = FJargonEffectContextBuilder::BuildForUnit(
		this,
		PlayerUnit,
		Trigger,
		PrimaryUnitTarget ? PrimaryUnitTarget : PlayerUnit,
		PrimaryTileTarget ? PrimaryTileTarget : PlayerTile,
		ActiveHeroDefinition);
	Context.TriggeringUnit = TriggeringUnit ? TriggeringUnit : PlayerUnit;

	FJargonCombatCueEvent AspectPassiveCue;
	AspectPassiveCue.CueType = EJargonCombatCueType::HeroAspectTriggered;
	AspectPassiveCue.Trigger = Trigger;
	AspectPassiveCue.SourceObject = ActiveHeroDefinition;
	AspectPassiveCue.SourceUnit = PlayerUnit;
	AspectPassiveCue.SourceTile = PlayerTile;
	AspectPassiveCue.TargetUnit = PrimaryUnitTarget ? PrimaryUnitTarget : PlayerUnit;
	AspectPassiveCue.TargetTile = PrimaryTileTarget ? PrimaryTileTarget : PlayerTile;
	AspectPassiveCue.HeroClass = ActiveHeroDefinition->HeroClass;
	AspectPassiveCue.HeroAspect = HeroRuntimeState.ActiveAspect;
	AspectPassiveCue.ElementType = HeroRuntimeState.TransformedElement;
	ApplyPassiveCueEffectMetadata(AspectPassiveCue, Effects, true);
	const FJargonHeroAspectDefinition* AspectDefinition = ActiveHeroDefinition->FindAspectDefinitionByAspect(HeroRuntimeState.ActiveAspect);
	AspectPassiveCue.TextOverride = BuildHeroAspectPassiveCueText(
		AspectDefinition,
		HeroRuntimeState.ActiveAspect,
		PassiveName,
		Effects);
	AspectPassiveCue.WorldLocation = PrimaryTileTarget
		? PrimaryTileTarget->GetActorLocation()
		: PlayerUnit->GetActorLocation();
	AspectPassiveCue.bHasWorldLocation = true;
	EmitCombatCue(AspectPassiveCue);
	BP_OnHeroAspectPassiveTriggered(HeroRuntimeState.ActiveAspect, Trigger, AspectPassiveCue.TextOverride);

	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		AbilityDefinition,
		Context,
		GetNameSafe(ActiveHeroDefinition),
		nullptr,
		Trigger == EJargonEffectTrigger::OnEnemyDeath
			? EJargonAbilityHookContextType::HeroAspectEnemyDeath
			: EJargonAbilityHookContextType::HeroAspectTurnStart);
	if (!ExecutionReport.bResolverSucceeded)
	{
		return;
	}
}

void AJargonCombatGameMode::ExecuteRunRelicOnCombatStartAbilities()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance || !PlayerUnit)
	{
		return;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
	if (!PlayerTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Run relic combat-start effects skipped because PlayerUnit has no current tile."));
		return;
	}

	const TArray<UJargonRelicDefinition*> RunRelics = GameInstance->GetRunRelics();
	for (UJargonRelicDefinition* RelicDefinition : RunRelics)
	{
		if (!RelicDefinition || !RelicDefinition->OnCombatStartAbility)
		{
			continue;
		}

		const FJargonEffectContext EffectContext = FJargonEffectContextBuilder::BuildForRelic(
			this,
			RelicDefinition,
			EJargonEffectTrigger::OnCombatStart,
			PlayerUnit,
			PlayerUnit,
			PlayerTile,
			PlayerUnit);

		ResolveRunRelicEffects(
			this,
			RelicDefinition,
			RelicDefinition->OnCombatStartAbility,
			EffectContext,
			TEXT("OnCombatStart"),
			EJargonAbilityHookContextType::BoonCombatStart);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}
	}
}

void AJargonCombatGameMode::ExecuteRunRelicOnPlayerTurnStartAbilities()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance || !PlayerUnit || PlayerUnit->IsDead())
	{
		return;
	}

	AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
	if (!PlayerTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Run relic player-turn-start effects skipped because PlayerUnit has no current tile."));
		return;
	}

	const TArray<UJargonRelicDefinition*> RunRelics = GameInstance->GetRunRelics();
	for (UJargonRelicDefinition* RelicDefinition : RunRelics)
	{
		if (!RelicDefinition || !RelicDefinition->OnPlayerTurnStartAbility)
		{
			continue;
		}

		const FJargonEffectContext EffectContext = FJargonEffectContextBuilder::BuildForRelic(
			this,
			RelicDefinition,
			EJargonEffectTrigger::OnTurnStart,
			PlayerUnit,
			PlayerUnit,
			PlayerTile,
			PlayerUnit);

		ResolveRunRelicEffects(
			this,
			RelicDefinition,
			RelicDefinition->OnPlayerTurnStartAbility,
			EffectContext,
			TEXT("OnPlayerTurnStart"),
			EJargonAbilityHookContextType::BoonPlayerTurnStart);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}
	}
}

void AJargonCombatGameMode::ExecuteRunRelicOnEnemyDeathAbilities(ABattleUnit* DeadEnemy, AGridTile* DeathTile)
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance || !DeadEnemy)
	{
		return;
	}

	if (!DeathTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Run relic enemy-death effects skipped for '%s' because no death tile was captured."),
			*GetNameSafe(DeadEnemy));
		return;
	}

	ABattleUnit* RelicSourceUnit = IsValid(PlayerUnit) && !PlayerUnit->IsDead()
		? PlayerUnit.Get()
		: nullptr;

	const TArray<UJargonRelicDefinition*> RunRelics = GameInstance->GetRunRelics();
	for (UJargonRelicDefinition* RelicDefinition : RunRelics)
	{
		if (!RelicDefinition || !RelicDefinition->OnEnemyDeathAbility)
		{
			continue;
		}

		const FJargonEffectContext EffectContext = FJargonEffectContextBuilder::BuildForRelic(
			this,
			RelicDefinition,
			EJargonEffectTrigger::OnEnemyDeath,
			RelicSourceUnit,
			DeadEnemy,
			DeathTile,
			DeadEnemy);

		ResolveRunRelicEffects(
			this,
			RelicDefinition,
			RelicDefinition->OnEnemyDeathAbility,
			EffectContext,
			TEXT("OnEnemyDeath"),
			EJargonAbilityHookContextType::BoonEnemyDeath);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}
	}
}

void AJargonCombatGameMode::NotifyPlayerTurnStartTileEffects()
{
	ActiveTileEffects.RemoveAllSwap([](const TObjectPtr<ABattleTileEffect>& TileEffect)
	{
		return !IsValid(TileEffect);
	});

	TArray<TObjectPtr<ABattleTileEffect>> TileEffectsSnapshot = ActiveTileEffects;
	for (const TObjectPtr<ABattleTileEffect>& TileEffect : TileEffectsSnapshot)
	{
		if (!IsValid(TileEffect))
		{
			continue;
		}

		TileEffect->HandlePlayerTurnStart(this);

		if (IsValid(TileEffect))
		{
			TileEffect->ConsumeDurationTick();
		}
	}

	ActiveTileEffects.RemoveAllSwap([](const TObjectPtr<ABattleTileEffect>& TileEffect)
	{
		return !IsValid(TileEffect);
	});
}

void AJargonCombatGameMode::RefreshPlayerMovementHighlights()
{
	if (!GridBoard)
	{
		return;
	}

	GridBoard->ClearHighlights();

	if (!IsFriendlyUnitSelectable(SelectedFriendlyUnit) || !SelectedFriendlyUnit->GetCurrentTile())
	{
		return;
	}

	if (CombatPhase == ECombatPhase::PlayerTurn && SelectedFriendlyUnit->HasMoveActionRemaining())
	{
		GridBoard->HighlightReachableTilesFrom(SelectedFriendlyUnit->GetCurrentTile(), SelectedFriendlyUnit->GetMoveRange());
	}
	else
	{
		SelectedFriendlyUnit->GetCurrentTile()->SetHighlightState(ETileHighlightState::Selected);
	}
}

void AJargonCombatGameMode::PreviewUnitMovementRange(ABattleUnit* UnitToPreview)
{
	if (!GridBoard)
	{
		return;
	}

	GridBoard->ClearHighlights();

	if (!UnitToPreview || UnitToPreview->IsDead() || !UnitToPreview->GetCurrentTile())
	{
		return;
	}

	GridBoard->HighlightReachableTilesFrom(
		UnitToPreview->GetCurrentTile(),
		UnitToPreview->GetMoveRange()
	);
}

void AJargonCombatGameMode::RefreshCardTargetHighlights(ABattleUnit* SourceUnit, const UCardDefinition* Card)
{
	if (!GridBoard)
	{
		return;
	}

	GridBoard->ClearHighlights();

	if (!SourceUnit || !Card)
	{
		return;
	}

	AGridTile* SourceTile = SourceUnit->GetCurrentTile();
	if (!SourceTile)
	{
		return;
	}

	if (Card->TargetType == ECardTargetType::Self)
	{
		SourceTile->SetHighlightState(ETileHighlightState::Selected);
		return;
	}

	GridBoard->HighlightTilesInRangeFrom(SourceTile, Card->Range);
}

AJargonCombatPlayerController* AJargonCombatGameMode::GetCombatPlayerController() const
{
	return Cast<AJargonCombatPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

bool AJargonCombatGameMode::DrawCardsForPlayer(int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	AJargonCombatPlayerController* CombatPC = GetCombatPlayerController();
	if (!CombatPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("AJargonCombatGameMode::DrawCardsForPlayer failed because no combat player controller was found."));
		return false;
	}

	CombatPC->DrawCards(Count);
	return true;
}

int32 AJargonCombatGameMode::CalculateMaxEnergyForRound(int32 RoundNumber) const
{
	const int32 RoundIndex = FMath::Max(0, RoundNumber - 1);
	const int32 UncappedMaxEnergy = EnergyPerTurn + (RoundIndex * MaxEnergyIncreasePerRound);

	if (MaxEnergyCap > 0)
	{
		return FMath::Clamp(UncappedMaxEnergy, 0, MaxEnergyCap);
	}

	return FMath::Max(0, UncappedMaxEnergy);
}

ABattleUnit* AJargonCombatGameMode::FindPreferredEnemyTarget(ABattleUnit* EnemyUnit) const
{
	if (!GridBoard || !EnemyUnit || EnemyUnit->IsDead() || !EnemyUnit->GetCurrentTile())
	{
		return nullptr;
	}

	ABattleUnit* BestTarget = nullptr;
	int32 BestScore = MAX_int32;

	for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
	{
		ABattleUnit* CandidateUnit = FriendlyUnit.Get();
		if (!IsFriendlyUnitSelectable(CandidateUnit) || !CandidateUnit->GetCurrentTile())
		{
			continue;
		}

		const int32 DistanceToTarget = GridBoard->GetTileDistance(EnemyUnit->GetCurrentTile(), CandidateUnit->GetCurrentTile());
		if (DistanceToTarget == MAX_int32)
		{
			continue;
		}

		int32 TargetScore = DistanceToTarget * 10;

		if (EnemyUnit->CanAttackTarget(CandidateUnit))
		{
			TargetScore -= 1000;
		}

		TargetScore += CandidateUnit->GetCurrentHP();

		if (CandidateUnit == PlayerUnit)
		{
			TargetScore -= 1;
		}

		if (TargetScore < BestScore)
		{
			BestScore = TargetScore;
			BestTarget = CandidateUnit;
		}
	}

	return BestTarget;
}

ABattleTileEffect* AJargonCombatGameMode::SpawnPersistentTileEffectFromDefinition(
	UJargonTileEffectDefinition* Definition,
	TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass,
	const UCardDefinition* Card,
	const ABattleUnit* SourceUnit,
	AGridTile* TargetTile)
{
	if (!SourceUnit)
	{
		return nullptr;
	}

	return SpawnPersistentTileEffectFromDefinitionForTeam(
		Definition,
		RuntimeTileEffectClass,
		Card,
		SourceUnit->GetTeam(),
		TargetTile);
}

ABattleTileEffect* AJargonCombatGameMode::SpawnPersistentTileEffectFromDefinitionForTeam(
	UJargonTileEffectDefinition* Definition,
	TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass,
	const UCardDefinition* Card,
	ETeam SourceTeam,
	AGridTile* TargetTile)
{
	if (!Definition || !RuntimeTileEffectClass || !TargetTile)
	{
		return nullptr;
	}

	if (!Definition->IsValidDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("Tile effect definition '%s' is invalid and cannot spawn."), *GetNameSafe(Definition));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABattleTileEffect* SpawnedEffect = World->SpawnActor<ABattleTileEffect>(
		RuntimeTileEffectClass,
		TargetTile->GetActorLocation(),
		FRotator::ZeroRotator);

	if (!SpawnedEffect)
	{
		return nullptr;
	}

	SpawnedEffect->InitializeFromDefinition(
		const_cast<UCardDefinition*>(Card),
		SourceTeam,
		Definition);
	SpawnedEffect->PlaceOnTile(TargetTile);

	ActiveTileEffects.Add(SpawnedEffect);
	return SpawnedEffect;
}

void AJargonCombatGameMode::UnregisterPersistentTileEffect(ABattleTileEffect* TileEffect)
{
	if (!TileEffect)
	{
		return;
	}

	ActiveTileEffects.RemoveAllSwap([TileEffect](const TObjectPtr<ABattleTileEffect>& ActiveTileEffect)
	{
		return ActiveTileEffect.Get() == TileEffect;
	});
}

ABattleUnit* AJargonCombatGameMode::SpawnSummonedUnitFromDefinition(
	UJargonSummonedUnitDefinition* Definition,
	TSubclassOf<ABattleUnit> RuntimeSummonedUnitClass,
	const ABattleUnit* SourceUnit,
	AGridTile* TargetTile,
	bool bAttackExhaustedOverride,
	bool bUseAttackExhaustedOverride)
{
	if (!Definition || !RuntimeSummonedUnitClass || !TargetTile)
	{
		return nullptr;
	}

	if (!Definition->IsValidDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("Summon definition '%s' is invalid and cannot spawn."), *GetNameSafe(Definition));
		return nullptr;
	}

	ABattleUnit* SpawnedUnit = SpawnSummonedUnitActor(RuntimeSummonedUnitClass, TargetTile);
	if (!SpawnedUnit)
	{
		return nullptr;
	}

	SpawnedUnit->ApplySummonedUnitDefinition(Definition);

	const bool bAttackExhaustedOnSpawn = bUseAttackExhaustedOverride
		? bAttackExhaustedOverride
		: Definition->bSummonEntersWithAttackExhausted;

	return FinalizeSpawnedSummonedUnit(
		SpawnedUnit,
		SourceUnit,
		TargetTile,
		bAttackExhaustedOnSpawn,
		true);
}

ABattleUnit* AJargonCombatGameMode::SpawnSummonedUnitActor(
	TSubclassOf<ABattleUnit> UnitClass,
	AGridTile* TargetTile)
{
	if (!TargetTile || !UnitClass)
	{
		return nullptr;
	}

	if (!TargetTile->IsWalkable())
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->SpawnActor<ABattleUnit>(
		UnitClass,
		TargetTile->GetUnitStandLocation(),
		FRotator::ZeroRotator);
}

ABattleUnit* AJargonCombatGameMode::FinalizeSpawnedSummonedUnit(
	ABattleUnit* SpawnedUnit,
	const ABattleUnit* SourceUnit,
	AGridTile* TargetTile,
	bool bAttackExhaustedOnSpawn,
	bool bRegisterEnemyTeam)
{
	if (!SpawnedUnit || !TargetTile)
	{
		return nullptr;
	}

	RecordCombatUnitSummoned(SpawnedUnit);
	RegisterBattleUnitCallbacks(SpawnedUnit);
	SpawnedUnit->PlaceOnTile(TargetTile);

	if (SpawnedUnit->GetTeam() == ETeam::Player)
	{
		FriendlyUnits.AddUnique(SpawnedUnit);

		if (bAttackExhaustedOnSpawn)
		{
			SpawnedUnit->ConsumeAttackAction();
		}
	}
	else if (bRegisterEnemyTeam && SpawnedUnit->GetTeam() == ETeam::Enemy)
	{
		EnemyUnits.AddUnique(SpawnedUnit);

		if (bAttackExhaustedOnSpawn)
		{
			SpawnedUnit->ConsumeAttackAction();
		}
	}

	ABattleUnit* FacingTarget = nullptr;
	const TArray<TObjectPtr<ABattleUnit>>& PreferredFacingTargets = bRegisterEnemyTeam && SpawnedUnit->GetTeam() == ETeam::Enemy
		? FriendlyUnits
		: EnemyUnits;

	for (const TObjectPtr<ABattleUnit>& CandidateUnit : PreferredFacingTargets)
	{
		if (IsValid(CandidateUnit) && CandidateUnit.Get() != SpawnedUnit && !CandidateUnit->IsDead())
		{
			FacingTarget = CandidateUnit.Get();
			break;
		}
	}

	if (FacingTarget)
	{
		SpawnedUnit->FaceLocation(FacingTarget->GetActorLocation());
	}
	else if (SourceUnit)
	{
		SpawnedUnit->FaceLocation(SourceUnit->GetActorLocation());
	}

	ExecuteOnSummonedAbility(SpawnedUnit);

	return SpawnedUnit;
}

void AJargonCombatGameMode::ExecuteOnSummonedAbility(ABattleUnit* SummonedUnit)
{
	if (!IsValid(SummonedUnit) || SummonedUnit->IsDead())
	{
		return;
	}

	UJargonAbilityDefinition* OnSummonedAbility = SummonedUnit->GetOnSummonedAbility();
	if (!OnSummonedAbility)
	{
		return;
	}

	AGridTile* SummonedTile = SummonedUnit->GetCurrentTile();
	if (!SummonedTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Summoned unit '%s' has OnSummonedAbility but no current tile."),
			*GetNameSafe(SummonedUnit));
		return;
	}

	const FJargonEffectContext EffectContext = FJargonEffectContextBuilder::BuildForUnit(
		this,
		SummonedUnit,
		EJargonEffectTrigger::OnSummoned,
		nullptr,
		SummonedTile,
		SummonedUnit);

	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		OnSummonedAbility,
		EffectContext,
		GetNameSafe(SummonedUnit),
		nullptr,
		EJargonAbilityHookContextType::SummonOnSummoned);
	if (!ExecutionReport.bResolverSucceeded)
	{
		return;
	}
}

void AJargonCombatGameMode::ExecuteOnTurnStartAbility(ABattleUnit* SourceUnit)
{
	if (!IsValid(SourceUnit) || SourceUnit->IsDead())
	{
		return;
	}

	UJargonAbilityDefinition* OnTurnStartAbility = SourceUnit->GetOnTurnStartAbility();
	if (!OnTurnStartAbility)
	{
		return;
	}

	AGridTile* SourceTile = SourceUnit->GetCurrentTile();
	if (!SourceTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unit '%s' has OnTurnStartAbility but no current tile."),
			*GetNameSafe(SourceUnit));
		return;
	}

	const FJargonEffectContext EffectContext = FJargonEffectContextBuilder::BuildForUnit(
		this,
		SourceUnit,
		EJargonEffectTrigger::OnTurnStart,
		SourceUnit,
		SourceTile,
		SourceUnit);

	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		OnTurnStartAbility,
		EffectContext,
		GetNameSafe(SourceUnit),
		nullptr,
		EJargonAbilityHookContextType::SummonTurnStart);
	if (!ExecutionReport.bResolverSucceeded)
	{
		return;
	}
}

void AJargonCombatGameMode::ExecuteOnDeathAbility(ABattleUnit* DeadUnit, AGridTile* DeathTile)
{
	if (!IsValid(DeadUnit))
	{
		return;
	}

	if (DeadUnit->HasExecutedDeathEffects())
	{
		return;
	}

	DeadUnit->MarkDeathEffectsExecuted();

	UJargonAbilityDefinition* OnDeathAbility = DeadUnit->GetOnDeathAbility();
	if (!OnDeathAbility)
	{
		return;
	}

	if (!DeathTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unit '%s' has OnDeathAbility but no captured death tile."),
			*GetNameSafe(DeadUnit));
		return;
	}

	const FJargonEffectContext EffectContext = FJargonEffectContextBuilder::BuildForUnit(
		this,
		DeadUnit,
		EJargonEffectTrigger::OnDeath,
		DeadUnit,
		DeathTile,
		DeadUnit);

	UE_LOG(LogTemp, Log, TEXT("Executing OnDeathAbility for unit '%s' at tile %s."),
		*GetNameSafe(DeadUnit),
		*DeathTile->GetCoord().ToString());

	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		OnDeathAbility,
		EffectContext,
		GetNameSafe(DeadUnit),
		nullptr,
		EJargonAbilityHookContextType::SummonDeath);
	if (!ExecutionReport.bResolverSucceeded)
	{
		return;
	}
}

void AJargonCombatGameMode::NotifyTileEffectsUnitEntered(ABattleUnit* EnteringUnit, AGridTile* EnteredTile)
{
	if (!EnteringUnit || !EnteredTile)
	{
		return;
	}

	ActiveTileEffects.RemoveAllSwap([](const TObjectPtr<ABattleTileEffect>& TileEffect)
	{
		return !IsValid(TileEffect);
	});

	TArray<TObjectPtr<ABattleTileEffect>> TileEffectsSnapshot = EnteredTile->GetTileEffects();
	for (const TObjectPtr<ABattleTileEffect>& TileEffect : TileEffectsSnapshot)
	{
		if (!IsValid(TileEffect))
		{
			continue;
		}

		TileEffect->HandleUnitEnteredTile(this, EnteringUnit);
	}
}

bool AJargonCombatGameMode::TryPlayCardWithResolvedTile(
	UCardDefinition* Card,
	AGridTile* TileTarget,
	ABattleUnit* ExplicitUnitTarget,
	bool bSkipRangeValidation,
	const TArray<int32>& SelectedElementalBonusIndices)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (!Card || !PlayerUnit || !TileTarget)
	{
		return false;
	}

	if (!bSkipRangeValidation && !Card->UsesBoardTileTargeting())
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPlayCardWithResolvedTile received non-board-target card '%s'."),
			*Card->DisplayName.ToString());
		return false;
	}

	if (Card->Cost > CurrentEnergy)
	{
		UE_LOG(LogTemp, Log, TEXT("Not enough energy to play '%s'. Cost=%d CurrentEnergy=%d"),
			*Card->DisplayName.ToString(), Card->Cost, CurrentEnergy);
		return false;
	}

	if (!bSkipRangeValidation)
	{
		if (!GridBoard)
		{
			return false;
		}

		AGridTile* PlayerTile = PlayerUnit->GetCurrentTile();
		if (!PlayerTile)
		{
			return false;
		}

		const int32 TargetDistance = GridBoard->GetTileDistance(PlayerTile, TileTarget);
		if (TargetDistance > Card->Range)
		{
			UE_LOG(LogTemp, Log, TEXT("Card target out of range. Required <= %d, actual %d."),
				Card->Range, TargetDistance);
			return false;
		}
	}

	ABattleUnit* ResolvedUnitTarget = ExplicitUnitTarget ? ExplicitUnitTarget : TileTarget->GetOccupyingUnit();
	if (Card->RequiresUnitOnTargetTile() && !ResolvedUnitTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("Card '%s' requires a unit on the targeted tile."),
			*Card->DisplayName.ToString());
		return false;
	}

	if (Card->RequiresEmptyTargetTile() && !TileTarget->IsWalkable())
	{
		UE_LOG(LogTemp, Log, TEXT("Card '%s' requires an empty walkable target tile."),
			*Card->DisplayName.ToString());
		return false;
	}

	FCardResolveContext ResolveContext;
	ResolveContext.GameMode = this;
	ResolveContext.SourceUnit = PlayerUnit;
	ResolveContext.UnitTarget = ResolvedUnitTarget;
	ResolveContext.TileTarget = TileTarget;
	ResolveContext.SelectedElementalBonusIndices = SelectedElementalBonusIndices;

	FCardResolveResult ResolveResult;

	SetCombatPhase(ECombatPhase::Resolving);

	const bool bShouldLogNextCardEffectTrace = bLogNextCardEffectTrace;
	bool bResolved = false;
	if (bShouldLogNextCardEffectTrace)
	{
		bLogNextCardEffectTrace = false;

		FJargonEffectTrace CardEffectTrace;
		bResolved = FCardResolver::ResolveCard(Card, ResolveContext, ResolveResult, &CardEffectTrace);

		const FString CardName = Card->DisplayName.IsEmpty()
			? GetNameSafe(Card)
			: Card->DisplayName.ToString();
		UE_LOG(LogTemp, Display, TEXT("Jargon next card effect trace: Card=%s Source=%s UnitTarget=%s TileTarget=%s Resolved=%s"),
			*CardName,
			*GetNameSafe(PlayerUnit.Get()),
			*GetNameSafe(ResolvedUnitTarget),
			*GetNameSafe(TileTarget),
			bResolved ? TEXT("true") : TEXT("false"));

		if (CardEffectTrace.Events.Num() > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("%s"), *CardEffectTrace.ToMultilineString());
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("Jargon next card effect trace: no FJargonEffectResolver trace was produced. The card likely failed before base effects reached the shared resolver."));
		}

		if (CardEffectTrace.bContinuedAsynchronously && SelectedElementalBonusIndices.Num() > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Card trace note: base effects continued asynchronously. Elemental bonus groups are skipped by CardResolver for this resolve pass."));
		}
	}
	else
	{
		bResolved = FCardResolver::ResolveCard(Card, ResolveContext, ResolveResult);
	}

	if (!bResolved)
	{
		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
			RefreshCardTargetHighlights(PlayerUnit, Card);
			BroadcastPlayerActionAvailabilityChanged();
		}

		return false;
	}

	FJargonCombatCueEvent CardCue;
	CardCue.CueType = EJargonCombatCueType::CardPlayed;
	CardCue.Trigger = EJargonEffectTrigger::OnPlayed;
	CardCue.SourceObject = Card;
	CardCue.SourceCard = Card;
	CardCue.SourceUnit = PlayerUnit;
	CardCue.TargetUnit = ResolvedUnitTarget;
	CardCue.SourceTile = PlayerUnit ? PlayerUnit->GetCurrentTile() : nullptr;
	CardCue.TargetTile = TileTarget;
	CardCue.WorldLocation = TileTarget ? TileTarget->GetActorLocation() : (PlayerUnit ? PlayerUnit->GetActorLocation() : FVector::ZeroVector);
	CardCue.bHasWorldLocation = TileTarget != nullptr || PlayerUnit.Get() != nullptr;
	CardCue.TextOverride = Card->DisplayName;
	EmitCombatCue(CardCue);

	// Only after this point should the card be paid for.
	if (ResolveResult.bConsumeEnergy)
	{
		RecordCombatCardPlayed(Card, Card->Cost);
		SetCurrentEnergy(CurrentEnergy - Card->Cost);
	}
	else
	{
		RecordCombatCardPlayed(Card, 0);
	}

	if (ResolveResult.EnergyGainAfterCost > 0)
	{
		AddCurrentEnergy(ResolveResult.EnergyGainAfterCost);
	}

	if (ResolveResult.bConsumePlayerMove && PlayerUnit)
	{
		PlayerUnit->ConsumeMoveAction();
	}

	if (!ResolveResult.bContinuesAsynchronously)
	{
		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
			RefreshPlayerMovementHighlights();
			BroadcastPlayerActionAvailabilityChanged();
		}
	}

	return true;
}

AGridTile* AJargonCombatGameMode::FindBestEnemyMoveDestination(ABattleUnit* EnemyUnit, ABattleUnit* TargetUnit) const
{
	if (!GridBoard || !EnemyUnit || !TargetUnit)
	{
		return nullptr;
	}

	AGridTile* StartTile = EnemyUnit->GetCurrentTile();
	AGridTile* TargetTile = TargetUnit->GetCurrentTile();
	if (!StartTile || !TargetTile)
	{
		return nullptr;
	}

	TArray<AGridTile*> ReachableTiles = GridBoard->FindReachableTiles(StartTile, EnemyUnit->GetMoveRange());
	if (ReachableTiles.Num() == 0)
	{
		return nullptr;
	}

	AGridTile* BestTile = nullptr;
	int32 BestScore = MAX_int32;

	for (AGridTile* CandidateTile : ReachableTiles)
	{
		if (!CandidateTile)
		{
			continue;
		}

		const int32 CandidateScore = GetEnemyTileScore(EnemyUnit, CandidateTile, TargetUnit);
		if (CandidateScore < BestScore)
		{
			BestScore = CandidateScore;
			BestTile = CandidateTile;
		}
	}

	// Only move if the best tile is meaningfully better than staying put.
	const int32 CurrentScore = GetEnemyTileScore(EnemyUnit, StartTile, TargetUnit);
	if (BestTile && BestScore < CurrentScore)
	{
		return BestTile;
	}

	return nullptr;
}

int32 AJargonCombatGameMode::GetPreferredEnemyDistance(const ABattleUnit* EnemyUnit) const
{
	if (!EnemyUnit)
	{
		return 1;
	}

	return FMath::Max(1, EnemyUnit->GetAttackRange());
}

bool AJargonCombatGameMode::CanUnitAttackFromTile(
	const ABattleUnit* EnemyUnit,
	const AGridTile* FromTile,
	const ABattleUnit* TargetUnit
) const
{
	if (!EnemyUnit || !FromTile || !TargetUnit || !TargetUnit->GetCurrentTile())
	{
		return false;
	}

	return GridBoard && GridBoard->AreTilesWithinRange(FromTile, TargetUnit->GetCurrentTile(), EnemyUnit->GetAttackRange());
}

int32 AJargonCombatGameMode::GetEnemyTileScore(
	const ABattleUnit* EnemyUnit,
	const AGridTile* CandidateTile,
	const ABattleUnit* TargetUnit
) const
{
	if (!EnemyUnit || !CandidateTile || !TargetUnit || !TargetUnit->GetCurrentTile())
	{
		return MAX_int32;
	}

	const AGridTile* TargetTile = TargetUnit->GetCurrentTile();
	const int32 PreferredDistance = GetPreferredEnemyDistance(EnemyUnit);
	const int32 DistanceToTarget = GridBoard
		? GridBoard->GetTileDistance(CandidateTile, TargetTile)
		: MAX_int32;
	if (DistanceToTarget == MAX_int32)
	{
		return MAX_int32;
	}

	int32 Score = 0;

	// Main goal: end near preferred range.
	const int32 DistanceError = FMath::Abs(DistanceToTarget - PreferredDistance);
	Score += DistanceError * 10;

	// Strong preference for tiles that allow an attack immediately.
	if (CanUnitAttackFromTile(EnemyUnit, CandidateTile, TargetUnit))
	{
		Score -= 6;
	}

	// Mild penalty for being too close if this is a ranged unit.
	if (EnemyUnit->GetAttackRange() > 1 && DistanceToTarget <= 1)
	{
		Score += 8;
	}

	for (const TObjectPtr<ABattleUnit>& OtherEnemy : EnemyUnits)
	{
		if (!IsValid(OtherEnemy) || OtherEnemy == EnemyUnit || OtherEnemy->IsDead())
		{
			continue;
		}

		AGridTile* OtherTile = OtherEnemy->GetCurrentTile();
		if (!OtherTile)
		{
			continue;
		}

		const int32 DistanceToOther = GridBoard
			? GridBoard->GetTileDistance(CandidateTile, OtherTile)
			: MAX_int32;
		if (DistanceToOther == MAX_int32)
		{
			continue;
		}

		// Slight penalty for standing adjacent to allies.
		if (DistanceToOther <= 1)
		{
			Score += 2;
		}
	}

	return Score;
}



void AJargonCombatGameMode::RequestEndPlayerTurn()
{
	EndPlayerTurn();
}

void AJargonCombatGameMode::RequestLogNextCardEffectTrace()
{
	bLogNextCardEffectTrace = true;
}

bool AJargonCombatGameMode::TryMovePlayerUnitToTile(AGridTile* DestinationTile)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (!IsFriendlyUnitSelectable(SelectedFriendlyUnit) || !SelectedFriendlyUnit->HasMoveActionRemaining())
	{
		return false;
	}

	if (!GridBoard || !DestinationTile)
	{
		return false;
	}

	AGridTile* StartTile = SelectedFriendlyUnit->GetCurrentTile();
	if (!StartTile)
	{
		return false;
	}

	if (DestinationTile == StartTile)
	{
		return false;
	}

	if (!DestinationTile->IsWalkable())
	{
		return false;
	}

	const TArray<AGridTile*> Path = GridBoard->BuildPath(StartTile, DestinationTile);
	if (Path.Num() < 2)
	{
		return false;
	}

	const int32 StepsRequired = Path.Num() - 1;
	if (StepsRequired > SelectedFriendlyUnit->GetMoveRange())
	{
		return false;
	}

	return StartPlayerControlledMoveSequence(SelectedFriendlyUnit, Path, true);
}

bool AJargonCombatGameMode::TryBasicAttackWithPlayerUnit(ABattleUnit* Target)
{
	if (CombatPhase != ECombatPhase::PlayerTurn)
	{
		return false;
	}

	if (!IsFriendlyUnitSelectable(SelectedFriendlyUnit) || !SelectedFriendlyUnit->HasAttackActionRemaining())
	{
		return false;
	}

	if (!Target || Target->IsDead())
	{
		return false;
	}

	if (Target == SelectedFriendlyUnit || Target->GetTeam() == SelectedFriendlyUnit->GetTeam())
	{
		return false;
	}

	if (!SelectedFriendlyUnit->CanAttackTarget(Target))
	{
		UE_LOG(LogTemp, Log, TEXT("Basic attack target is out of range."));
		return false;
	}

	SetCombatPhase(ECombatPhase::Resolving);

	if (!StartPresentedBasicAttack(SelectedFriendlyUnit, Target, true, false))
	{
		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			SetCombatPhase(ECombatPhase::PlayerTurn);
		}
		return false;
	}

	SelectedFriendlyUnit->ConsumeAttackAction();
	BroadcastPlayerActionAvailabilityChanged();
	RefreshPlayerMovementHighlights();
	return true;
}

bool AJargonCombatGameMode::TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target)
{
	const TArray<int32> EmptySelectedElementalBonusIndices;
	return TryPlayCardOnTarget(Card, Target, EmptySelectedElementalBonusIndices);
}

bool AJargonCombatGameMode::TryPlayCardOnTarget(
	UCardDefinition* Card,
	ABattleUnit* Target,
	const TArray<int32>& SelectedElementalBonusIndices)
{
	if (!Card || !Target)
	{
		return false;
	}

	AGridTile* TargetTile = Target->GetCurrentTile();
	return TryPlayCardWithResolvedTile(Card, TargetTile, Target, false, SelectedElementalBonusIndices);
}

bool AJargonCombatGameMode::TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget)
{
	const TArray<int32> EmptySelectedElementalBonusIndices;
	return TryPlayCardOnTile(Card, TileTarget, EmptySelectedElementalBonusIndices);
}

bool AJargonCombatGameMode::TryPlayCardOnTile(
	UCardDefinition* Card,
	AGridTile* TileTarget,
	const TArray<int32>& SelectedElementalBonusIndices)
{
	if (!Card || !TileTarget)
	{
		return false;
	}

	if (!Card->UsesBoardTileTargeting())
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPlayCardOnTile only supports board-target cards."));
		return false;
	}

	return TryPlayCardWithResolvedTile(Card, TileTarget, TileTarget->GetOccupyingUnit(), false, SelectedElementalBonusIndices);
}

bool AJargonCombatGameMode::TryPlayCardOnSelf(UCardDefinition* Card)
{
	const TArray<int32> EmptySelectedElementalBonusIndices;
	return TryPlayCardOnSelf(Card, EmptySelectedElementalBonusIndices);
}

bool AJargonCombatGameMode::TryPlayCardOnSelf(
	UCardDefinition* Card,
	const TArray<int32>& SelectedElementalBonusIndices)
{
	if (!Card || !PlayerUnit)
	{
		return false;
	}

	if (Card->TargetType != ECardTargetType::Self)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPlayCardOnSelf only supports Self target cards."));
		return false;
	}

	if (Card->Cost > CurrentEnergy)
	{
		UE_LOG(LogTemp, Log, TEXT("Not enough energy to play '%s'. Cost=%d CurrentEnergy=%d"),
			*Card->DisplayName.ToString(), Card->Cost, CurrentEnergy);
		return false;
	}

	return TryPlayCardWithResolvedTile(Card, PlayerUnit->GetCurrentTile(), PlayerUnit, true, SelectedElementalBonusIndices);
}

void AJargonCombatGameMode::HandleUnitDied(ABattleUnit* DeadUnit, AGridTile* DeathTile)
{
	if (!DeadUnit)
	{
		return;
	}

	bool bRecordedDeathForPacing = false;

	if (PendingMovementUnit.Get() == DeadUnit)
	{
		const EPendingMovementContext CompletedContext = PendingMovementContext;
		ClearPendingMovementSequence();
		const bool bShouldRestorePlayerTurnAfterMovement =
			(CompletedContext == EPendingMovementContext::PlayerControlled) &&
			(DeadUnit != PlayerUnit);

		if (CombatPhase != ECombatPhase::Victory && CombatPhase != ECombatPhase::Defeat)
		{
			if (bShouldRestorePlayerTurnAfterMovement)
			{
				SetCombatPhase(ECombatPhase::PlayerTurn);
				RefreshPlayerMovementHighlights();
				BroadcastPlayerActionAvailabilityChanged();
			}
			else if (CompletedContext == EPendingMovementContext::Enemy && CombatPhase == ECombatPhase::EnemyTurn)
			{
				SetCurrentActingEnemy(nullptr);
				ScheduleNextEnemyAction(0.f);
			}
		}
	}

	const bool bWasSelectedFriendlyUnit = (DeadUnit == SelectedFriendlyUnit);
	FriendlyUnits.RemoveSingleSwap(DeadUnit);

	if (DeadUnit == PlayerUnit)
	{
		RecordCombatUnitDied(DeadUnit);
		bRecordedDeathForPacing = true;
		SelectedFriendlyUnit = nullptr;
		RefreshSelectedFriendlyUnitPresentation();
		PlayerUnit = nullptr;
		ExecuteOnDeathAbility(DeadUnit, DeathTile);
		if (CombatPhase != ECombatPhase::Victory)
		{
			HandleDefeat();
		}
		return;
	}

	if (DeadUnit == CurrentActingEnemy)
	{
		SetCurrentActingEnemy(nullptr);
	}

	const int32 RemovedCount = EnemyUnits.RemoveSingleSwap(DeadUnit);
	if (RemovedCount > 0)
	{
		RecordCombatUnitDied(DeadUnit);
		bRecordedDeathForPacing = true;
		AccumulateEnemyKillReward(DeadUnit);

		ExecuteOnDeathAbility(DeadUnit, DeathTile);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		ExecuteHeroAspectEnemyDeathPassive(DeadUnit, DeathTile);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		ExecuteRunRelicOnEnemyDeathAbilities(DeadUnit, DeathTile);
		if (CombatPhase == ECombatPhase::Victory || CombatPhase == ECombatPhase::Defeat)
		{
			return;
		}

		if (AreAllEnemiesDefeated())
		{
			HandleVictory();
			return;
		}
	}

	if (bWasSelectedFriendlyUnit)
	{
		SetSelectedFriendlyUnit(nullptr);
	}

	if (!bRecordedDeathForPacing)
	{
		RecordCombatUnitDied(DeadUnit);
	}
	ExecuteOnDeathAbility(DeadUnit, DeathTile);
}

void AJargonCombatGameMode::HandleVictory()
{
	if (CombatPhase == ECombatPhase::Victory)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearBasicAttackTimer();
	ClearPendingMovementSequence();

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		SetCombatPhase(ECombatPhase::Victory);
		LogCombatPacingSummary(true);
		UE_LOG(LogTemp, Warning, TEXT("Combat victory occurred but UJargonGameInstance was not available."));
		return;
	}

	const FName PendingEncounterId = GameInstance->GetPendingEncounterId();
	if (!PendingEncounterId.IsNone())
	{
		GameInstance->MarkEncounterCleared(PendingEncounterId);
	}

	GameInstance->HandleCombatVictory(AccumulatedEnemyKillCurrency, DefeatedEnemyCount);
	SetCombatPhase(ECombatPhase::Victory);
	LogCombatPacingSummary(true);
}

void AJargonCombatGameMode::HandleDefeat()
{
	if (CombatPhase == ECombatPhase::Defeat)
	{
		return;
	}

	SetCurrentActingEnemy(nullptr);
	ClearEnemyTurnTimer();
	ClearBasicAttackTimer();
	ClearPendingMovementSequence();

	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		SetCombatPhase(ECombatPhase::Defeat);
		LogCombatPacingSummary(false);
		UE_LOG(LogTemp, Warning, TEXT("Combat defeat occurred but UJargonGameInstance was not available."));
		return;
	}

	GameInstance->HandleCombatDefeat(AccumulatedEnemyKillCurrency, DefeatedEnemyCount);
	SetCombatPhase(ECombatPhase::Defeat);
	LogCombatPacingSummary(false);
}

void AJargonCombatGameMode::ReturnToExploration()
{
	UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnFromCombat failed because UJargonGameInstance was not available."));
		return;
	}

	const FName DestinationMapName = GameInstance->GetPostCombatDestinationMapName();
	if (DestinationMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnFromCombat failed because no destination map name was available."));
		return;
	}

	UGameplayStatics::OpenLevel(this, DestinationMapName);
}
