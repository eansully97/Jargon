#include "Data/JargonDataAssetValidationHelpers.h"

#if WITH_EDITOR

#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "Misc/DataValidation.h"

namespace JargonDataAssetValidation
{
namespace
{
	FText BuildValidationText(const UObject* Source, const FString& Message)
	{
		return FText::FromString(FString::Printf(TEXT("%s: %s"), *GetNameSafe(Source), *Message));
	}

	bool IsJargonValueOperation(EJargonEffectOperation Operation)
	{
		return JargonEffectContracts::RequiresValue(Operation);
	}

	bool IsSuspiciousOneOffOperationName(const FString& OperationName)
	{
		static const TArray<FString> SuspiciousTokens =
		{
			TEXT("If"),
			TEXT("Per"),
			TEXT("Ignoring"),
			TEXT("When"),
			TEXT("While"),
			TEXT("With"),
			TEXT("Without")
		};

		for (const FString& Token : SuspiciousTokens)
		{
			if (OperationName.Contains(Token, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}

		return false;
	}

	void AddSuspiciousOperationNameWarning(
		FDataValidationContext& Context,
		const UObject* Source,
		const FString& EffectLabel,
		const FString& OperationName)
	{
		if (IsSuspiciousOneOffOperationName(OperationName))
		{
			AddWarning(
				Context,
				Source,
				FString::Printf(
					TEXT("%s uses suspicious one-off operation name '%s'. Prefer primitive operations plus targeting, payload, lightweight conditions, or status/keyword definitions."),
					*EffectLabel,
					*OperationName));
		}
	}

	bool OperationRequiresLivingSourceUnit(EJargonEffectOperation Operation)
	{
		switch (Operation)
		{
		case EJargonEffectOperation::MoveSource:
		case EJargonEffectOperation::PushTarget:
		case EJargonEffectOperation::PullTarget:
		case EJargonEffectOperation::SummonUnit:
			return true;
		default:
			return false;
		}
	}

	bool ValidateJargonEffectTriggerContext(
		const UObject* Source,
		const FJargonEffectSpec& EffectSpec,
		const FString& EffectLabel,
		EJargonEffectTrigger Trigger,
		FDataValidationContext& Context)
	{
		bool bValid = true;
		const FString OperationName = JargonEffectContracts::GetOperationName(EffectSpec.Operation);
		const FString DeliveryName = JargonEffectContracts::GetDeliveryName(EffectSpec.Delivery);

		if (Trigger == EJargonEffectTrigger::OnDeath)
		{
			const bool bOperationRequiresLivingSource = OperationRequiresLivingSourceUnit(EffectSpec.Operation);
			if (bOperationRequiresLivingSource)
			{
				AddError(
					Context,
					Source,
					FString::Printf(
						TEXT("%s uses %s in OnDeath. OnDeath effects run after the source unit is dead, so operations that require a living SourceUnit will fail at runtime."),
						*EffectLabel,
						*OperationName));
				bValid = false;
			}

			if (EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits)
			{
				AddError(
					Context,
					Source,
					FString::Printf(
						TEXT("%s uses Delivery=ChainUnits in OnDeath. OnDeath has no living initial unit target; use UnitsInRadius around the death tile or a tile operation instead."),
						*EffectLabel));
				bValid = false;
			}

			if ((EffectSpec.Delivery == EJargonEffectDelivery::Self || EffectSpec.Delivery == EJargonEffectDelivery::ExplicitUnit) &&
				JargonEffectContracts::RequiresUnitTargets(EffectSpec.Operation) &&
				!bOperationRequiresLivingSource)
			{
				AddError(
					Context,
					Source,
					FString::Printf(
						TEXT("%s uses %s with Delivery=%s in OnDeath. OnDeath cannot target the dead source as a living unit; use UnitsInRadius from the death tile when nearby units should be affected."),
						*EffectLabel,
						*OperationName,
						*DeliveryName));
				bValid = false;
			}
		}

		if (Trigger == EJargonEffectTrigger::OnTurnStart &&
			EffectSpec.Operation == EJargonEffectOperation::MoveSource)
		{
			AddError(
				Context,
				Source,
				FString::Printf(
					TEXT("%s uses MoveSource in OnTurnStart. Async turn-start movement is not sequenced yet, so this effect will fail at runtime."),
					*EffectLabel));
			bValid = false;
		}

		return bValid;
	}

}

void AddError(FDataValidationContext& Context, const UObject* Source, const FString& Message)
{
	Context.AddError(BuildValidationText(Source, Message));
}

void AddWarning(FDataValidationContext& Context, const UObject* Source, const FString& Message)
{
	Context.AddWarning(BuildValidationText(Source, Message));
}

bool IsJargonStatusLikeOperation(EJargonEffectOperation Operation)
{
	switch (Operation)
	{
	case EJargonEffectOperation::ApplyStun:
	case EJargonEffectOperation::ApplyFreeze:
	case EJargonEffectOperation::ApplyBurn:
	case EJargonEffectOperation::ApplyRoot:
	case EJargonEffectOperation::ApplyVulnerable:
	case EJargonEffectOperation::ApplyStatus:
		return true;
	default:
		return false;
	}
}

bool ValidateJargonEffectSpec(
	const UObject* Source,
	const FJargonEffectSpec& EffectSpec,
	const FString& EffectLabel,
	FDataValidationContext& Context)
{
	bool bValid = true;

	if (EffectSpec.Operation == EJargonEffectOperation::None)
	{
		AddError(Context, Source, FString::Printf(TEXT("%s has Operation=None."), *EffectLabel));
		bValid = false;
	}

	AddSuspiciousOperationNameWarning(
		Context,
		Source,
		EffectLabel,
		JargonEffectContracts::GetOperationName(EffectSpec.Operation));

	if (JargonEffectContracts::IsDirectStatusMigrationOperation(EffectSpec.Operation))
	{
		AddWarning(
			Context,
			Source,
			FString::Printf(
				TEXT("%s uses direct status operation '%s'. Prefer ApplyStatus with UJargonStatusEffectDefinition for new authoring."),
				*EffectLabel,
				*JargonEffectContracts::GetOperationName(EffectSpec.Operation)));
	}

	if (IsJargonValueOperation(EffectSpec.Operation) && EffectSpec.Value <= 0)
	{
		AddError(Context, Source, FString::Printf(TEXT("%s requires Value > 0."), *EffectLabel));
		bValid = false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge &&
		EffectSpec.ElementType == EJargonElementType::None)
	{
		AddError(Context, Source, FString::Printf(TEXT("%s requires ElementType other than None."), *EffectLabel));
		bValid = false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::ApplyStatus)
	{
		if (!EffectSpec.StatusEffectDefinition)
		{
			AddError(Context, Source, FString::Printf(TEXT("%s requires StatusEffectDefinition."), *EffectLabel));
			bValid = false;
		}
		else if (!EffectSpec.StatusEffectDefinition->IsValidDefinition())
		{
			AddError(Context, Source, FString::Printf(TEXT("%s references invalid StatusEffectDefinition '%s'."), *EffectLabel, *GetPathNameSafe(EffectSpec.StatusEffectDefinition.Get())));
			bValid = false;
		}
	}
	else if (EffectSpec.Operation == EJargonEffectOperation::CleanseStatus &&
		EffectSpec.StatusEffectDefinition &&
		!EffectSpec.StatusEffectDefinition->IsValidDefinition())
	{
		AddError(Context, Source, FString::Printf(TEXT("%s references invalid optional StatusEffectDefinition '%s'."), *EffectLabel, *GetPathNameSafe(EffectSpec.StatusEffectDefinition.Get())));
		bValid = false;
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits)
	{
		if (EffectSpec.ChainCount <= 0)
		{
			AddError(Context, Source, FString::Printf(TEXT("%s uses ChainUnits and requires ChainCount > 0."), *EffectLabel));
			bValid = false;
		}

		if (EffectSpec.Radius <= 0)
		{
			AddWarning(Context, Source, FString::Printf(TEXT("%s uses ChainUnits with Radius <= 0; runtime clamps this to a minimum search radius."), *EffectLabel));
		}
	}

	if (EffectSpec.Operation == EJargonEffectOperation::MoveSource && EffectSpec.MoveDistance <= 0)
	{
		AddError(Context, Source, FString::Printf(TEXT("%s requires MoveDistance > 0."), *EffectLabel));
		bValid = false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PushTarget && EffectSpec.PushDistance <= 0)
	{
		AddError(Context, Source, FString::Printf(TEXT("%s requires PushDistance > 0."), *EffectLabel));
		bValid = false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PullTarget && EffectSpec.PullDistance <= 0)
	{
		AddError(Context, Source, FString::Printf(TEXT("%s requires PullDistance > 0."), *EffectLabel));
		bValid = false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::SummonUnit)
	{
		if (!EffectSpec.SummonedUnitDefinition)
		{
			AddError(Context, Source, FString::Printf(TEXT("%s requires SummonedUnitDefinition."), *EffectLabel));
			bValid = false;
		}

		if (!EffectSpec.RuntimeSummonedUnitClass)
		{
			AddError(Context, Source, FString::Printf(TEXT("%s requires RuntimeSummonedUnitClass."), *EffectLabel));
			bValid = false;
		}
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PlaceTileEffect)
	{
		if (!EffectSpec.TileEffectDefinition)
		{
			AddError(Context, Source, FString::Printf(TEXT("%s requires TileEffectDefinition."), *EffectLabel));
			bValid = false;
		}

		if (!EffectSpec.RuntimeTileEffectClass)
		{
			AddError(Context, Source, FString::Printf(TEXT("%s requires RuntimeTileEffectClass."), *EffectLabel));
			bValid = false;
		}
	}

	TArray<FString> ContractWarnings;
	JargonEffectContracts::AppendIgnoredPayloadWarnings(EffectSpec, ContractWarnings);
	JargonEffectContracts::AppendDeliveryFilterWarnings(EffectSpec, ContractWarnings);
	for (const FString& Warning : ContractWarnings)
	{
		AddWarning(Context, Source, FString::Printf(TEXT("%s contract: %s"), *EffectLabel, *Warning));
	}

	return bValid;
}

bool ValidateJargonEffectSpecForTrigger(
	const UObject* Source,
	const FJargonEffectSpec& EffectSpec,
	const FString& EffectLabel,
	EJargonEffectTrigger Trigger,
	FDataValidationContext& Context)
{
	const bool bSpecValid = ValidateJargonEffectSpec(Source, EffectSpec, EffectLabel, Context);
	const bool bTriggerValid = ValidateJargonEffectTriggerContext(Source, EffectSpec, EffectLabel, Trigger, Context);
	return bSpecValid && bTriggerValid;
}
}

#endif
