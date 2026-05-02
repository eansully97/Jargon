#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JargonDeckDefinition.generated.h"

class UCardDefinition;

UCLASS(BlueprintType, meta = (DisplayName = "Jargon Deck Definition"))
class JARGON_API UJargonDeckDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deck", meta = (ToolTip = "Player-facing deck name shown in deck selection, starter deck, and audit UI."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deck", meta = (MultiLine = "true", ToolTip = "Designer-facing notes or player-facing summary for this starter/prebuilt deck."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deck", meta = (TitleProperty = "DisplayName", ToolTip = "Authored card copies in this deck. Duplicate entries represent duplicate copies."))
	TArray<TObjectPtr<UCardDefinition>> Cards;

	UFUNCTION(BlueprintPure, Category = "Deck")
	TArray<UCardDefinition*> GetResolvedCards() const;

	UFUNCTION(BlueprintPure, Category = "Deck")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Deck|Audit")
	FString GetAuditSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
