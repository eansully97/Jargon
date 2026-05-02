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
		return (FMath::Max(0, Gold) * 1000) + (FMath::Max(0, Silver) * 100) + FMath::Max(0, Copper);
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

		Result.Gold = TotalCopper / 1000;
		TotalCopper %= 1000;

		Result.Silver = TotalCopper / 100;
		Result.Copper = TotalCopper % 100;
		return Result;
	}
};

UENUM(BlueprintType)
enum class EJargonPostCombatResult : uint8
{
	None UMETA(DisplayName = "None"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonPostCombatReportData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Post Combat")
	EJargonPostCombatResult Result = EJargonPostCombatResult::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Post Combat")
	FName EncounterId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Post Combat", meta = (ClampMin = "0"))
	int32 EnemiesDefeated = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Post Combat|Rewards")
	FJargonCurrencyAmount EnemyKillCurrency;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Post Combat|Rewards")
	FJargonCurrencyAmount VictoryBonusCurrency;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Post Combat|Rewards")
	FJargonCurrencyAmount TotalCurrencyEarned;

	void Reset()
	{
		Result = EJargonPostCombatResult::None;
		EncounterId = NAME_None;
		EnemiesDefeated = 0;
		EnemyKillCurrency = FJargonCurrencyAmount();
		VictoryBonusCurrency = FJargonCurrencyAmount();
		TotalCurrencyEarned = FJargonCurrencyAmount();
	}
};

USTRUCT(BlueprintType)
struct JARGON_API FWeightedCardPackEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ToolTip = "Card definition this weighted pack entry can grant."))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ClampMin = "0", ToolTip = "Relative roll weight. Entries with 0 or lower are skipped."))
	int32 Weight = 1;

	bool IsValid() const
	{
		return CardDefinition != nullptr && Weight > 0;
	}
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonSavedDeckData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Saved Deck")
	FString DeckName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Saved Deck")
	TArray<TSoftObjectPtr<UCardDefinition>> Cards;
};
