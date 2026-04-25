#pragma once

#include "CoreMinimal.h"
#include "JargonRunStateTypes.generated.h"

class UCardDefinition;

USTRUCT(BlueprintType)
struct JARGON_API FJargonCurrencyAmount
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency", meta = (ClampMin = "0"))
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency", meta = (ClampMin = "0"))
	int32 Silver = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency", meta = (ClampMin = "0"))
	int32 Copper = 0;

	int32 GetTotalCopperValue() const
	{
		return (FMath::Max(0, Gold) * 100) + (FMath::Max(0, Silver) * 10) + FMath::Max(0, Copper);
	}

	bool CanAfford(const FJargonCurrencyAmount& Cost) const
	{
		return GetTotalCopperValue() >= Cost.GetTotalCopperValue();
	}

	bool IsZero() const
	{
		return Gold <= 0 && Silver <= 0 && Copper <= 0;
	}

	void Normalize()
	{
		*this = FromTotalCopper(GetTotalCopperValue());
	}

	static FJargonCurrencyAmount FromTotalCopper(int32 TotalCopper)
	{
		FJargonCurrencyAmount Result;
		TotalCopper = FMath::Max(0, TotalCopper);

		Result.Gold = TotalCopper / 100;
		TotalCopper %= 100;

		Result.Silver = TotalCopper / 10;
		Result.Copper = TotalCopper % 10;
		return Result;
	}
};

USTRUCT(BlueprintType)
struct JARGON_API FWeightedCardPackEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ClampMin = "0"))
	int32 Weight = 1;

	bool IsValid() const
	{
		return CardDefinition != nullptr && Weight > 0;
	}
};
