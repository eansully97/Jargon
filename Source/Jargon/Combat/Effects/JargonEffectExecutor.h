#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"

class UJargonAbilityDefinition;

enum class EJargonEffectAsyncWarningPolicy : uint8
{
	Silent,
	Warn
};

struct JARGON_API FJargonEffectExecutionRequest
{
	const TArray<FJargonEffectSpec>* Effects = nullptr;
	FJargonEffectContext Context;
	FString SourceLabel;
	FString HookName;
	FJargonEffectTrace* OutTrace = nullptr;
	EJargonEffectAsyncWarningPolicy AsyncWarningPolicy = EJargonEffectAsyncWarningPolicy::Warn;
	bool bLogNoAuthoredEffects = false;
	bool bLogNoResolvedEffects = false;

	FString GetLogLabel() const;
};

struct JARGON_API FJargonEffectExecutionReport
{
	bool bAttempted = false;
	bool bResolverSucceeded = false;
	bool bResolvedAnyEffect = false;
	bool bContinuesAsynchronously = false;
	FJargonEffectResult Result;
	FString Reason;

	bool DidResolveSuccessfully() const
	{
		return bAttempted && bResolverSucceeded;
	}
};

class JARGON_API FJargonEffectExecutor
{
public:
	static FJargonEffectExecutionReport Execute(const FJargonEffectExecutionRequest& Request);
	static FJargonEffectExecutionReport ExecuteAbility(
		const UJargonAbilityDefinition* AbilityDefinition,
		const FJargonEffectContext& Context,
		const FString& SourceLabel = FString(),
		FJargonEffectTrace* OutTrace = nullptr);
};
