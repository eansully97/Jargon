#pragma once

#include "CoreMinimal.h"
#include "Combat/Widgets/JargonHoverInfoTypes.h"
#include "Core/JargonRunStateTypes.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "JargonTownPlayerController.generated.h"

class UTownHUDWidget;
class UCardShopWidget;
class UCombatHoverInfoWidget;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Town UI|Hover", meta = (AllowPrivateAccess = "true", ToolTip = "Optional Blueprint child of CombatHoverInfoWidget reused for description-only town/deck hover info."))
	TSubclassOf<UCombatHoverInfoWidget> TownHoverInfoWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Town UI|Hover", meta = (AllowPrivateAccess = "true"))
	int32 TownHoverInfoWidgetZOrder = 40;

	UPROPERTY()
	TObjectPtr<UTownHUDWidget> TownHUDWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UCardShopWidget> CardShopWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UDeckEditWidget> DeckEditWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UPostMatchReportWidget> PostMatchReportWidget = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Town UI|Hover", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatHoverInfoWidget> TownHoverInfoWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveModalWidget = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Town UI|Hover", meta = (AllowPrivateAccess = "true"))
	FJargonCombatHoverInfo CurrentTownHoverInfo;

	void CreateTownHUD();
	void InitializeTownHoverInfoWidget();
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
	void SetCurrentTownHoverInfo(const FJargonCombatHoverInfo& NewHoverInfo);

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

	UFUNCTION(BlueprintCallable, Category = "Town UI|Hover")
	void ShowTownHoverInfo(const FJargonCombatHoverInfo& HoverInfo);

	UFUNCTION(BlueprintCallable, Category = "Town UI|Hover")
	void ClearTownHoverInfo(UObject* SourceObject);

	UFUNCTION(BlueprintPure, Category = "Town UI|Hover")
	FJargonCombatHoverInfo GetCurrentTownHoverInfo() const
	{
		return CurrentTownHoverInfo;
	}

	UFUNCTION(BlueprintPure, Category = "Town UI|Hover")
	UCombatHoverInfoWidget* GetTownHoverInfoWidget() const
	{
		return TownHoverInfoWidget;
	}

	UPROPERTY(BlueprintAssignable, Category = "Town UI|Hover")
	FOnCombatHoverInfoChangedSignature OnTownHoverInfoChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town UI|Hover")
	bool bEnableTownHoverInfo = true;

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
