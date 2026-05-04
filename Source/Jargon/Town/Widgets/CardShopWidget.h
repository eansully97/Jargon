#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardShopWidget.generated.h"

class UJargonGameInstance;
class UCardPackDefinition;
class UCardDefinition;

UCLASS(Abstract, Blueprintable)
class JARGON_API UCardShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Copies available pack offers from GameInstance into Blueprint-readable shop state. */
	UFUNCTION(BlueprintCallable, Category = "Card Shop")
	virtual void RefreshFromRunState(UJargonGameInstance* JargonGameInstance);

	/** Attempts to buy a pack through GameInstance economy/deck state and caches the result for Blueprint UI. */
	UFUNCTION(BlueprintCallable, Category = "Card Shop")
	virtual bool PurchasePack(UJargonGameInstance* JargonGameInstance, UCardPackDefinition* PackDefinition);

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Shop")
	void BP_OnShopDataRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Shop")
	void BP_OnPackPurchased(bool bSuccess, const TArray<UCardDefinition*>& GrantedCards, const FText& FailureReason);

	UFUNCTION(BlueprintPure, Category = "Card Shop")
	const TArray<UCardPackDefinition*>& GetAvailablePackOffers() const
	{
		return AvailablePackOffers;
	}

	UFUNCTION(BlueprintPure, Category = "Card Shop")
	const TArray<UCardDefinition*>& GetLastGrantedCards() const
	{
		return LastGrantedCards;
	}

	UFUNCTION(BlueprintPure, Category = "Card Shop")
	FText GetLastFailureReason() const
	{
		return LastFailureReason;
	}

protected:
	/** Cached pack Data Asset offers currently available to the active run. */
	UPROPERTY(BlueprintReadOnly, Category = "Card Shop")
	TArray<TObjectPtr<UCardPackDefinition>> AvailablePackOffers;

	/** Cards granted by the most recent purchase attempt. */
	UPROPERTY(BlueprintReadOnly, Category = "Card Shop")
	TArray<TObjectPtr<UCardDefinition>> LastGrantedCards;

	/** Failure text from the most recent purchase attempt, empty on success. */
	UPROPERTY(BlueprintReadOnly, Category = "Card Shop")
	FText LastFailureReason;
};
