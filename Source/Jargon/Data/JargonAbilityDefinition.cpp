#include "Data/JargonAbilityDefinition.h"

#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

namespace
{
FString BoolText(bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

FString PluralSuffix(int32 Value)
{
	return FMath::Abs(Value) == 1 ? TEXT("") : TEXT("s");
}

FString ElementName(EJargonElementType ElementType)
{
	const UEnum* Enum = StaticEnum<EJargonElementType>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(ElementType)).ToString() : TEXT("Element");
}

FString DeliveryName(EJargonEffectDelivery Delivery)
{
	return JargonEffectContracts::GetDeliveryName(Delivery);
}

FString FilterName(EJargonEffectTargetFilter TargetFilter)
{
	return JargonEffectContracts::GetTargetFilterName(TargetFilter);
}

FString TargetingPresetName(EJargonAbilityTargetingPreset Preset)
{
	const UEnum* Enum = StaticEnum<EJargonAbilityTargetingPreset>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Preset)).ToString() : TEXT("Targeting");
}

FString MakeEditorTitle(FString Summary)
{
	Summary.TrimStartAndEndInline();
	while (Summary.EndsWith(TEXT(".")))
	{
		Summary.LeftChopInline(1);
		Summary.TrimEndInline();
	}
	return Summary.IsEmpty() ? TEXT("Ability Effect Line") : Summary;
}

FString FormatAbilityActionSummary(const FString& Operation, const FString& Delivery, const FString& Payload)
{
	return FString::Printf(TEXT("Operation=%s Delivery=%s Payload=%s"), *Operation, *Delivery, *Payload);
}

FString GetStatusDisplayName(const UJargonStatusEffectDefinition* StatusEffectDefinition)
{
	if (StatusEffectDefinition && !StatusEffectDefinition->DisplayName.IsEmpty())
	{
		return StatusEffectDefinition->DisplayName.ToString();
	}

	if (!StatusEffectDefinition)
	{
		return TEXT("Status");
	}

	const UEnum* Enum = StaticEnum<EJargonStatusEffectKind>();
	return Enum
		? Enum->GetDisplayNameTextByValue(static_cast<int64>(StatusEffectDefinition->StatusKind)).ToString()
		: TEXT("Status");
}

void ApplyAbilityDefaultTargeting(const UJargonAbilityDefinition* AbilityDefinition, FJargonEffectSpec& Effect)
{
	if (AbilityDefinition)
	{
		AbilityDefinition->TargetingProfile.ApplyToEffectSpec(Effect);
	}
	else
	{
		Effect.Delivery = EJargonEffectDelivery::ExplicitUnit;
		Effect.TargetFilter = EJargonEffectTargetFilter::Any;
		Effect.Radius = 0;
		Effect.ChainCount = 3;
	}
}

void ApplySelfTargeting(FJargonEffectSpec& Effect)
{
	Effect.Delivery = EJargonEffectDelivery::Self;
	Effect.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
	Effect.Radius = 0;
	Effect.ChainCount = 3;
}

FJargonEffectSpec MakeDefaultTargetedValueEffect(
	const UJargonAbilityDefinition* AbilityDefinition,
	EJargonEffectOperation Operation,
	int32 Value)
{
	FJargonEffectSpec Effect;
	Effect.Operation = Operation;
	Effect.Value = FMath::Max(0, Value);
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	return Effect;
}

bool IsEffectSpecDefinitionValid(const FJargonEffectSpec& EffectSpec)
{
	if (EffectSpec.Operation == EJargonEffectOperation::None)
	{
		return false;
	}

	if (JargonEffectContracts::RequiresValue(EffectSpec.Operation) && EffectSpec.Value <= 0)
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::ApplyStatus &&
		(!EffectSpec.StatusEffectDefinition || !EffectSpec.StatusEffectDefinition->IsValidDefinition()))
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::CleanseStatus &&
		EffectSpec.StatusEffectDefinition &&
		!EffectSpec.StatusEffectDefinition->IsValidDefinition())
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge &&
		EffectSpec.ElementType == EJargonElementType::None)
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::MoveSource && EffectSpec.MoveDistance <= 0)
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PushTarget && EffectSpec.PushDistance <= 0)
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PullTarget && EffectSpec.PullDistance <= 0)
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::SummonUnit &&
		(!EffectSpec.SummonedUnitDefinition || !EffectSpec.RuntimeSummonedUnitClass))
	{
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PlaceTileEffect &&
		(!EffectSpec.TileEffectDefinition || !EffectSpec.RuntimeTileEffectClass))
	{
		return false;
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits && EffectSpec.ChainCount <= 0)
	{
		return false;
	}

	return true;
}
}

EJargonEffectDelivery FJargonAbilityTargetingProfile::GetDelivery() const
{
	switch (Preset)
	{
	case EJargonAbilityTargetingPreset::Self:
		return EJargonEffectDelivery::Self;

	case EJargonAbilityTargetingPreset::TargetTile:
		return EJargonEffectDelivery::ExplicitTile;

	case EJargonAbilityTargetingPreset::EnemiesInRadius:
	case EJargonAbilityTargetingPreset::AlliesInRadius:
	case EJargonAbilityTargetingPreset::UnitsInRadius:
		return EJargonEffectDelivery::UnitsInRadius;

	case EJargonAbilityTargetingPreset::TilesInRadius:
		return EJargonEffectDelivery::TilesInRadius;

	case EJargonAbilityTargetingPreset::ChainEnemies:
		return EJargonEffectDelivery::ChainUnits;

	case EJargonAbilityTargetingPreset::SelectedEnemy:
	case EJargonAbilityTargetingPreset::SelectedAlly:
	case EJargonAbilityTargetingPreset::SelectedUnit:
	default:
		return EJargonEffectDelivery::ExplicitUnit;
	}
}

EJargonEffectTargetFilter FJargonAbilityTargetingProfile::GetTargetFilter() const
{
	switch (Preset)
	{
	case EJargonAbilityTargetingPreset::Self:
		return EJargonEffectTargetFilter::SourceOnly;

	case EJargonAbilityTargetingPreset::SelectedEnemy:
	case EJargonAbilityTargetingPreset::EnemiesInRadius:
	case EJargonAbilityTargetingPreset::ChainEnemies:
		return EJargonEffectTargetFilter::EnemyToSource;

	case EJargonAbilityTargetingPreset::SelectedAlly:
	case EJargonAbilityTargetingPreset::AlliesInRadius:
		return EJargonEffectTargetFilter::FriendlyToSource;

	case EJargonAbilityTargetingPreset::SelectedUnit:
	case EJargonAbilityTargetingPreset::UnitsInRadius:
	case EJargonAbilityTargetingPreset::TilesInRadius:
	case EJargonAbilityTargetingPreset::TargetTile:
	default:
		return EJargonEffectTargetFilter::Any;
	}
}

bool FJargonAbilityTargetingProfile::UsesRadius() const
{
	return Preset == EJargonAbilityTargetingPreset::EnemiesInRadius
		|| Preset == EJargonAbilityTargetingPreset::AlliesInRadius
		|| Preset == EJargonAbilityTargetingPreset::UnitsInRadius
		|| Preset == EJargonAbilityTargetingPreset::TilesInRadius
		|| Preset == EJargonAbilityTargetingPreset::ChainEnemies;
}

bool FJargonAbilityTargetingProfile::UsesChain() const
{
	return Preset == EJargonAbilityTargetingPreset::ChainEnemies;
}

FString FJargonAbilityTargetingProfile::GetSummary() const
{
	TArray<FString> Parts;
	Parts.Add(FString::Printf(TEXT("Preset=%s"), *TargetingPresetName(Preset)));
	Parts.Add(FString::Printf(TEXT("Delivery=%s"), *DeliveryName(GetDelivery())));
	Parts.Add(FString::Printf(TEXT("Filter=%s"), *FilterName(GetTargetFilter())));
	if (UsesRadius())
	{
		Parts.Add(FString::Printf(TEXT("Radius=%d"), FMath::Max(0, Radius)));
	}
	if (UsesChain())
	{
		Parts.Add(FString::Printf(TEXT("ChainCount=%d"), FMath::Max(1, ChainCount)));
	}
	return FString::Join(Parts, TEXT(" "));
}

void FJargonAbilityTargetingProfile::ApplyToEffectSpec(FJargonEffectSpec& Effect) const
{
	Effect.Delivery = GetDelivery();
	Effect.TargetFilter = GetTargetFilter();
	Effect.Radius = UsesRadius() ? FMath::Max(0, Radius) : 0;
	Effect.ChainCount = UsesChain() ? FMath::Max(1, ChainCount) : 3;
}

FText FJargonAbilityCueDefinition::GetLabelOrFallback(const FText& Fallback) const
{
	return CueLabel.IsEmpty() ? Fallback : CueLabel;
}

FString FJargonAbilityCueDefinition::GetAuditSummary() const
{
	return FString::Printf(
		TEXT("CueLabel=%s Icon=%s CueTypeHint=%s FloatingTextOverride=%s"),
		*CueLabel.ToString(),
		*GetNameSafe(Icon.Get()),
		*CueTypeHint.ToString(),
		*FloatingTextOverride.ToString());
}

UJargonAbilityAction::UJargonAbilityAction()
{
	EditorTitle = FText::FromString(TEXT("Ability Effect Line"));
}

void UJargonAbilityAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
}

FString UJargonAbilityAction::GetOperationName() const
{
	return GetClass() ? GetClass()->GetName() : TEXT("Custom");
}

FString UJargonAbilityAction::GetDeliverySummary() const
{
	return TEXT("Ability Targeting Profile");
}

FString UJargonAbilityAction::GetPayloadSummary() const
{
	return TEXT("None");
}

FString UJargonAbilityAction::GetActionSummary() const
{
	return FormatAbilityActionSummary(GetOperationName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonAbilityAction::GetRulesText() const
{
	return GetActionSummary();
}

void UJargonAbilityAction::RefreshEditorTitle()
{
	EditorTitle = FText::FromString(MakeEditorTitle(GetActionSummary()));
}

void UJargonAbilityAction::PostLoad()
{
	Super::PostLoad();
	RefreshEditorTitle();
}

#if WITH_EDITOR
void UJargonAbilityAction::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshEditorTitle();
}

bool UJargonAbilityAction::ValidateAction(
	const UJargonAbilityDefinition* AbilityDefinition,
	const FString& ActionLabel,
	FDataValidationContext& Context) const
{
	bool bValid = true;
	TArray<FJargonEffectSpec> BuiltEffects;
	BuildEffectSpecs(AbilityDefinition, BuiltEffects);

	if (BuiltEffects.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(
			Context,
			AbilityDefinition,
			FString::Printf(TEXT("%s did not build any effect specs."), *ActionLabel));
		return false;
	}

	for (int32 EffectIndex = 0; EffectIndex < BuiltEffects.Num(); ++EffectIndex)
	{
		bValid &= JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			AbilityDefinition,
			BuiltEffects[EffectIndex],
			FString::Printf(TEXT("%s effect %d"), *ActionLabel, EffectIndex),
			AbilityDefinition ? AbilityDefinition->ExpectedTrigger : EJargonEffectTrigger::Activated,
			Context);
	}

	return bValid;
}
#endif

UJargonAbilityDamageAction::UJargonAbilityDamageAction()
{
	EditorTitle = FText::FromString(TEXT("Deal Damage"));
}

void UJargonAbilityDamageAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Add(MakeDefaultTargetedValueEffect(AbilityDefinition, EJargonEffectOperation::DealDamage, Damage));
}

FString UJargonAbilityDamageAction::GetOperationName() const
{
	return TEXT("Damage");
}

FString UJargonAbilityDamageAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d damage"), Damage);
}

FString UJargonAbilityDamageAction::GetRulesText() const
{
	return FString::Printf(TEXT("Deal %d damage."), Damage);
}

UJargonAbilityLifestealDamageAction::UJargonAbilityLifestealDamageAction()
{
	EditorTitle = FText::FromString(TEXT("Lifesteal Damage"));
}

void UJargonAbilityLifestealDamageAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect = MakeDefaultTargetedValueEffect(AbilityDefinition, EJargonEffectOperation::DealDamage, Damage);
	Effect.bLifesteal = true;
	OutEffects.Add(Effect);
}

FString UJargonAbilityLifestealDamageAction::GetOperationName() const
{
	return TEXT("Lifesteal");
}

FString UJargonAbilityLifestealDamageAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d damage heal unblocked"), Damage);
}

FString UJargonAbilityLifestealDamageAction::GetRulesText() const
{
	return FString::Printf(TEXT("Deal %d damage. Heal for unblocked damage dealt."), Damage);
}

UJargonAbilityHealAction::UJargonAbilityHealAction()
{
	EditorTitle = FText::FromString(TEXT("Heal"));
}

void UJargonAbilityHealAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Add(MakeDefaultTargetedValueEffect(AbilityDefinition, EJargonEffectOperation::Heal, Healing));
}

FString UJargonAbilityHealAction::GetOperationName() const
{
	return TEXT("Heal");
}

FString UJargonAbilityHealAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d healing"), Healing);
}

FString UJargonAbilityHealAction::GetRulesText() const
{
	return FString::Printf(TEXT("Heal %d."), Healing);
}

UJargonAbilityShieldAction::UJargonAbilityShieldAction()
{
	EditorTitle = FText::FromString(TEXT("Apply Shield"));
}

void UJargonAbilityShieldAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Add(MakeDefaultTargetedValueEffect(AbilityDefinition, EJargonEffectOperation::ApplyShield, Shield));
}

FString UJargonAbilityShieldAction::GetOperationName() const
{
	return TEXT("Shield");
}

FString UJargonAbilityShieldAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d Shield"), Shield);
}

FString UJargonAbilityShieldAction::GetRulesText() const
{
	return FString::Printf(TEXT("Apply %d Shield."), Shield);
}

UJargonAbilityStatusAction::UJargonAbilityStatusAction()
{
	EditorTitle = FText::FromString(TEXT("Apply Status"));
}

void UJargonAbilityStatusAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect = MakeDefaultTargetedValueEffect(AbilityDefinition, EJargonEffectOperation::ApplyStatus, Amount);
	Effect.StatusEffectDefinition = StatusEffectDefinition;
	OutEffects.Add(Effect);
}

FString UJargonAbilityStatusAction::GetOperationName() const
{
	return TEXT("ApplyStatus");
}

FString UJargonAbilityStatusAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d %s Definition=%s"), Amount, *GetStatusDisplayName(StatusEffectDefinition), *GetNameSafe(StatusEffectDefinition.Get()));
}

FString UJargonAbilityStatusAction::GetRulesText() const
{
	return FString::Printf(TEXT("Apply %d %s."), Amount, *GetStatusDisplayName(StatusEffectDefinition));
}

UJargonAbilityCleanseStatusAction::UJargonAbilityCleanseStatusAction()
{
	EditorTitle = FText::FromString(TEXT("Cleanse Status"));
}

void UJargonAbilityCleanseStatusAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::CleanseStatus;
	Effect.StatusEffectDefinition = StatusEffectDefinition;
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityCleanseStatusAction::GetOperationName() const
{
	return TEXT("CleanseStatus");
}

FString UJargonAbilityCleanseStatusAction::GetPayloadSummary() const
{
	return StatusEffectDefinition
		? FString::Printf(TEXT("%s Definition=%s"), *GetStatusDisplayName(StatusEffectDefinition), *GetNameSafe(StatusEffectDefinition.Get()))
		: TEXT("All negative statuses");
}

FString UJargonAbilityCleanseStatusAction::GetRulesText() const
{
	return StatusEffectDefinition
		? FString::Printf(TEXT("Cleanse %s."), *GetStatusDisplayName(StatusEffectDefinition))
		: TEXT("Cleanse all negative statuses.");
}

UJargonAbilityMoveAction::UJargonAbilityMoveAction()
{
	EditorTitle = FText::FromString(TEXT("Move"));
}

void UJargonAbilityMoveAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::MoveSource;
	Effect.MoveDistance = FMath::Max(0, Distance);
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityMoveAction::GetOperationName() const
{
	return TEXT("Move");
}

FString UJargonAbilityMoveAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d tile%s"), Distance, *PluralSuffix(Distance));
}

FString UJargonAbilityMoveAction::GetRulesText() const
{
	return FString::Printf(TEXT("Move up to %d tile%s."), Distance, *PluralSuffix(Distance));
}

UJargonAbilityPushAction::UJargonAbilityPushAction()
{
	EditorTitle = FText::FromString(TEXT("Push"));
}

void UJargonAbilityPushAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::PushTarget;
	Effect.PushDistance = FMath::Max(0, Distance);
	Effect.CollisionDamage = FMath::Max(0, CollisionDamage);
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityPushAction::GetOperationName() const
{
	return TEXT("Push");
}

FString UJargonAbilityPushAction::GetPayloadSummary() const
{
	return CollisionDamage > 0
		? FString::Printf(TEXT("%d tile%s Collision=%d damage"), Distance, *PluralSuffix(Distance), CollisionDamage)
		: FString::Printf(TEXT("%d tile%s"), Distance, *PluralSuffix(Distance));
}

FString UJargonAbilityPushAction::GetRulesText() const
{
	return CollisionDamage > 0
		? FString::Printf(TEXT("Push %d tile%s. Collision deals %d damage."), Distance, *PluralSuffix(Distance), CollisionDamage)
		: FString::Printf(TEXT("Push %d tile%s."), Distance, *PluralSuffix(Distance));
}

UJargonAbilityPullAction::UJargonAbilityPullAction()
{
	EditorTitle = FText::FromString(TEXT("Pull"));
}

void UJargonAbilityPullAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::PullTarget;
	Effect.PullDistance = FMath::Max(0, Distance);
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityPullAction::GetOperationName() const
{
	return TEXT("Pull");
}

FString UJargonAbilityPullAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d tile%s"), Distance, *PluralSuffix(Distance));
}

FString UJargonAbilityPullAction::GetRulesText() const
{
	return FString::Printf(TEXT("Pull %d tile%s."), Distance, *PluralSuffix(Distance));
}

UJargonAbilitySummonAction::UJargonAbilitySummonAction()
{
	EditorTitle = FText::FromString(TEXT("Summon Unit"));
}

void UJargonAbilitySummonAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::SummonUnit;
	Effect.SummonedUnitDefinition = SummonedUnitDefinition;
	Effect.RuntimeSummonedUnitClass = RuntimeSummonedUnitClass;
	Effect.bSummonEntersWithAttackExhausted = bSummonEntersWithAttackExhausted;
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilitySummonAction::GetOperationName() const
{
	return TEXT("Summon");
}

FString UJargonAbilitySummonAction::GetPayloadSummary() const
{
	const FString SummonName = SummonedUnitDefinition && !SummonedUnitDefinition->DisplayName.IsEmpty()
		? SummonedUnitDefinition->DisplayName.ToString()
		: GetNameSafe(SummonedUnitDefinition.Get());
	return FString::Printf(
		TEXT("%s Runtime=%s AttackExhausted=%s"),
		SummonName.IsEmpty() ? TEXT("MissingDefinition") : *SummonName,
		*GetNameSafe(RuntimeSummonedUnitClass.Get()),
		*BoolText(bSummonEntersWithAttackExhausted));
}

FString UJargonAbilitySummonAction::GetRulesText() const
{
	const FString SummonName = SummonedUnitDefinition && !SummonedUnitDefinition->DisplayName.IsEmpty()
		? SummonedUnitDefinition->DisplayName.ToString()
		: TEXT("a unit");
	return FString::Printf(TEXT("Summon %s."), *SummonName);
}

UJargonAbilityPlaceTileEffectAction::UJargonAbilityPlaceTileEffectAction()
{
	EditorTitle = FText::FromString(TEXT("Place Tile Effect"));
}

void UJargonAbilityPlaceTileEffectAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::PlaceTileEffect;
	Effect.Value = 1;
	Effect.TileEffectDefinition = TileEffectDefinition;
	Effect.RuntimeTileEffectClass = RuntimeTileEffectClass;
	Effect.TileEffectCategory = TileEffectCategory;
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityPlaceTileEffectAction::GetOperationName() const
{
	return TEXT("PlaceTileEffect");
}

FString UJargonAbilityPlaceTileEffectAction::GetPayloadSummary() const
{
	const FString TileEffectName = TileEffectDefinition && !TileEffectDefinition->DisplayName.IsEmpty()
		? TileEffectDefinition->DisplayName.ToString()
		: GetNameSafe(TileEffectDefinition.Get());
	return FString::Printf(
		TEXT("%s Runtime=%s Category=%s"),
		TileEffectName.IsEmpty() ? TEXT("MissingDefinition") : *TileEffectName,
		*GetNameSafe(RuntimeTileEffectClass.Get()),
		*StaticEnum<ECardCategory>()->GetNameStringByValue(static_cast<int64>(TileEffectCategory)));
}

FString UJargonAbilityPlaceTileEffectAction::GetRulesText() const
{
	const FString TileEffectName = TileEffectDefinition && !TileEffectDefinition->DisplayName.IsEmpty()
		? TileEffectDefinition->DisplayName.ToString()
		: TEXT("a tile effect");
	return FString::Printf(TEXT("Place %s."), *TileEffectName);
}

UJargonAbilityDestroyTileEffectAction::UJargonAbilityDestroyTileEffectAction()
{
	EditorTitle = FText::FromString(TEXT("Destroy Tile Effect"));
}

void UJargonAbilityDestroyTileEffectAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::DestroyTileEffect;
	ApplyAbilityDefaultTargeting(AbilityDefinition, Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityDestroyTileEffectAction::GetOperationName() const
{
	return TEXT("DestroyTileEffect");
}

FString UJargonAbilityDestroyTileEffectAction::GetPayloadSummary() const
{
	return TEXT("Opposing tile effects");
}

FString UJargonAbilityDestroyTileEffectAction::GetRulesText() const
{
	return TEXT("Destroy opposing tile effects.");
}

UJargonAbilityDrawCardsAction::UJargonAbilityDrawCardsAction()
{
	EditorTitle = FText::FromString(TEXT("Draw Cards"));
}

void UJargonAbilityDrawCardsAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::DrawCards;
	Effect.Value = FMath::Max(0, Count);
	ApplySelfTargeting(Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityDrawCardsAction::GetOperationName() const
{
	return TEXT("Draw");
}

FString UJargonAbilityDrawCardsAction::GetDeliverySummary() const
{
	return TEXT("Self");
}

FString UJargonAbilityDrawCardsAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d card%s"), Count, *PluralSuffix(Count));
}

FString UJargonAbilityDrawCardsAction::GetRulesText() const
{
	return FString::Printf(TEXT("Draw %d card%s."), Count, *PluralSuffix(Count));
}

UJargonAbilityGainEnergyAction::UJargonAbilityGainEnergyAction()
{
	EditorTitle = FText::FromString(TEXT("Gain Energy"));
}

void UJargonAbilityGainEnergyAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::GainEnergy;
	Effect.Value = FMath::Max(0, Amount);
	ApplySelfTargeting(Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityGainEnergyAction::GetOperationName() const
{
	return TEXT("GainEnergy");
}

FString UJargonAbilityGainEnergyAction::GetDeliverySummary() const
{
	return TEXT("Self");
}

FString UJargonAbilityGainEnergyAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d Energy"), Amount);
}

FString UJargonAbilityGainEnergyAction::GetRulesText() const
{
	return FString::Printf(TEXT("Gain %d Energy."), Amount);
}

UJargonAbilityGainElementChargeAction::UJargonAbilityGainElementChargeAction()
{
	EditorTitle = FText::FromString(TEXT("Gain Element Charge"));
}

void UJargonAbilityGainElementChargeAction::BuildEffectSpecs(const UJargonAbilityDefinition* AbilityDefinition, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::GainElementCharge;
	Effect.Value = FMath::Max(0, Amount);
	Effect.ElementType = ElementType;
	ApplySelfTargeting(Effect);
	OutEffects.Add(Effect);
}

FString UJargonAbilityGainElementChargeAction::GetOperationName() const
{
	return TEXT("GainElementCharge");
}

FString UJargonAbilityGainElementChargeAction::GetDeliverySummary() const
{
	return TEXT("Self");
}

FString UJargonAbilityGainElementChargeAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d %s charge%s"), Amount, *ElementName(ElementType), *PluralSuffix(Amount));
}

FString UJargonAbilityGainElementChargeAction::GetRulesText() const
{
	return FString::Printf(TEXT("Gain %d %s charge%s."), Amount, *ElementName(ElementType), *PluralSuffix(Amount));
}

void UJargonAbilityDefinition::BuildEffectSpecs(TArray<FJargonEffectSpec>& OutEffects) const
{
	for (const TObjectPtr<UJargonAbilityAction>& Action : Actions)
	{
		if (Action)
		{
			Action->BuildEffectSpecs(this, OutEffects);
		}
	}
}

bool UJargonAbilityDefinition::IsValidDefinition() const
{
	if (DisplayName.IsEmpty() || Actions.Num() <= 0)
	{
		return false;
	}

	TArray<FJargonEffectSpec> BuiltEffects;
	BuildEffectSpecs(BuiltEffects);
	if (BuiltEffects.Num() <= 0)
	{
		return false;
	}

	for (const TObjectPtr<UJargonAbilityAction>& Action : Actions)
	{
		if (!Action)
		{
			return false;
		}
	}

	for (const FJargonEffectSpec& Effect : BuiltEffects)
	{
		if (!IsEffectSpecDefinitionValid(Effect))
		{
			return false;
		}
	}

	return true;
}

FString UJargonAbilityDefinition::GetAuditSummary() const
{
	TArray<FString> ActionSummaries;
	for (const TObjectPtr<UJargonAbilityAction>& Action : Actions)
	{
		ActionSummaries.Add(Action ? Action->GetActionSummary() : TEXT("NullAction"));
	}

	TArray<FJargonEffectSpec> BuiltEffects;
	BuildEffectSpecs(BuiltEffects);

	return FString::Printf(
		TEXT("DisplayName=%s Trigger=%s Targeting=[%s] Cue=[%s] Actions=%d Effects=%d IsValidDefinition=%s ActionSummaries=[%s]"),
		*DisplayName.ToString(),
		*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(ExpectedTrigger)),
		*TargetingProfile.GetSummary(),
		*CueDefinition.GetAuditSummary(),
		Actions.Num(),
		BuiltEffects.Num(),
		IsValidDefinition() ? TEXT("true") : TEXT("false"),
		*FString::Join(ActionSummaries, TEXT(" | ")));
}

FText UJargonAbilityDefinition::GetExecutionLabel() const
{
	const FText Fallback = DisplayName.IsEmpty() ? FText::FromString(GetNameSafe(this)) : DisplayName;
	return CueDefinition.GetLabelOrFallback(Fallback);
}

#if WITH_EDITOR
EDataValidationResult UJargonAbilityDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	bool bValid = true;

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is required."));
		bValid = false;
	}

	if (RulesText.IsEmpty())
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("RulesText is recommended so reusable abilities are readable in editor and future UI."));
	}

	if (Actions.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("At least one ability effect line is required."));
		bValid = false;
	}

	if (TargetingProfile.UsesChain() && TargetingProfile.ChainCount <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("TargetingProfile ChainEnemies requires ChainCount > 0."));
		bValid = false;
	}

	if (TargetingProfile.UsesRadius() && TargetingProfile.Radius <= 0)
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("TargetingProfile uses radius targeting but Radius <= 0."));
	}

	for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
	{
		const UJargonAbilityAction* Action = Actions[ActionIndex];
		const FString ActionLabel = FString::Printf(TEXT("Action %d"), ActionIndex);
		if (!Action)
		{
			JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("%s is null."), *ActionLabel));
			bValid = false;
			continue;
		}

		bValid &= Action->ValidateAction(this, ActionLabel, Context);
	}

	return bValid ? Result : EDataValidationResult::Invalid;
}
#endif
