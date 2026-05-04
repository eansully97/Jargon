#include "Combat/Effects/JargonEffectExecutor.h"

#include "Combat/Effects/JargonEffectResolver.h"
#include "Data/JargonAbilityDefinition.h"

namespace
{
FString GetTriggerLogName(EJargonEffectTrigger Trigger)
{
	return JargonEffectContracts::GetEnumTokenName(
		StaticEnum<EJargonEffectTrigger>(),
		static_cast<int64>(Trigger));
}
}

FString FJargonEffectExecutionRequest::GetLogLabel() const
{
	if (!SourceLabel.IsEmpty() && !HookName.IsEmpty())
	{
		return FString::Printf(TEXT("%s %s"), *SourceLabel, *HookName);
	}

	if (!SourceLabel.IsEmpty())
	{
		return SourceLabel;
	}

	if (!HookName.IsEmpty())
	{
		return HookName;
	}

	return TEXT("EffectExecution");
}

FJargonEffectExecutionReport FJargonEffectExecutor::Execute(const FJargonEffectExecutionRequest& Request)
{
	FJargonEffectExecutionReport Report;
	const FString LogLabel = Request.GetLogLabel();

	if (!Request.Effects)
	{
		Report.Reason = TEXT("No effect array was supplied.");
		UE_LOG(LogTemp, Warning, TEXT("Effect execution skipped for '%s': %s"),
			*LogLabel,
			*Report.Reason);
		return Report;
	}

	if (Request.Effects->Num() <= 0)
	{
		Report.Reason = TEXT("No authored effects.");
		if (Request.bLogNoAuthoredEffects)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Effect execution skipped for '%s': %s"),
				*LogLabel,
				*Report.Reason);
		}
		return Report;
	}

	Report.bAttempted = true;
	Report.bResolverSucceeded = FJargonEffectResolver::ResolveEffects(
		*Request.Effects,
		Request.Context,
		Report.Result,
		Request.OutTrace);
	Report.bResolvedAnyEffect = Report.Result.bResolvedAnyEffect;
	Report.bContinuesAsynchronously = Report.Result.bContinuesAsynchronously;

	if (!Report.bResolverSucceeded)
	{
		Report.Reason = TEXT("Resolver returned false.");
		UE_LOG(LogTemp, Warning, TEXT("Effect execution failed for '%s'. Trigger=%s HookContext=%s Source=%s Effects=%d Reason=%s"),
			*LogLabel,
			*GetTriggerLogName(Request.Context.Trigger),
			*JargonEffectContracts::GetHookContextName(Request.HookContextType),
			*GetNameSafe(Request.Context.SourceObject.Get()),
			Request.Effects->Num(),
			*Report.Reason);
		return Report;
	}

	if (!Report.bResolvedAnyEffect)
	{
		Report.Reason = TEXT("No effects resolved.");
		if (Request.bLogNoResolvedEffects)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Effect execution completed for '%s' with no resolved effects. Trigger=%s HookContext=%s Source=%s Effects=%d"),
				*LogLabel,
				*GetTriggerLogName(Request.Context.Trigger),
				*JargonEffectContracts::GetHookContextName(Request.HookContextType),
				*GetNameSafe(Request.Context.SourceObject.Get()),
				Request.Effects->Num());
		}
	}
	else
	{
		Report.Reason = TEXT("Resolved.");
	}

	if (Report.bContinuesAsynchronously && Request.AsyncWarningPolicy == EJargonEffectAsyncWarningPolicy::Warn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Effect execution for '%s' started an async effect. Async sequencing is unchanged for this hook."),
			*LogLabel);
	}

	return Report;
}

FJargonEffectExecutionReport FJargonEffectExecutor::ExecuteAbility(
	const UJargonAbilityDefinition* AbilityDefinition,
	const FJargonEffectContext& Context,
	const FString& SourceLabel,
	FJargonEffectTrace* OutTrace,
	EJargonAbilityHookContextType HookContextType)
{
	FJargonEffectExecutionReport Report;
	if (!AbilityDefinition)
	{
		Report.Reason = TEXT("No ability definition was supplied.");
		UE_LOG(LogTemp, Warning, TEXT("Ability effect execution skipped: %s"), *Report.Reason);
		return Report;
	}

	TArray<FJargonEffectSpec> BuiltEffects;
	AbilityDefinition->BuildEffectSpecs(BuiltEffects);

	FJargonEffectExecutionRequest Request;
	Request.Effects = &BuiltEffects;
	Request.Context = Context;
	Request.HookContextType = HookContextType;
	Request.SourceLabel = SourceLabel.IsEmpty() ? GetNameSafe(AbilityDefinition) : SourceLabel;
	Request.HookName = AbilityDefinition->GetExecutionLabel().ToString();
	Request.OutTrace = OutTrace;
	Request.bLogNoAuthoredEffects = true;
	Request.bLogNoResolvedEffects = true;
	return Execute(Request);
}
