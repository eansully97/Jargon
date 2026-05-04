#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "ElementalBonusChoiceTypes.generated.h"

class UCardDefinition;

USTRUCT(BlueprintType)
struct JARGON_API FJargonElementalBonusChoiceOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	TObjectPtr<UCardDefinition> Card = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	int32 BonusIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	EJargonElementType ElementType = EJargonElementType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	FText ElementText;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	int32 RequiredChargeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	int32 CurrentChargeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	bool bSpendCharges = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	bool bIsUsable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	FText SummaryText;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	FText RulesText;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonElementalBonusChoiceRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	TObjectPtr<UCardDefinition> Card = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	TArray<FJargonElementalBonusChoiceOption> Options;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	FText PromptText;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Elemental Bonus")
	bool bHasUsableOptions = false;
};
