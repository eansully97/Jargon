#pragma once

#include "CoreMinimal.h"
#include "Core/JargonRunStateTypes.h"
#include "Engine/DataAsset.h"
#include "CardPackDefinition.generated.h"

class UCardDefinition;

UCLASS(BlueprintType)
class JARGON_API UCardPackDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack")
	FJargonCurrencyAmount Price;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ClampMin = "1"))
	int32 MinCardsGranted = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ClampMin = "1"))
	int32 MaxCardsGranted = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack")
	bool bAllowDuplicateCardsPerPurchase = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack")
	TArray<FWeightedCardPackEntry> CardPool;

	UFUNCTION(BlueprintPure, Category = "Pack")
	bool IsValidDefinition() const;

	bool RollGrantedCards(TArray<UCardDefinition*>& OutGrantedCards) const;
};
