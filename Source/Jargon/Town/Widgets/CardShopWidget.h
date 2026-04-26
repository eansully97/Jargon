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
	UFUNCTION(BlueprintCallable, Category = "Card Shop")
	virtual void RefreshFromRunState(UJargonGameInstance* JargonGameInstance);

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
	UPROPERTY(BlueprintReadOnly, Category = "Card Shop")
	TArray<TObjectPtr<UCardPackDefinition>> AvailablePackOffers;

	UPROPERTY(BlueprintReadOnly, Category = "Card Shop")
	TArray<TObjectPtr<UCardDefinition>> LastGrantedCards;

	UPROPERTY(BlueprintReadOnly, Category = "Card Shop")
	FText LastFailureReason;
};