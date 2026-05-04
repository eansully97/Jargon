#include "Data/JargonTileEffectDefinition.h"

#include "Data/JargonAbilityDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

bool UJargonTileEffectDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& (TriggerAbility != nullptr || Effects.Num() > 0)
		&& (!TriggerAbility || TriggerAbility->IsValidDefinition())
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

	if (!TriggerAbility && Effects.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("TriggerAbility or raw Effects is required."));
	}

	if (TriggerAbility)
	{
		if (!TriggerAbility->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("TriggerAbility is assigned but is not a valid ability definition."));
		}

		if (Effects.Num() > 0)
		{
			JargonDataAssetValidation::AddWarning(Context, this, TEXT("TriggerAbility and raw Effects are both authored. Runtime will prefer TriggerAbility; clear raw Effects after migration verification."));
		}
	}

	if (EffectRadius > 0 && Effects.Num() > 0)
	{
		bool bAnySharedEffectUsesRadius = false;
		for (const FJargonEffectSpec& Effect : Effects)
		{
			if (Effect.Radius > 0)
			{
				bAnySharedEffectUsesRadius = true;
				break;
			}
		}

		if (!bAnySharedEffectUsesRadius)
		{
			JargonDataAssetValidation::AddWarning(
				Context,
				this,
				TEXT("EffectRadius is set, but no shared Effects entry has Radius > 0. Runtime shared effect resolution uses each effect's own Radius value."));
		}
	}

	const EJargonEffectTrigger EffectTrigger = Trigger == EJargonTileEffectTrigger::OnPlayerTurnStart
		? EJargonEffectTrigger::OnTurnStart
		: EJargonEffectTrigger::OnEnterTile;

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			Effects[EffectIndex],
			FString::Printf(TEXT("Effects effect %d"), EffectIndex),
			EffectTrigger,
			Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
