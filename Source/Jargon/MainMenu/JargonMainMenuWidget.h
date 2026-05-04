#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/JargonSaveGame.h"
#include "JargonMainMenuWidget.generated.h"

class UButton;
class UJargonHeroDefinition;
class UScrollBox;
class UTextBlock;
class UWidgetSwitcher;

UCLASS()
class JARGON_API UJargonMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Page navigation helpers used by native button handlers and Blueprint menu flows. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowMainPage();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowSaveSelectPage();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowClassSelectPage();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void RefreshSaveList();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleNewSaveClicked();

	UFUNCTION()
	void HandleMageClassClicked();

	UFUNCTION()
	void HandlePaladinClassClicked();

	UFUNCTION()
	void HandleRogueClassClicked();

	UFUNCTION()
	void HandleClassSelectBackClicked();

	UFUNCTION()
	void HandleLoadSelectedSaveClicked();

	UFUNCTION()
	void HandleDeleteSelectedSaveClicked();

	UFUNCTION()
	void HandleCancelSelectedSaveClicked();

	UFUNCTION()
	void HandleConfirmDeleteClicked();

	UFUNCTION()
	void HandleCancelDeleteClicked();

	void HandleSaveSlotSelected(const FJargonSaveSlotSummary& SaveSlotSummary);
	void ShowSelectedSavePage();
	void ShowDeleteConfirmationPage();
	void ApplySelectedSaveSummaryText();
	void StartNewSaveWithHeroDefinition(const TCHAR* HeroDefinitionPath, const TCHAR* ClassDisplayName);
	void OpenTownMap();

	/** Map opened after loading or creating a run from the main menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Travel")
	FName TownMapName = TEXT("L_TownMap");

	/** Runtime widget references found from the rebuilt UMG tree; the widget does not own class defaults. */
	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> PageSwitcher = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PlayButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NewSaveButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MageClassButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PaladinClassButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RogueClassButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ClassSelectBackButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LoadSelectedSaveButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DeleteSelectedSaveButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelSelectedSaveButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmDeleteButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelDeleteButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> SaveListScrollBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EmptySaveListText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedSaveSummaryText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DeleteSaveSummaryText = nullptr;

	/** Currently selected save slot summary used by load/delete confirmation pages. */
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu|Save")
	FJargonSaveSlotSummary SelectedSaveSlotSummary;
};
