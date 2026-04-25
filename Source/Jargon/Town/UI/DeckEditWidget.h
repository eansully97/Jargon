#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeckEditWidget.generated.h"

class UJargonGameInstance;
class UCardDefinition;

UCLASS(Abstract, Blueprintable)
class JARGON_API UDeckEditWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual void RefreshFromRunState(UJargonGameInstance* JargonGameInstance);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual bool MoveCardFromReserveToDeck(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual bool MoveCardFromDeckToReserve(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card);

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Edit")
	void BP_OnDeckDataRefreshed();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<TObjectPtr<UCardDefinition>> RunDeckCards;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<TObjectPtr<UCardDefinition>> RunReserveCards;
};