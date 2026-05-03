#include "Data/JargonTileEffectDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

bool UJargonTileEffectDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& Effects.Num() > 0
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

	if (Effects.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("Effects is empty."));
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

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpec(
			this,
			Effects[EffectIndex],
			FString::Printf(TEXT("Effects effect %d"), EffectIndex),
			Context);
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
