#pragma once

#include "CoreMinimal.h"
#include "Core/JargonRunStateTypes.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "JargonTownPlayerController.generated.h"

class UTownHUDWidget;
class UCardShopWidget;
class UDeckEditWidget;
class UPostMatchReportWidget;
class UUserWidget;

UCLASS()
class JARGON_API AJargonTownPlayerController : public AJargonExplorationPlayerController
{
	GENERATED_BODY()

public:
	AJargonTownPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Town UI")
	TSubclassOf<UTownHUDWidget> TownHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Town UI")
	TSubclassOf<UCardShopWidget> CardShopWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Town UI")
	TSubclassOf<UDeckEditWidget> DeckEditWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Town UI")
	TSubclassOf<UPostMatchReportWidget> PostMatchReportWidgetClass;

	UPROPERTY()
	TObjectPtr<UTownHUDWidget> TownHUDWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UCardShopWidget> CardShopWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UDeckEditWidget> DeckEditWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UPostMatchReportWidget> PostMatchReportWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveModalWidget = nullptr;

	void CreateTownHUD();
	void RefreshTownHUD();
	void RestoreTownWorldInputNextTick();

	void SetTownInputModeGameOnly();
	void SetTownInputModeUI(UUserWidget* FocusWidget);
	
	bool HasBlockingModalOpen() const;
	UUserWidget* GetTopmostTownModalWidget() const;
	void ApplyTownModalInputState(UUserWidget* PreferredFocusWidget = nullptr);

	void HideCardShopWithoutInputUpdate();
	void HideDeckEditWithoutInputUpdate();
	void HidePostMatchReportWithoutInputUpdate();

	void HandleOpenShopPressed();
	void HandleOpenDeckEditPressed();
	void HandleCloseTownPanelPressed();
	void TryOpenPendingPostCombatReport();

public:
	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void OpenCardShop();

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void CloseCardShop();

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void OpenDeckEdit();

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void CloseDeckEdit();

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void OpenPostMatchReport(const FJargonPostCombatReportData& ReportData);

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void ClosePostMatchReport();

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void CloseActiveTownPanel();

	UFUNCTION(BlueprintCallable, Category = "Town UI")
	void RefreshAllTownUI();

	UFUNCTION(BlueprintPure, Category = "Town UI")
	UTownHUDWidget* GetTownHUDWidget() const
	{
		return TownHUDWidget;
	}

	UFUNCTION(BlueprintPure, Category = "Town UI")
	UCardShopWidget* GetCardShopWidget() const
	{
		return CardShopWidget;
	}

	UFUNCTION(BlueprintPure, Category = "Town UI")
	UDeckEditWidget* GetDeckEditWidget() const
	{
		return DeckEditWidget;
	}

	UFUNCTION(BlueprintPure, Category = "Town UI")
	UPostMatchReportWidget* GetPostMatchReportWidget() const
	{
		return PostMatchReportWidget;
	}
};
