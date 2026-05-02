#include "Data/CardScriptDefinition.h"

#include "Combat/Effects/JargonEffectTypes.h"
#include "Combat/Grid/Effects/AuraTileEffect.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Grid/Effects/TrapTileEffect.h"
#include "Combat/Units/SummonedBattleUnit.h"
#include "Data/CardDefinition.h"
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

	FString ElementName(EJargonElementType ElementType)
	{
		const UEnum* Enum = StaticEnum<EJargonElementType>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(ElementType)).ToString() : TEXT("Element");
	}

	FString PluralSuffix(int32 Value)
	{
		return FMath::Abs(Value) == 1 ? TEXT("") : TEXT("s");
	}

	FString MakeEditorTitleFromRules(FString RulesText)
	{
		RulesText.TrimStartAndEndInline();
		while (RulesText.EndsWith(TEXT(".")))
		{
			RulesText.LeftChopInline(1);
			RulesText.TrimEndInline();
		}
		return RulesText.IsEmpty() ? TEXT("Card Effect Line") : RulesText;
	}

	FString FormatKeywordSummary(const FString& Operation, const FString& Delivery, const FString& Payload)
	{
		return FString::Printf(TEXT("Operation=%s Delivery=%s Payload=%s"), *Operation, *Delivery, *Payload);
	}

	FString GetCardKeywordToken(EJargonCardKeyword Keyword)
	{
		const UEnum* Enum = StaticEnum<EJargonCardKeyword>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Keyword)) : TEXT("Custom");
	}

	FString UnitDeliverySummary(int32 Radius, const TCHAR* SingleTarget, const TCHAR* AreaTarget)
	{
		return Radius > 0
			? FString::Printf(TEXT("%s Radius=%d"), AreaTarget, Radius)
			: FString(SingleTarget);
	}

	FString TileDeliverySummary(int32 Radius, const TCHAR* SingleTarget, const TCHAR* AreaTarget)
	{
		return Radius > 0
			? FString::Printf(TEXT("%s Radius=%d"), AreaTarget, Radius)
			: FString(SingleTarget);
	}

	FJargonEffectSpec MakeUnitEffect(
		EJargonEffectOperation Operation,
		int32 Value,
		int32 Radius,
		EJargonEffectTargetFilter TargetFilter)
	{
		FJargonEffectSpec Effect;
		Effect.Operation = Operation;
		Effect.Value = FMath::Max(0, Value);
		Effect.Radius = FMath::Max(0, Radius);
		Effect.Delivery = Effect.Radius > 0 ? EJargonEffectDelivery::UnitsInRadius : EJargonEffectDelivery::ExplicitUnit;
		Effect.TargetFilter = TargetFilter;
		return Effect;
	}

	FString GetStatusKindName(EJargonStatusEffectKind StatusKind)
	{
		const UEnum* Enum = StaticEnum<EJargonStatusEffectKind>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(StatusKind)).ToString() : TEXT("Status");
	}

	FString GetStatusName(const UJargonStatusEffectDefinition* StatusEffectDefinition)
	{
		if (StatusEffectDefinition && !StatusEffectDefinition->DisplayName.IsEmpty())
		{
			return StatusEffectDefinition->DisplayName.ToString();
		}

		return StatusEffectDefinition
			? GetStatusKindName(StatusEffectDefinition->StatusKind)
			: TEXT("Status");
	}

	FString GetStatusValueLabel(const UJargonStatusEffectDefinition* StatusEffectDefinition)
	{
		if (StatusEffectDefinition && !StatusEffectDefinition->ValueLabel.IsEmpty())
		{
			return StatusEffectDefinition->ValueLabel.ToString();
		}

		switch (StatusEffectDefinition ? StatusEffectDefinition->StatusKind : EJargonStatusEffectKind::None)
		{
		case EJargonStatusEffectKind::Burn:
			return TEXT("stack");
		case EJargonStatusEffectKind::Vulnerable:
			return TEXT("next-hit bonus damage");
		case EJargonStatusEffectKind::Stun:
		case EJargonStatusEffectKind::Freeze:
		case EJargonStatusEffectKind::Root:
		default:
			return TEXT("turn");
		}
	}

	EJargonEffectOperation GetChainOperation(EJargonCardChainActionType ChainType)
	{
		switch (ChainType)
		{
		case EJargonCardChainActionType::Damage:
			return EJargonEffectOperation::DealDamage;
		case EJargonCardChainActionType::Heal:
			return EJargonEffectOperation::Heal;
		case EJargonCardChainActionType::Stun:
			return EJargonEffectOperation::ApplyStatus;
		default:
			return EJargonEffectOperation::None;
		}
	}

	FString GetChainName(EJargonCardChainActionType ChainType)
	{
		const UEnum* Enum = StaticEnum<EJargonCardChainActionType>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(ChainType)).ToString() : TEXT("Chain");
	}

	bool IsFriendlyChain(EJargonCardChainActionType ChainType)
	{
		return ChainType == EJargonCardChainActionType::Heal;
	}

	bool IsPositiveValue(int32 Value)
	{
		return Value > 0;
	}

#if WITH_EDITOR
	bool ValidatePositiveAmount(
		const UCardDefinition* Card,
		const FString& ActionLabel,
		const FString& FieldName,
		int32 Value,
		FDataValidationContext& Context)
	{
		if (Value > 0)
		{
			return true;
		}

		JargonDataAssetValidation::AddError(
			Context,
			Card,
			FString::Printf(TEXT("%s requires payload field %s > 0."), *ActionLabel, *FieldName));
		return false;
	}
#endif
}

UJargonCardAction::UJargonCardAction()
{
	EditorTitle = FText::FromString(TEXT("Card Effect Line"));
}

void UJargonCardAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
}

bool UJargonCardAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	TArray<FJargonEffectSpec> Effects;
	BuildEffectSpecs(nullptr, Effects);
	for (const FJargonEffectSpec& Effect : Effects)
	{
		if (Effect.Operation == Operation)
		{
			return true;
		}
	}
	return false;
}

EJargonCardKeyword UJargonCardAction::GetKeyword() const
{
	return EJargonCardKeyword::None;
}

FString UJargonCardAction::GetKeywordName() const
{
	const FString KeywordName = GetCardKeywordToken(GetKeyword());
	return KeywordName == TEXT("None") && GetClass() ? GetClass()->GetName() : KeywordName;
}

FString UJargonCardAction::GetDeliverySummary() const
{
	return TEXT("Custom");
}

FString UJargonCardAction::GetPayloadSummary() const
{
	return TEXT("None");
}

FString UJargonCardAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardAction::GetRulesText() const
{
	return GetActionSummary();
}

void UJargonCardAction::RefreshEditorTitle()
{
	EditorTitle = FText::FromString(MakeEditorTitleFromRules(GetActionSummary()));
}

void UJargonCardAction::PostLoad()
{
	Super::PostLoad();
	RefreshEditorTitle();
}

#if WITH_EDITOR
void UJargonCardAction::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshEditorTitle();
}

bool UJargonCardAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return true;
}
#endif

UJargonCardDamageAction::UJargonCardDamageAction()
{
	EditorTitle = FText::FromString(TEXT("Deal Damage"));
}

void UJargonCardDamageAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Add(MakeUnitEffect(EJargonEffectOperation::DealDamage, Damage, Radius, EJargonEffectTargetFilter::EnemyToSource));
}

bool UJargonCardDamageAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::DealDamage;
}

EJargonCardKeyword UJargonCardDamageAction::GetKeyword() const
{
	return EJargonCardKeyword::Damage;
}

FString UJargonCardDamageAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardDamageAction::GetDeliverySummary() const
{
	return UnitDeliverySummary(Radius, TEXT("Single Enemy"), TEXT("AOE Enemies"));
}

FString UJargonCardDamageAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d damage"), Damage);
}

FString UJargonCardDamageAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardDamageAction::GetRulesText() const
{
	return Radius > 0
		? FString::Printf(TEXT("Deal %d damage in radius %d."), Damage, Radius)
		: FString::Printf(TEXT("Deal %d damage."), Damage);
}

#if WITH_EDITOR
bool UJargonCardDamageAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Damage"), Damage, Context);
}
#endif

UJargonCardHealAction::UJargonCardHealAction()
{
	EditorTitle = FText::FromString(TEXT("Heal"));
}

void UJargonCardHealAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Add(MakeUnitEffect(EJargonEffectOperation::Heal, Healing, Radius, EJargonEffectTargetFilter::FriendlyToSource));
}

bool UJargonCardHealAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::Heal;
}

EJargonCardKeyword UJargonCardHealAction::GetKeyword() const
{
	return EJargonCardKeyword::Heal;
}

FString UJargonCardHealAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardHealAction::GetDeliverySummary() const
{
	return UnitDeliverySummary(Radius, TEXT("Single Ally"), TEXT("AOE Allies"));
}

FString UJargonCardHealAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d healing"), Healing);
}

FString UJargonCardHealAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardHealAction::GetRulesText() const
{
	return Radius > 0
		? FString::Printf(TEXT("Heal %d in radius %d."), Healing, Radius)
		: FString::Printf(TEXT("Heal %d."), Healing);
}

#if WITH_EDITOR
bool UJargonCardHealAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Healing"), Healing, Context);
}
#endif

UJargonCardShieldAction::UJargonCardShieldAction()
{
	EditorTitle = FText::FromString(TEXT("Apply Shield"));
}

void UJargonCardShieldAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Add(MakeUnitEffect(EJargonEffectOperation::ApplyShield, Shield, Radius, EJargonEffectTargetFilter::FriendlyToSource));
}

bool UJargonCardShieldAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::ApplyShield;
}

EJargonCardKeyword UJargonCardShieldAction::GetKeyword() const
{
	return EJargonCardKeyword::Shield;
}

FString UJargonCardShieldAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardShieldAction::GetDeliverySummary() const
{
	return UnitDeliverySummary(Radius, TEXT("Single Ally"), TEXT("AOE Allies"));
}

FString UJargonCardShieldAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d Shield"), Shield);
}

FString UJargonCardShieldAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardShieldAction::GetRulesText() const
{
	return Radius > 0
		? FString::Printf(TEXT("Apply %d Shield in radius %d."), Shield, Radius)
		: FString::Printf(TEXT("Apply %d Shield."), Shield);
}

#if WITH_EDITOR
bool UJargonCardShieldAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Shield"), Shield, Context);
}
#endif

UJargonCardStatusAction::UJargonCardStatusAction()
{
	EditorTitle = FText::FromString(TEXT("Apply Status"));
}

void UJargonCardStatusAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect = MakeUnitEffect(
		EJargonEffectOperation::ApplyStatus,
		Amount,
		Radius,
		EJargonEffectTargetFilter::EnemyToSource);
	Effect.StatusEffectDefinition = StatusEffectDefinition;
	OutEffects.Add(Effect);
}

bool UJargonCardStatusAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::ApplyStatus;
}

EJargonCardKeyword UJargonCardStatusAction::GetKeyword() const
{
	return EJargonCardKeyword::Status;
}

FString UJargonCardStatusAction::GetKeywordName() const
{
	return GetStatusName(StatusEffectDefinition).Replace(TEXT(" "), TEXT(""));
}

FString UJargonCardStatusAction::GetDeliverySummary() const
{
	return UnitDeliverySummary(Radius, TEXT("Single Enemy"), TEXT("AOE Enemies"));
}

FString UJargonCardStatusAction::GetPayloadSummary() const
{
	const FString StatusName = GetStatusName(StatusEffectDefinition);
	const FString ValueLabel = GetStatusValueLabel(StatusEffectDefinition);
	const FString DefinitionName = StatusEffectDefinition ? GetNameSafe(StatusEffectDefinition.Get()) : TEXT("MissingDefinition");
	return FString::Printf(TEXT("%d %s %s Definition=%s"), Amount, *StatusName, *ValueLabel, *DefinitionName);
}

FString UJargonCardStatusAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardStatusAction::GetRulesText() const
{
	const FString StatusName = GetStatusName(StatusEffectDefinition);
	const FString ValueLabel = GetStatusValueLabel(StatusEffectDefinition);
	const FString ValueSuffix = Amount == 1 || ValueLabel.Contains(TEXT("damage"))
		? ValueLabel
		: ValueLabel + TEXT("s");
	return Radius > 0
		? FString::Printf(TEXT("Apply %d %s %s in radius %d."), Amount, *StatusName, *ValueSuffix, Radius)
		: FString::Printf(TEXT("Apply %d %s %s."), Amount, *StatusName, *ValueSuffix);
}

#if WITH_EDITOR
bool UJargonCardStatusAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	bool bValid = ValidatePositiveAmount(Card, ActionLabel, TEXT("Amount"), Amount, Context);

	if (!StatusEffectDefinition)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Card,
			FString::Printf(TEXT("%s requires StatusEffectDefinition payload."), *ActionLabel));
		bValid = false;
	}
	else if (!StatusEffectDefinition->IsValidDefinition())
	{
		JargonDataAssetValidation::AddError(
			Context,
			Card,
			FString::Printf(TEXT("%s references invalid StatusEffectDefinition payload '%s'."), *ActionLabel, *GetPathNameSafe(StatusEffectDefinition.Get())));
		bValid = false;
	}

	return bValid;
}
#endif

UJargonCardMoveSelfAction::UJargonCardMoveSelfAction()
{
	EditorTitle = FText::FromString(TEXT("Move Self"));
}

void UJargonCardMoveSelfAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::MoveSource;
	Effect.Delivery = EJargonEffectDelivery::ExplicitTile;
	Effect.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
	Effect.MoveDistance = FMath::Max(0, Distance);
	OutEffects.Add(Effect);
}

bool UJargonCardMoveSelfAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::MoveSource;
}

EJargonCardKeyword UJargonCardMoveSelfAction::GetKeyword() const
{
	return EJargonCardKeyword::Move;
}

FString UJargonCardMoveSelfAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardMoveSelfAction::GetDeliverySummary() const
{
	return TEXT("Self To Tile");
}

FString UJargonCardMoveSelfAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d tile%s"), Distance, *PluralSuffix(Distance));
}

FString UJargonCardMoveSelfAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardMoveSelfAction::GetRulesText() const
{
	return FString::Printf(TEXT("Move up to %d tile%s."), Distance, *PluralSuffix(Distance));
}

#if WITH_EDITOR
bool UJargonCardMoveSelfAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Distance"), Distance, Context);
}
#endif

UJargonCardPushAction::UJargonCardPushAction()
{
	EditorTitle = FText::FromString(TEXT("Push Target"));
}

void UJargonCardPushAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::PushTarget;
	Effect.Delivery = EJargonEffectDelivery::ExplicitUnit;
	Effect.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
	Effect.PushDistance = FMath::Max(0, Distance);
	Effect.CollisionDamage = FMath::Max(0, CollisionDamage);
	OutEffects.Add(Effect);
}

bool UJargonCardPushAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::PushTarget;
}

EJargonCardKeyword UJargonCardPushAction::GetKeyword() const
{
	return EJargonCardKeyword::Push;
}

FString UJargonCardPushAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardPushAction::GetDeliverySummary() const
{
	return TEXT("Single Enemy");
}

FString UJargonCardPushAction::GetPayloadSummary() const
{
	return CollisionDamage > 0
		? FString::Printf(TEXT("%d tile%s Collision=%d damage"), Distance, *PluralSuffix(Distance), CollisionDamage)
		: FString::Printf(TEXT("%d tile%s"), Distance, *PluralSuffix(Distance));
}

FString UJargonCardPushAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardPushAction::GetRulesText() const
{
	return CollisionDamage > 0
		? FString::Printf(TEXT("Push %d tile%s. Collision deals %d damage."), Distance, *PluralSuffix(Distance), CollisionDamage)
		: FString::Printf(TEXT("Push %d tile%s."), Distance, *PluralSuffix(Distance));
}

#if WITH_EDITOR
bool UJargonCardPushAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Distance"), Distance, Context);
}
#endif

UJargonCardPullAction::UJargonCardPullAction()
{
	EditorTitle = FText::FromString(TEXT("Pull Target"));
}

void UJargonCardPullAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::PullTarget;
	Effect.Delivery = EJargonEffectDelivery::ExplicitUnit;
	Effect.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
	Effect.PullDistance = FMath::Max(0, Distance);
	OutEffects.Add(Effect);
}

bool UJargonCardPullAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::PullTarget;
}

EJargonCardKeyword UJargonCardPullAction::GetKeyword() const
{
	return EJargonCardKeyword::Pull;
}

FString UJargonCardPullAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardPullAction::GetDeliverySummary() const
{
	return TEXT("Single Enemy");
}

FString UJargonCardPullAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d tile%s"), Distance, *PluralSuffix(Distance));
}

FString UJargonCardPullAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardPullAction::GetRulesText() const
{
	return FString::Printf(TEXT("Pull %d tile%s."), Distance, *PluralSuffix(Distance));
}

#if WITH_EDITOR
bool UJargonCardPullAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Distance"), Distance, Context);
}
#endif

UJargonCardSummonAction::UJargonCardSummonAction()
{
	EditorTitle = FText::FromString(TEXT("Summon Unit"));
}

void UJargonCardSummonAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::SummonUnit;
	Effect.Delivery = EJargonEffectDelivery::ExplicitTile;
	Effect.TargetFilter = EJargonEffectTargetFilter::None;
	Effect.SummonedUnitDefinition = SummonedUnitDefinition;
	Effect.RuntimeSummonedUnitClass = RuntimeSummonedUnitClass;
	Effect.bSummonEntersWithAttackExhausted = bSummonEntersWithAttackExhausted;
	OutEffects.Add(Effect);
}

bool UJargonCardSummonAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::SummonUnit;
}

EJargonCardKeyword UJargonCardSummonAction::GetKeyword() const
{
	return EJargonCardKeyword::Summon;
}

FString UJargonCardSummonAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardSummonAction::GetDeliverySummary() const
{
	return TEXT("Target Tile");
}

FString UJargonCardSummonAction::GetPayloadSummary() const
{
	const FString SummonName = SummonedUnitDefinition && !SummonedUnitDefinition->DisplayName.IsEmpty()
		? SummonedUnitDefinition->DisplayName.ToString()
		: GetNameSafe(SummonedUnitDefinition.Get());
	const FString PayloadName = SummonName.IsEmpty() ? TEXT("MissingDefinition") : SummonName;
	return FString::Printf(
		TEXT("%s Runtime=%s AttackExhausted=%s"),
		*PayloadName,
		*GetNameSafe(RuntimeSummonedUnitClass.Get()),
		*BoolText(bSummonEntersWithAttackExhausted));
}

FString UJargonCardSummonAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardSummonAction::GetRulesText() const
{
	const FString SummonName = SummonedUnitDefinition ? SummonedUnitDefinition->DisplayName.ToString() : TEXT("a unit");
	return FString::Printf(TEXT("Summon %s."), *SummonName);
}

#if WITH_EDITOR
bool UJargonCardSummonAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	bool bValid = true;
	if (!SummonedUnitDefinition)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("%s requires SummonedUnitDefinition payload."), *ActionLabel));
		bValid = false;
	}

	if (!RuntimeSummonedUnitClass)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("%s requires RuntimeSummonedUnitClass payload."), *ActionLabel));
		bValid = false;
	}
	else if (!RuntimeSummonedUnitClass->IsChildOf(ASummonedBattleUnit::StaticClass()))
	{
		JargonDataAssetValidation::AddError(
			Context,
			Card,
			FString::Printf(TEXT("%s RuntimeSummonedUnitClass must be a child of ASummonedBattleUnit."), *ActionLabel));
		bValid = false;
	}

	return bValid;
}
#endif

UJargonCardPlaceTileEffectAction::UJargonCardPlaceTileEffectAction()
{
	EditorTitle = FText::FromString(TEXT("Place Tile Effect"));
}

void UJargonCardPlaceTileEffectAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::PlaceTileEffect;
	Effect.Delivery = EJargonEffectDelivery::ExplicitTile;
	Effect.TargetFilter = EJargonEffectTargetFilter::None;
	Effect.TileEffectDefinition = TileEffectDefinition;
	Effect.RuntimeTileEffectClass = RuntimeTileEffectClass;
	Effect.TileEffectCategory = Card ? Card->Category : ECardCategory::Trap;
	OutEffects.Add(Effect);
}

bool UJargonCardPlaceTileEffectAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::PlaceTileEffect;
}

EJargonCardKeyword UJargonCardPlaceTileEffectAction::GetKeyword() const
{
	return EJargonCardKeyword::PlaceTileEffect;
}

FString UJargonCardPlaceTileEffectAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardPlaceTileEffectAction::GetDeliverySummary() const
{
	return TEXT("Target Tile");
}

FString UJargonCardPlaceTileEffectAction::GetPayloadSummary() const
{
	const FString TileEffectName = TileEffectDefinition && !TileEffectDefinition->DisplayName.IsEmpty()
		? TileEffectDefinition->DisplayName.ToString()
		: GetNameSafe(TileEffectDefinition.Get());
	const FString PayloadName = TileEffectName.IsEmpty() ? TEXT("MissingDefinition") : TileEffectName;
	return FString::Printf(TEXT("%s Runtime=%s"), *PayloadName, *GetNameSafe(RuntimeTileEffectClass.Get()));
}

FString UJargonCardPlaceTileEffectAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardPlaceTileEffectAction::GetRulesText() const
{
	const FString TileEffectName = TileEffectDefinition ? TileEffectDefinition->DisplayName.ToString() : TEXT("a tile effect");
	return FString::Printf(TEXT("Place %s."), *TileEffectName);
}

#if WITH_EDITOR
bool UJargonCardPlaceTileEffectAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	bool bValid = true;
	if (!TileEffectDefinition)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("%s requires TileEffectDefinition payload."), *ActionLabel));
		bValid = false;
	}

	if (!RuntimeTileEffectClass)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("%s requires RuntimeTileEffectClass payload."), *ActionLabel));
		bValid = false;
	}
	else if (TileEffectDefinition)
	{
		const bool bDefinitionIsTrap = TileEffectDefinition->TileEffectCategory == ECardCategory::Trap;
		const bool bDefinitionIsAura = TileEffectDefinition->TileEffectCategory == ECardCategory::Aura;
		if (bDefinitionIsTrap && !RuntimeTileEffectClass->IsChildOf(ATrapTileEffect::StaticClass()))
		{
			JargonDataAssetValidation::AddError(
				Context,
				Card,
				FString::Printf(TEXT("%s RuntimeTileEffectClass must be a child of ATrapTileEffect for trap definitions."), *ActionLabel));
			bValid = false;
		}
		else if (bDefinitionIsAura && !RuntimeTileEffectClass->IsChildOf(AAuraTileEffect::StaticClass()))
		{
			JargonDataAssetValidation::AddError(
				Context,
				Card,
				FString::Printf(TEXT("%s RuntimeTileEffectClass must be a child of AAuraTileEffect for aura definitions."), *ActionLabel));
			bValid = false;
		}
	}

	return bValid;
}
#endif

UJargonCardDestroyTileEffectAction::UJargonCardDestroyTileEffectAction()
{
	EditorTitle = FText::FromString(TEXT("Destroy Tile Effect"));
}

void UJargonCardDestroyTileEffectAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::DestroyTileEffect;
	Effect.Delivery = Radius > 0 ? EJargonEffectDelivery::TilesInRadius : EJargonEffectDelivery::ExplicitTile;
	Effect.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
	Effect.Radius = FMath::Max(0, Radius);
	OutEffects.Add(Effect);
}

bool UJargonCardDestroyTileEffectAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::DestroyTileEffect;
}

EJargonCardKeyword UJargonCardDestroyTileEffectAction::GetKeyword() const
{
	return EJargonCardKeyword::DestroyTileEffect;
}

FString UJargonCardDestroyTileEffectAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardDestroyTileEffectAction::GetDeliverySummary() const
{
	return TileDeliverySummary(Radius, TEXT("Target Tile"), TEXT("AOE Tiles"));
}

FString UJargonCardDestroyTileEffectAction::GetPayloadSummary() const
{
	return TEXT("Opposing tile effects");
}

FString UJargonCardDestroyTileEffectAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardDestroyTileEffectAction::GetRulesText() const
{
	return Radius > 0
		? FString::Printf(TEXT("Destroy opposing tile effects in radius %d."), Radius)
		: TEXT("Destroy an opposing tile effect.");
}

UJargonCardDrawCardsAction::UJargonCardDrawCardsAction()
{
	EditorTitle = FText::FromString(TEXT("Draw Cards"));
}

void UJargonCardDrawCardsAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::DrawCards;
	Effect.Delivery = EJargonEffectDelivery::Self;
	Effect.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
	Effect.Value = FMath::Max(0, Count);
	OutEffects.Add(Effect);
}

bool UJargonCardDrawCardsAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::DrawCards;
}

EJargonCardKeyword UJargonCardDrawCardsAction::GetKeyword() const
{
	return EJargonCardKeyword::Draw;
}

FString UJargonCardDrawCardsAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardDrawCardsAction::GetDeliverySummary() const
{
	return TEXT("Self");
}

FString UJargonCardDrawCardsAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d card%s"), Count, *PluralSuffix(Count));
}

FString UJargonCardDrawCardsAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardDrawCardsAction::GetRulesText() const
{
	return FString::Printf(TEXT("Draw %d card%s."), Count, *PluralSuffix(Count));
}

#if WITH_EDITOR
bool UJargonCardDrawCardsAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Count"), Count, Context);
}
#endif

UJargonCardGainEnergyAction::UJargonCardGainEnergyAction()
{
	EditorTitle = FText::FromString(TEXT("Gain Energy"));
}

void UJargonCardGainEnergyAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::GainEnergy;
	Effect.Delivery = EJargonEffectDelivery::Self;
	Effect.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
	Effect.Value = FMath::Max(0, Amount);
	OutEffects.Add(Effect);
}

bool UJargonCardGainEnergyAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::GainEnergy;
}

EJargonCardKeyword UJargonCardGainEnergyAction::GetKeyword() const
{
	return EJargonCardKeyword::GainEnergy;
}

FString UJargonCardGainEnergyAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardGainEnergyAction::GetDeliverySummary() const
{
	return TEXT("Self");
}

FString UJargonCardGainEnergyAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d Energy"), Amount);
}

FString UJargonCardGainEnergyAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardGainEnergyAction::GetRulesText() const
{
	return FString::Printf(TEXT("Gain %d Energy."), Amount);
}

#if WITH_EDITOR
bool UJargonCardGainEnergyAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	return ValidatePositiveAmount(Card, ActionLabel, TEXT("Amount"), Amount, Context);
}
#endif

UJargonCardGainElementChargeAction::UJargonCardGainElementChargeAction()
{
	EditorTitle = FText::FromString(TEXT("Gain Element Charge"));
}

void UJargonCardGainElementChargeAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = EJargonEffectOperation::GainElementCharge;
	Effect.Delivery = EJargonEffectDelivery::Self;
	Effect.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
	Effect.Value = FMath::Max(0, Amount);
	Effect.ElementType = ElementType;
	OutEffects.Add(Effect);
}

bool UJargonCardGainElementChargeAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == EJargonEffectOperation::GainElementCharge;
}

EJargonCardKeyword UJargonCardGainElementChargeAction::GetKeyword() const
{
	return EJargonCardKeyword::GainElement;
}

FString UJargonCardGainElementChargeAction::GetKeywordName() const
{
	return GetCardKeywordToken(GetKeyword());
}

FString UJargonCardGainElementChargeAction::GetDeliverySummary() const
{
	return TEXT("Self");
}

FString UJargonCardGainElementChargeAction::GetPayloadSummary() const
{
	return FString::Printf(TEXT("%d %s charge%s"), Amount, *ElementName(ElementType), *PluralSuffix(Amount));
}

FString UJargonCardGainElementChargeAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardGainElementChargeAction::GetRulesText() const
{
	return FString::Printf(TEXT("Gain %d %s charge%s."), Amount, *ElementName(ElementType), *PluralSuffix(Amount));
}

#if WITH_EDITOR
bool UJargonCardGainElementChargeAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	bool bValid = ValidatePositiveAmount(Card, ActionLabel, TEXT("Amount"), Amount, Context);
	if (ElementType == EJargonElementType::None)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("%s requires ElementType payload other than None."), *ActionLabel));
		bValid = false;
	}
	return bValid;
}
#endif

UJargonCardChainAction::UJargonCardChainAction()
{
	EditorTitle = FText::FromString(TEXT("Chain"));
}

void UJargonCardChainAction::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	FJargonEffectSpec Effect;
	Effect.Operation = GetChainOperation(ChainType);
	Effect.Delivery = EJargonEffectDelivery::ChainUnits;
	Effect.TargetFilter = IsFriendlyChain(ChainType) ? EJargonEffectTargetFilter::FriendlyToSource : EJargonEffectTargetFilter::EnemyToSource;
	Effect.Value = FMath::Max(0, Amount);
	Effect.ChainCount = FMath::Max(1, ChainCount);
	Effect.Radius = FMath::Max(0, Radius);
	if (ChainType == EJargonCardChainActionType::Stun)
	{
		Effect.StatusEffectDefinition = StatusEffectDefinition;
	}
	OutEffects.Add(Effect);
}

bool UJargonCardChainAction::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	return Operation == GetChainOperation(ChainType);
}

EJargonCardKeyword UJargonCardChainAction::GetKeyword() const
{
	return EJargonCardKeyword::Chain;
}

FString UJargonCardChainAction::GetKeywordName() const
{
	const FString PayloadName = ChainType == EJargonCardChainActionType::Stun
		? GetStatusName(StatusEffectDefinition)
		: GetChainName(ChainType);
	return FString::Printf(TEXT("Chain%s"), *PayloadName.Replace(TEXT(" "), TEXT("")));
}

FString UJargonCardChainAction::GetDeliverySummary() const
{
	return FString::Printf(TEXT("Chain Units Count=%d Radius=%d"), ChainCount, Radius);
}

FString UJargonCardChainAction::GetPayloadSummary() const
{
	FString PayloadName = TEXT("damage");
	if (ChainType == EJargonCardChainActionType::Stun)
	{
		PayloadName = GetStatusName(StatusEffectDefinition).ToLower();
	}
	else if (ChainType != EJargonCardChainActionType::Damage)
	{
		PayloadName = GetChainName(ChainType).ToLower();
	}
	return FString::Printf(TEXT("%d %s"), Amount, *PayloadName);
}

FString UJargonCardChainAction::GetActionSummary() const
{
	return FormatKeywordSummary(GetKeywordName(), GetDeliverySummary(), GetPayloadSummary());
}

FString UJargonCardChainAction::GetRulesText() const
{
	const FString ChainPayload = ChainType == EJargonCardChainActionType::Damage
		? TEXT("damage")
		: (ChainType == EJargonCardChainActionType::Stun
			? GetStatusName(StatusEffectDefinition).ToLower()
			: GetChainName(ChainType).ToLower());
	return FString::Printf(
		TEXT("Chain %d %s to up to %d targets within radius %d."),
		Amount,
		*ChainPayload,
		ChainCount,
		Radius);
}

#if WITH_EDITOR
bool UJargonCardChainAction::ValidateAction(const UCardDefinition* Card, const FString& ActionLabel, FDataValidationContext& Context) const
{
	bool bValid = ValidatePositiveAmount(Card, ActionLabel, TEXT("Amount"), Amount, Context);
	bValid &= ValidatePositiveAmount(Card, ActionLabel, TEXT("ChainCount"), ChainCount, Context);
	bValid &= ValidatePositiveAmount(Card, ActionLabel, TEXT("Radius"), Radius, Context);
	if (ChainType == EJargonCardChainActionType::Stun)
	{
		if (!StatusEffectDefinition)
		{
			JargonDataAssetValidation::AddError(
				Context,
				Card,
				FString::Printf(TEXT("%s requires StatusEffectDefinition payload for a status chain."), *ActionLabel));
			bValid = false;
		}
		else if (!StatusEffectDefinition->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(
				Context,
				Card,
				FString::Printf(TEXT("%s references invalid StatusEffectDefinition payload '%s'."), *ActionLabel, *GetPathNameSafe(StatusEffectDefinition.Get())));
			bValid = false;
		}
	}
	return bValid;
}
#endif

bool FJargonCardElementalBonusScript::HasAnyActions() const
{
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			return true;
		}
	}
	return false;
}

void FJargonCardElementalBonusScript::BuildEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			Action->BuildEffectSpecs(Card, OutEffects);
		}
	}
}

bool FJargonCardElementalBonusScript::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	for (const TObjectPtr<UJargonCardAction>& ActionPtr : Actions)
	{
		const UJargonCardAction* Action = ActionPtr.Get();
		if (Action && Action->HasRuntimeOperation(Operation))
		{
			return true;
		}
	}
	return false;
}

FString FJargonCardElementalBonusScript::GetBonusSummary() const
{
	TArray<FString> ActionSummaries;
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		ActionSummaries.Add(Action ? Action->GetActionSummary() : TEXT("NullEffectLine"));
	}

	return FString::Printf(
		TEXT("Bonus=%s Required=%d Spend=%s EffectLines=[%s]"),
		*ElementName(ElementType),
		RequiredCharges,
		*BoolText(bSpendCharges),
		ActionSummaries.Num() > 0 ? *FString::Join(ActionSummaries, TEXT("; ")) : TEXT("None"));
}

FString FJargonCardElementalBonusScript::GetRulesText() const
{
	TArray<FString> ActionRules;
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			ActionRules.Add(Action->GetRulesText());
		}
	}

	return FString::Printf(
		TEXT("%s %d %s charge%s: %s"),
		bSpendCharges ? TEXT("Spend") : TEXT("If you have"),
		RequiredCharges,
		*ElementName(ElementType),
		*PluralSuffix(RequiredCharges),
		ActionRules.Num() > 0 ? *FString::Join(ActionRules, TEXT(" ")) : TEXT("No bonus effect lines."));
}

void FJargonCardElementalBonusScript::RefreshEditorTitle()
{
	EditorTitle = FText::FromString(MakeEditorTitleFromRules(GetBonusSummary()));
}

#if WITH_EDITOR
bool FJargonCardElementalBonusScript::ValidateBonus(const UCardDefinition* Card, int32 BonusIndex, FDataValidationContext& Context) const
{
	bool bValid = true;

	if (ElementType == EJargonElementType::None)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("Elemental bonus %d requires ElementType other than None."), BonusIndex));
		bValid = false;
	}

	if (RequiredCharges <= 0)
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("Elemental bonus %d requires RequiredCharges > 0."), BonusIndex));
		bValid = false;
	}

	if (!HasAnyActions())
	{
		JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("Elemental bonus %d has no effect-line entries."), BonusIndex));
		bValid = false;
	}

	for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
	{
		const UJargonCardAction* Action = Actions[ActionIndex].Get();
		if (!Action)
		{
			JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("Elemental bonus %d effect line %d is null."), BonusIndex, ActionIndex));
			bValid = false;
			continue;
		}

		bValid &= Action->ValidateAction(
			Card,
				FString::Printf(TEXT("Elemental bonus %d effect line %d (%s)"), BonusIndex, ActionIndex, *Action->GetActionSummary()),
			Context);
	}

	return bValid;
}
#endif

bool UJargonCardScript::HasAnyActions() const
{
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			return true;
		}
	}
	return false;
}

void UJargonCardScript::BuildBaseEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects) const
{
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			Action->BuildEffectSpecs(Card, OutEffects);
		}
	}
}

bool UJargonCardScript::BuildElementalBonusEffectSpecs(const UCardDefinition* Card, int32 BonusIndex, TArray<FJargonEffectSpec>& OutEffects) const
{
	if (!ElementalBonuses.IsValidIndex(BonusIndex))
	{
		return false;
	}

	ElementalBonuses[BonusIndex].BuildEffectSpecs(Card, OutEffects);
	return OutEffects.Num() > 0;
}

bool UJargonCardScript::HasRuntimeOperation(EJargonEffectOperation Operation) const
{
	for (const TObjectPtr<UJargonCardAction>& ActionPtr : Actions)
	{
		const UJargonCardAction* Action = ActionPtr.Get();
		if (Action && Action->HasRuntimeOperation(Operation))
		{
			return true;
		}
	}

	for (const FJargonCardElementalBonusScript& BonusScript : ElementalBonuses)
	{
		if (BonusScript.HasRuntimeOperation(Operation))
		{
			return true;
		}
	}

	return false;
}

FString UJargonCardScript::GetScriptSummary() const
{
	TArray<FString> ActionSummaries;
	for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
	{
		const UJargonCardAction* Action = Actions[ActionIndex].Get();
		ActionSummaries.Add(FString::Printf(
			TEXT("Effect line %d: %s"),
			ActionIndex,
			Action ? *Action->GetActionSummary() : TEXT("NullEffectLine")));
	}

	TArray<FString> BonusSummaries;
	for (int32 BonusIndex = 0; BonusIndex < ElementalBonuses.Num(); ++BonusIndex)
	{
		BonusSummaries.Add(FString::Printf(TEXT("Bonus %d: %s"), BonusIndex, *ElementalBonuses[BonusIndex].GetBonusSummary()));
	}

	return FString::Printf(
		TEXT("EffectLines=[%s] ElementalBonuses=[%s]"),
		ActionSummaries.Num() > 0 ? *FString::Join(ActionSummaries, TEXT("; ")) : TEXT("None"),
		BonusSummaries.Num() > 0 ? *FString::Join(BonusSummaries, TEXT("; ")) : TEXT("None"));
}

FString UJargonCardScript::GetRulesText() const
{
	TArray<FString> Parts;
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			Parts.Add(Action->GetRulesText());
		}
	}

	for (const FJargonCardElementalBonusScript& BonusScript : ElementalBonuses)
	{
		Parts.Add(BonusScript.GetRulesText());
	}

	return FString::Join(Parts, TEXT(" "));
}

void UJargonCardScript::RefreshEditorTitles()
{
	for (const TObjectPtr<UJargonCardAction>& Action : Actions)
	{
		if (Action)
		{
			Action->RefreshEditorTitle();
		}
	}

	for (FJargonCardElementalBonusScript& BonusScript : ElementalBonuses)
	{
		BonusScript.RefreshEditorTitle();
	}
}

void UJargonCardScript::PostLoad()
{
	Super::PostLoad();
	RefreshEditorTitles();
}

#if WITH_EDITOR
void UJargonCardScript::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshEditorTitles();
}

bool UJargonCardScript::ValidateScript(const UCardDefinition* Card, FDataValidationContext& Context) const
{
	bool bValid = true;

	if (!HasAnyActions())
	{
		JargonDataAssetValidation::AddError(Context, Card, TEXT("CardScript has no base effect-line entries."));
		bValid = false;
	}

	for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex)
	{
		const UJargonCardAction* Action = Actions[ActionIndex].Get();
		if (!Action)
		{
			JargonDataAssetValidation::AddError(Context, Card, FString::Printf(TEXT("CardScript effect line %d is null."), ActionIndex));
			bValid = false;
			continue;
		}

		bValid &= Action->ValidateAction(
			Card,
				FString::Printf(TEXT("CardScript effect line %d (%s)"), ActionIndex, *Action->GetActionSummary()),
			Context);
	}

	for (int32 BonusIndex = 0; BonusIndex < ElementalBonuses.Num(); ++BonusIndex)
	{
		bValid &= ElementalBonuses[BonusIndex].ValidateBonus(Card, BonusIndex, Context);
	}

	return bValid;
}

#endif
