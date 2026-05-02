#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

class FDataValidationContext;
class UObject;
enum class EJargonEffectOperation : uint8;
struct FJargonEffectSpec;

namespace JargonDataAssetValidation
{
	void AddError(FDataValidationContext& Context, const UObject* Source, const FString& Message);
	void AddWarning(FDataValidationContext& Context, const UObject* Source, const FString& Message);

	bool IsJargonStatusLikeOperation(EJargonEffectOperation Operation);

	bool ValidateJargonEffectSpec(
		const UObject* Source,
		const FJargonEffectSpec& EffectSpec,
		const FString& EffectLabel,
		FDataValidationContext& Context);
}

#endif
