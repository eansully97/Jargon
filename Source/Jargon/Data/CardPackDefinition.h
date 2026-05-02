#pragma once

#include "CoreMinimal.h"
#include "Core/JargonRunStateTypes.h"
#include "Engine/DataAsset.h"
#include "CardPackDefinition.generated.h"

class UCardDefinition;

UCLASS(BlueprintType)
class JARGON_API UCardPackDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ToolTip = "Player-facing pack name shown in town/shop UI."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (MultiLine = "true", ToolTip = "Player-facing pack description for shop/reward UI."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ToolTip = "Currency cost to purchase this pack."))
	FJargonCurrencyAmount Price;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ClampMin = "1", ToolTip = "Number of cards granted by one purchase."))
	int32 AmountToGrant = 5;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (ToolTip = "When false, one purchase attempts to grant unique cards from this pool."))
	bool bAllowDuplicateCardsPerPurchase = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pack", meta = (TitleProperty = "CardDefinition", ToolTip = "Weighted card entries this pack can grant. Entries with null cards or weight <= 0 are skipped."))
	TArray<FWeightedCardPackEntry> CardPool;

	UFUNCTION(BlueprintPure, Category = "Pack")
	bool IsValidDefinition() const;

	bool RollGrantedCards(TArray<UCardDefinition*>& OutGrantedCards) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
