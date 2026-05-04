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
	/** Authored specs to resolve. The array is not owned by the request and must outlive the Execute call. */
	const TArray<FJargonEffectSpec>* Effects = nullptr;

	/** Fully populated runtime roles used by delivery, filters, placement, and presentation cues. */
	FJargonEffectContext Context;

	/** Optional expected hook context for validation/logging; ability execution fills this from the call site. */
	EJargonAbilityHookContextType HookContextType = EJargonAbilityHookContextType::None;

	/** Human-readable source label used only in logs and trace summaries. */
	FString SourceLabel;

	/** Human-readable hook label used only in logs and trace summaries. */
	FString HookName;

	/** Optional caller-owned trace sink for debugging validation, target gathering, and operation results. */
	FJargonEffectTrace* OutTrace = nullptr;

	/** Controls whether async effects warn when they leave resolution in progress. */
	EJargonEffectAsyncWarningPolicy AsyncWarningPolicy = EJargonEffectAsyncWarningPolicy::Warn;

	/** Log when a hook exists but authored no effect specs; useful during Data Asset migration. */
	bool bLogNoAuthoredEffects = false;

	/** Log when specs were authored but nothing resolved, usually due to target/context mismatch. */
	bool bLogNoResolvedEffects = false;

	FString GetLogLabel() const;
};

struct JARGON_API FJargonEffectExecutionReport
{
	/** True once the executor reached the resolver boundary. */
	bool bAttempted = false;

	/** Raw success/failure result from FJargonEffectResolver. */
	bool bResolverSucceeded = false;

	/** True when at least one spec produced gameplay or async work. */
	bool bResolvedAnyEffect = false;

	/** Mirrors resolver output for effects that start movement/presentation continuing after the call. */
	bool bContinuesAsynchronously = false;

	/** Resolver payload output used by card and hook callers. */
	FJargonEffectResult Result;

	/** Failure or skip reason for logs; empty on normal successful execution. */
	FString Reason;

	bool DidResolveSuccessfully() const
	{
		return bAttempted && bResolverSucceeded;
	}
};

class JARGON_API FJargonEffectExecutor
{
public:
	/** Preferred runtime entry point for authored effect specs outside the low-level resolver itself. */
	static FJargonEffectExecutionReport Execute(const FJargonEffectExecutionRequest& Request);

	/** Builds specs from a non-card ability definition and resolves them through the shared executor path. */
	static FJargonEffectExecutionReport ExecuteAbility(
		const UJargonAbilityDefinition* AbilityDefinition,
		const FJargonEffectContext& Context,
		const FString& SourceLabel = FString(),
		FJargonEffectTrace* OutTrace = nullptr,
		EJargonAbilityHookContextType HookContextType = EJargonAbilityHookContextType::None);
};
