#include "Data/JargonStatusEffectDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

namespace
{
const TCHAR* GetStatusDefinitionKindName(EJargonStatusEffectKind StatusKind)
{
	switch (StatusKind)
	{
	case EJargonStatusEffectKind::Stun:
		return TEXT("Stun");
	case EJargonStatusEffectKind::Freeze:
		return TEXT("Freeze");
	case EJargonStatusEffectKind::Burn:
		return TEXT("Burn");
	case EJargonStatusEffectKind::Root:
		return TEXT("Root");
	case EJargonStatusEffectKind::Vulnerable:
		return TEXT("Vulnerable");
	case EJargonStatusEffectKind::Regen:
		return TEXT("Regen");
	case EJargonStatusEffectKind::Weak:
		return TEXT("Weak");
	case EJargonStatusEffectKind::None:
	default:
		return TEXT("None");
	}
}

const TCHAR* GetStatusIntentName(EJargonStatusEffectIntent Intent)
{
	switch (Intent)
	{
	case EJargonStatusEffectIntent::Friendly:
		return TEXT("Friendly");
	case EJargonStatusEffectIntent::Any:
		return TEXT("Any");
	case EJargonStatusEffectIntent::Hostile:
	default:
		return TEXT("Hostile");
	}
}
}

UJargonStatusEffectDefinition::UJargonStatusEffectDefinition()
{
	ValueLabel = FText::FromString(TEXT("value"));
}

bool UJargonStatusEffectDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty()
		&& StatusKind != EJargonStatusEffectKind::None;
}

FString UJargonStatusEffectDefinition::GetAuditSummary() const
{
	const FString NameText = DisplayName.IsEmpty()
		? GetNameSafe(this)
		: DisplayName.ToString();

	return FString::Printf(
		TEXT("DisplayName=%s StatusKind=%s TargetIntent=%s ValueLabel=%s HasRulesText=%s IsValidDefinition=%s"),
		*NameText,
		GetStatusDefinitionKindName(StatusKind),
		GetStatusIntentName(TargetIntent),
		*ValueLabel.ToString(),
		RulesText.IsEmpty() ? TEXT("false") : TEXT("true"),
		IsValidDefinition() ? TEXT("true") : TEXT("false"));
}

#if WITH_EDITOR
EDataValidationResult UJargonStatusEffectDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (StatusKind == EJargonStatusEffectKind::None)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("StatusKind must not be None."));
	}

	if (RulesText.IsEmpty())
	{
		JargonDataAssetValidation::AddWarning(Context, this, TEXT("RulesText is empty. Add concise rules-first wording before using this status in authoring."));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
