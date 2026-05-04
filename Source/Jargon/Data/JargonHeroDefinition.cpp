#include "Data/JargonHeroDefinition.h"

#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonArtifactDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogJargonHeroDefinition, Log, All);

#if WITH_EDITOR
namespace
{
void ValidateHeroAbilityForHook(
	const UObject* Owner,
	const UJargonAbilityDefinition* Ability,
	const FString& Label,
	EJargonEffectTrigger ExpectedTrigger,
	EJargonAbilityHookContextType ExpectedHookContext,
	FDataValidationContext& Context)
{
	if (!Ability)
	{
		return;
	}

	if (!Ability->IsValidDefinition())
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(TEXT("%s is assigned but is not a valid ability definition."), *Label));
	}

	if (Ability->ExpectedTrigger != ExpectedTrigger)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(
				TEXT("%s expects trigger %s but this hook requires %s."),
				*Label,
				*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(Ability->ExpectedTrigger)),
				*JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(ExpectedTrigger))));
	}

	if (Ability->ExpectedHookContext == EJargonAbilityHookContextType::None)
	{
		JargonDataAssetValidation::AddWarning(
			Context,
			Owner,
			FString::Printf(
				TEXT("%s has ExpectedHookContext=None; set it to %s for clearer targeting/placement validation."),
				*Label,
				*JargonEffectContracts::GetHookContextName(ExpectedHookContext)));
	}
	else if (Ability->ExpectedHookContext != ExpectedHookContext)
	{
		JargonDataAssetValidation::AddError(
			Context,
			Owner,
			FString::Printf(
				TEXT("%s expects hook context %s but this hook requires %s."),
				*Label,
				*JargonEffectContracts::GetHookContextName(Ability->ExpectedHookContext),
				*JargonEffectContracts::GetHookContextName(ExpectedHookContext)));
	}
}
}
#endif

void UJargonHeroDefinition::ApplyRecommendedClassPreset()
{
	switch (HeroClass)
	{
	case EJargonHeroClass::Paladin:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Paladin"));
		}
		MaxHP = 30;
		MoveRange = 3;
		AttackRange = 1;
		AttackDamage = 3;
		break;

	case EJargonHeroClass::Mage:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Mage"));
		}
		MaxHP = 25;
		MoveRange = 4;
		AttackRange = 2;
		AttackDamage = 2;
		break;

	case EJargonHeroClass::Rogue:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Rogue"));
		}
		MaxHP = 25;
		MoveRange = 4;
		AttackRange = 1;
		AttackDamage = 4;
		break;

	default:
		MaxHP = 30;
		MoveRange = 3;
		AttackRange = 1;
		AttackDamage = 3;
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

	if (DefaultClassArtifact)
	{
		if (!DefaultClassArtifact->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("DefaultClassArtifact is assigned but is not a valid Artifact definition."));
		}
		if (!DefaultClassArtifact->IsEligibleForHeroDefinition(this))
		{
			JargonDataAssetValidation::AddError(Context, this, TEXT("DefaultClassArtifact is not eligible for this hero class."));
		}
	}
	else if (HeroClass != EJargonHeroClass::None)
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("DefaultClassArtifact is empty. Real hero classes should seed class abilities through a default Artifact."));
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
			ValidateHeroAbilityForHook(
				this,
				AspectDefinition.TransformationAbility,
				FString::Printf(TEXT("HeroAspects entry %d TransformationAbility"), AspectIndex),
				EJargonEffectTrigger::Activated,
				EJargonAbilityHookContextType::HeroAspectTransformed,
				Context);
		}

		if (AspectDefinition.TurnStartAbility)
		{
			ValidateHeroAbilityForHook(
				this,
				AspectDefinition.TurnStartAbility,
				FString::Printf(TEXT("HeroAspects entry %d TurnStartAbility"), AspectIndex),
				EJargonEffectTrigger::OnTurnStart,
				EJargonAbilityHookContextType::HeroAspectTurnStart,
				Context);
		}

		if (AspectDefinition.EnemyDeathAbility)
		{
			ValidateHeroAbilityForHook(
				this,
				AspectDefinition.EnemyDeathAbility,
				FString::Printf(TEXT("HeroAspects entry %d EnemyDeathAbility"), AspectIndex),
				EJargonEffectTrigger::OnEnemyDeath,
				EJargonAbilityHookContextType::HeroAspectEnemyDeath,
				Context);
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
