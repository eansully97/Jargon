#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/JargonSaveGame.h"
#include "JargonSaveSlotEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnJargonSaveSlotEntryClicked, const FJargonSaveSlotSummary&);

UCLASS()
class JARGON_API UJargonSaveSlotEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnJargonSaveSlotEntryClicked OnSaveSlotEntryClicked;

	void SetSaveSlotSummary(const FJargonSaveSlotSummary& InSummary);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleSaveButtonClicked();

	void RefreshVisuals();

	UPROPERTY(Transient)
	TObjectPtr<UButton> SaveButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ClassNameText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeOfSaveText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrencyText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FJargonSaveSlotSummary SaveSlotSummary;
};
