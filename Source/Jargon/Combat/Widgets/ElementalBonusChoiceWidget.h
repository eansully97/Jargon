#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/Widgets/ElementalBonusChoiceTypes.h"
#include "ElementalBonusChoiceWidget.generated.h"

class UButton;

UCLASS(Abstract, Blueprintable)
class JARGON_API UElementalBonusChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Opens/rebuilds the manual elemental bonus prompt from a combat controller request. */
	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	void SetChoiceRequest(const FJargonElementalBonusChoiceRequest& InRequest);

	/** Clears the current request and selected bonus indices without playing a card. */
	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	void ClearChoiceRequest();

	UFUNCTION(BlueprintPure, Category = "Elemental Bonus Choice")
	FJargonElementalBonusChoiceRequest GetChoiceRequest() const
	{
		return CurrentChoiceRequest;
	}

	UFUNCTION(BlueprintPure, Category = "Elemental Bonus Choice")
	bool HasChoiceRequest() const
	{
		return CurrentChoiceRequest.bHasUsableOptions;
	}

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	bool ConfirmSelectedBonusIndices(const TArray<int32>& InSelectedBonusIndices);

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	bool ConfirmSingleBonusIndex(int32 BonusIndex);

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	bool ConfirmFirstOfferedBonus();

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	bool ConfirmCurrentSelection();

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	bool SkipBonuses();

	/** Cancels the pending prompt; the controller decides whether card targeting remains active. */
	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	void CancelChoice();

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	void SetBonusSelected(int32 BonusIndex, bool bSelected);

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	void ToggleBonusSelected(int32 BonusIndex);

	UFUNCTION(BlueprintPure, Category = "Elemental Bonus Choice")
	bool IsBonusSelected(int32 BonusIndex) const;

	UFUNCTION(BlueprintPure, Category = "Elemental Bonus Choice")
	int32 GetFirstOfferedBonusIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice")
	void ClearSelectedBonusIndices();

	UFUNCTION(BlueprintPure, Category = "Elemental Bonus Choice")
	TArray<int32> GetSelectedBonusIndices() const
	{
		return SelectedBonusIndices;
	}

	UFUNCTION(BlueprintCallable, Category = "Elemental Bonus Choice|Layout")
	void PositionAtMouseCursor();

protected:
	void RefreshChoiceVisibility();

	UFUNCTION()
	void HandleConfirmButtonClicked();

	UFUNCTION()
	void HandleSkipButtonClicked();

	UFUNCTION()
	void HandleCancelButtonClicked();

	UFUNCTION()
	void HandleSpendButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Elemental Bonus Choice")
	void BP_OnChoiceRequestChanged(const FJargonElementalBonusChoiceRequest& Request);

	UFUNCTION(BlueprintImplementableEvent, Category = "Elemental Bonus Choice")
	void BP_OnChoiceRequestCleared();

protected:
	/** Optional Button binding named ConfirmButton for confirming the current manual selection. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Elemental Bonus Choice")
	TObjectPtr<UButton> ConfirmButton = nullptr;

	/** Optional Button binding named SpendButton for UIs that label confirmation as charge spending. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Elemental Bonus Choice")
	TObjectPtr<UButton> SpendButton = nullptr;

	/** Optional Button binding named SkipButton for playing only base card effects. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Elemental Bonus Choice")
	TObjectPtr<UButton> SkipButton = nullptr;

	/** Optional Button binding named CancelButton for backing out of the pending choice. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Elemental Bonus Choice")
	TObjectPtr<UButton> CancelButton = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elemental Bonus Choice|Layout", meta = (ToolTip = "When true, the choice prompt is placed near the owning player's mouse when a new request opens. The prompt does not follow the mouse after opening so buttons remain easy to click."))
	bool bPositionAtMouseOnOpen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elemental Bonus Choice|Layout", meta = (ToolTip = "Screen-space offset from the mouse cursor when the choice prompt opens."))
	FVector2D CursorOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elemental Bonus Choice|Layout", meta = (ClampMin = "0.0", ToolTip = "Minimum screen-space padding kept between the choice prompt and viewport edges."))
	FVector2D ViewportPadding = FVector2D(16.0f, 16.0f);

	/** Runtime-selected elemental bonus indices. These map to the card's ElementalBonuses array. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Elemental Bonus Choice")
	TArray<int32> SelectedBonusIndices;

	/** Current prompt data supplied by the combat controller; authoritative card play remains there. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Elemental Bonus Choice")
	FJargonElementalBonusChoiceRequest CurrentChoiceRequest;
};
