#include "Data/JargonTileEffectDefinition.h"

#include "Data/JargonAbilityDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
namespace
{
void ValidateTileEffectTriggerAbility(
	const UJargonTileEffectDefinition* Owner,
	FDataValidationContext& Context)
{
	if (!Owner || !Owner->TriggerAbility)
	{
		return;
	}

	if (!Owner->TriggerAbility->IsValidDefinition())
	{
		JargonDataAssetValidation::AddError(Context, Owner, TEXT("TriggerAbility is assigned but is not a valid ability definition."));
	}

	const EJargonEffectTrigger ExpectedTrigger = Owner->Trigger == EJargonTileEffectTrigger::OnPlayerTurnStart
		? EJargonEffectTrigger::OnTurnStart
		: EJargonEffectTrigger::OnEnterTile;
	const EJargonAbilityHookContextType ExpectedHookContext = Owner->Trigger == EJargonTileEffectTrigger::OnPlayerTurnStart
		? EJargonAbilityHookContextType::AuraPlayerTurnStart
		: EJargonAbilityHookContextType::TrapUnitEnter;

	if (Owner->TriggerAbility->ExpectedTrigger != ExpectedTrigger)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(
				TEXT("TriggerAbility expects trigger %s but this tile-effect hook requires %s."),
				*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(Owner->TriggerAbility->ExpectedTrigger)),
				*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(ExpectedTrigger))));
	}

	if (Owner->TriggerAbility->ExpectedHookContext == EJargonAbilityHookContextType::None)
	{
		JargonDataAssetValidation::AddWarning(Context, Owner, FString::Printf(TEXT("TriggerAbility has ExpectedHookContext=None; set it to %s."), *JargonEffectContracts::GetHookContextName(ExpectedHookContext)));
	}
	else if (Owner->TriggerAbility->ExpectedHookContext != ExpectedHookContext)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(
				TEXT("TriggerAbility expects hook context %s but this tile-effect hook requires %s."),
				*JargonEffectContracts::GetHookContextName(Owner->TriggerAbility->ExpectedHookContext),
				*JargonEffectContracts::GetHookContextName(ExpectedHookContext)));
	}
}
}
#endif

bool UJargonTileEffectDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& TriggerAbility != nullptr
		&& TriggerAbility->IsValidDefinition()
		&& Duration >= 0
		&& EffectRadius >= 0
		&& (TileEffectCategory == ECardCategory::Trap || TileEffectCategory == ECardCategory::Aura);
}

#if WITH_EDITOR
EDataValidationResult UJargonTileEffectDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (TileEffectCategory != ECardCategory::Trap && TileEffectCategory != ECardCategory::Aura)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("TileEffectCategory must be Trap or Aura."));
	}

	if (Duration < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("Duration must be >= 0. Current value: %d."), Duration));
	}

	if (EffectRadius < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("EffectRadius must be >= 0. Current value: %d."), EffectRadius));
	}

	if (!TriggerAbility)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("TriggerAbility is required."));
	}

	if (TriggerAbility)
	{
		ValidateTileEffectTriggerAbility(this, Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
