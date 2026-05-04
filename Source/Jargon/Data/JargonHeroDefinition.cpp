#include "Data/JargonHeroDefinition.h"

#include "Data/JargonAbilityDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogJargonHeroDefinition, Log, All);

void UJargonHeroDefinition::ApplyRecommendedClassPreset()
{
	switch (HeroClass)
	{
	case EJargonHeroClass::Paladin:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Paladin"));
		}
		MaxHP = 7;
		MoveRange = 3;
		AttackRange = 1;
		AttackDamage = 2;
		break;

	case EJargonHeroClass::Mage:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Mage"));
		}
		MaxHP = 4;
		MoveRange = 3;
		AttackRange = 3;
		AttackDamage = 1;
		break;

	case EJargonHeroClass::Rogue:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Rogue"));
		}
		MaxHP = 5;
		MoveRange = 4;
		AttackRange = 1;
		AttackDamage = 2;
		break;

	default:
		MaxHP = 5;
		MoveRange = 3;
		AttackRange = 1;
		AttackDamage = 1;
		break;
	}
}

bool UJargonHeroDefinition::IsValidDefinition() const
{
	const bool bStatsValid = MaxHP > 0
		&& MoveRange >= 0
		&& AttackRange > 0
		&& AttackDamage >= 0;

	if (!bStatsValid)
	{
		UE_LOG(LogJargonHeroDefinition, Warning, TEXT("Hero definition '%s' has invalid combat stats. MaxHP=%d MoveRange=%d AttackRange=%d AttackDamage=%d"),
			*GetNameSafe(this),
			MaxHP,
			MoveRange,
			AttackRange,
			AttackDamage);
	}

	TSet<EJargonHeroAspect> SeenAspects;
	TSet<EJargonElementType> SeenRequiredElements;

	for (const FJargonHeroAspectDefinition& AspectDefinition : HeroAspects)
	{
		if (AspectDefinition.Aspect == EJargonHeroAspect::None)
		{
			UE_LOG(LogJargonHeroDefinition, Warning, TEXT("Hero definition '%s' has an aspect entry with Aspect=None."),
				*GetNameSafe(this));
		}
		else if (SeenAspects.Contains(AspectDefinition.Aspect))
		{
			UE_LOG(LogJargonHeroDefinition, Warning, TEXT("Hero definition '%s' has duplicate aspect entry '%s'. The first matching entry is used at runtime."),
				*GetNameSafe(this),
				*StaticEnum<EJargonHeroAspect>()->GetNameStringByValue(static_cast<int64>(AspectDefinition.Aspect)));
		}
		else
		{
			SeenAspects.Add(AspectDefinition.Aspect);
		}

		if (AspectDefinition.RequiredElement == EJargonElementType::None)
		{
			UE_LOG(LogJargonHeroDefinition, Warning, TEXT("Hero definition '%s' has aspect '%s' with RequiredElement=None."),
				*GetNameSafe(this),
				*StaticEnum<EJargonHeroAspect>()->GetNameStringByValue(static_cast<int64>(AspectDefinition.Aspect)));
		}
		else if (SeenRequiredElements.Contains(AspectDefinition.RequiredElement))
		{
			UE_LOG(LogJargonHeroDefinition, Warning, TEXT("Hero definition '%s' has duplicate required element '%s'. The first matching entry is used for aspect activation."),
				*GetNameSafe(this),
				*StaticEnum<EJargonElementType>()->GetNameStringByValue(static_cast<int64>(AspectDefinition.RequiredElement)));
		}
		else
		{
			SeenRequiredElements.Add(AspectDefinition.RequiredElement);
		}

		if (!AspectDefinition.HasAnyTransformationOrPassiveEffects())
		{
			UE_LOG(LogJargonHeroDefinition, Warning, TEXT("Hero definition '%s' has aspect '%s' with no transformation or passive effects."),
				*GetNameSafe(this),
				*StaticEnum<EJargonHeroAspect>()->GetNameStringByValue(static_cast<int64>(AspectDefinition.Aspect)));
		}
	}

	return bStatsValid;
}

const FJargonHeroAspectDefinition* UJargonHeroDefinition::FindAspectDefinitionByAspect(EJargonHeroAspect Aspect) const
{
	if (Aspect == EJargonHeroAspect::None)
	{
		return nullptr;
	}

	return HeroAspects.FindByPredicate([Aspect](const FJargonHeroAspectDefinition& AspectDefinition)
	{
		return AspectDefinition.Aspect == Aspect;
	});
}

const FJargonHeroAspectDefinition* UJargonHeroDefinition::FindAspectDefinitionForElement(EJargonElementType Element) const
{
	if (Element == EJargonElementType::None)
	{
		return nullptr;
	}

	return HeroAspects.FindByPredicate([Element](const FJargonHeroAspectDefinition& AspectDefinition)
	{
		return AspectDefinition.RequiredElement == Element;
	});
}

#if WITH_EDITOR
EDataValidationResult UJargonHeroDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (HeroClass == EJargonHeroClass::None)
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("HeroClass is None; legacy placeholder heroes should be migrated before production use."));
	}

	if (MaxHP <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("MaxHP must be greater than 0. Current value: %d."), MaxHP));
	}

	if (MoveRange < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("MoveRange must be >= 0. Current value: %d."), MoveRange));
	}

	if (AttackRange <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("AttackRange must be greater than 0. Current value: %d."), AttackRange));
	}

	if (AttackDamage < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("AttackDamage must be >= 0. Current value: %d."), AttackDamage));
	}

	for (int32 EffectIndex = 0; EffectIndex < CombatStartPassive.Effects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			CombatStartPassive.Effects[EffectIndex],
			FString::Printf(TEXT("CombatStartPassive effect %d"), EffectIndex),
			EJargonEffectTrigger::OnCombatStart,
			Context);
	}

	if (CombatStartPassive.Ability)
	{
		if (!CombatStartPassive.Ability->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("CombatStartPassive Ability is assigned but is not a valid ability definition."));
		}

		if (CombatStartPassive.Effects.Num() > 0)
		{
			JargonDataAssetValidation::AddWarning(Context, this, TEXT("CombatStartPassive has both Ability and raw Effects authored. Runtime will prefer Ability; migrate or clear raw Effects after verification."));
		}
	}

	for (int32 EffectIndex = 0; EffectIndex < PlayerTurnStartPassive.Effects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
			this,
			PlayerTurnStartPassive.Effects[EffectIndex],
			FString::Printf(TEXT("PlayerTurnStartPassive effect %d"), EffectIndex),
			EJargonEffectTrigger::OnTurnStart,
			Context);
	}

	if (PlayerTurnStartPassive.Ability)
	{
		if (!PlayerTurnStartPassive.Ability->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("PlayerTurnStartPassive Ability is assigned but is not a valid ability definition."));
		}

		if (PlayerTurnStartPassive.Effects.Num() > 0)
		{
			JargonDataAssetValidation::AddWarning(Context, this, TEXT("PlayerTurnStartPassive has both Ability and raw Effects authored. Runtime will prefer Ability; migrate or clear raw Effects after verification."));
		}
	}

	TSet<EJargonHeroAspect> SeenAspects;
	TSet<EJargonElementType> SeenRequiredElements;
	for (int32 AspectIndex = 0; AspectIndex < HeroAspects.Num(); ++AspectIndex)
	{
		const FJargonHeroAspectDefinition& AspectDefinition = HeroAspects[AspectIndex];
		if (AspectDefinition.Aspect == EJargonHeroAspect::None)
		{
			JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d has Aspect=None."), AspectIndex));
		}
		else if (SeenAspects.Contains(AspectDefinition.Aspect))
		{
			JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d duplicates Aspect %s. Runtime uses the first matching entry."), AspectIndex, *StaticEnum<EJargonHeroAspect>()->GetNameStringByValue(static_cast<int64>(AspectDefinition.Aspect))));
		}
		else
		{
			SeenAspects.Add(AspectDefinition.Aspect);
		}

		if (AspectDefinition.RequiredElement == EJargonElementType::None)
		{
			JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d has RequiredElement=None."), AspectIndex));
		}
		else if (SeenRequiredElements.Contains(AspectDefinition.RequiredElement))
		{
			JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d duplicates RequiredElement %s. Runtime uses the first matching entry."), AspectIndex, *StaticEnum<EJargonElementType>()->GetNameStringByValue(static_cast<int64>(AspectDefinition.RequiredElement))));
		}
		else
		{
			SeenRequiredElements.Add(AspectDefinition.RequiredElement);
		}

		if (!AspectDefinition.HasAnyTransformationOrPassiveEffects())
		{
			JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d has no transformation or passive effects."), AspectIndex));
		}

		if (AspectDefinition.TransformationAbility)
		{
			if (!AspectDefinition.TransformationAbility->IsValidDefinition())
			{
				JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("HeroAspects entry %d TransformationAbility is assigned but is not a valid ability definition."), AspectIndex));
			}

			if (AspectDefinition.TransformationEffects.Num() > 0)
			{
				JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d has both TransformationAbility and raw TransformationEffects. Runtime will prefer the ability definition."), AspectIndex));
			}
		}

		if (AspectDefinition.TurnStartAbility)
		{
			if (!AspectDefinition.TurnStartAbility->IsValidDefinition())
			{
				JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("HeroAspects entry %d TurnStartAbility is assigned but is not a valid ability definition."), AspectIndex));
			}

			if (AspectDefinition.TurnStartEffects.Num() > 0)
			{
				JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d has both TurnStartAbility and raw TurnStartEffects. Runtime will prefer the ability definition."), AspectIndex));
			}
		}

		if (AspectDefinition.EnemyDeathAbility)
		{
			if (!AspectDefinition.EnemyDeathAbility->IsValidDefinition())
			{
				JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("HeroAspects entry %d EnemyDeathAbility is assigned but is not a valid ability definition."), AspectIndex));
			}

			if (AspectDefinition.EnemyDeathEffects.Num() > 0)
			{
				JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("HeroAspects entry %d has both EnemyDeathAbility and raw EnemyDeathEffects. Runtime will prefer the ability definition."), AspectIndex));
			}
		}

		for (int32 EffectIndex = 0; EffectIndex < AspectDefinition.TransformationEffects.Num(); ++EffectIndex)
		{
			JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
				this,
				AspectDefinition.TransformationEffects[EffectIndex],
				FString::Printf(TEXT("HeroAspects entry %d TransformationEffects effect %d"), AspectIndex, EffectIndex),
				EJargonEffectTrigger::Activated,
				Context);
		}

		for (int32 EffectIndex = 0; EffectIndex < AspectDefinition.TurnStartEffects.Num(); ++EffectIndex)
		{
			JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
				this,
				AspectDefinition.TurnStartEffects[EffectIndex],
				FString::Printf(TEXT("HeroAspects entry %d TurnStartEffects effect %d"), AspectIndex, EffectIndex),
				EJargonEffectTrigger::OnTurnStart,
				Context);
		}

		for (int32 EffectIndex = 0; EffectIndex < AspectDefinition.EnemyDeathEffects.Num(); ++EffectIndex)
		{
			JargonDataAssetValidation::ValidateJargonEffectSpecForTrigger(
				this,
				AspectDefinition.EnemyDeathEffects[EffectIndex],
				FString::Printf(TEXT("HeroAspects entry %d EnemyDeathEffects effect %d"), AspectIndex, EffectIndex),
				EJargonEffectTrigger::OnEnemyDeath,
				Context);
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
