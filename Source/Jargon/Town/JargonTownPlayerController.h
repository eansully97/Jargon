#pragma once

#include "CoreMinimal.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "JargonTownPlayerController.generated.h"

class UTownHUDWidget;
class UCardShopWidget;
class UDeckEditWidget;
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

	UPROPERTY()
	TObjectPtr<UTownHUDWidget> TownHUDWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UCardShopWidget> CardShopWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UDeckEditWidget> DeckEditWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveModalWidget = nullptr;

	void CreateTownHUD();
	void RefreshTownHUD();

	void SetTownInputModeGameOnly();
	void SetTownInputModeUI(UUserWidget* FocusWidget);

	void HandleOpenShopPressed();
	void HandleOpenDeckEditPressed();
	void HandleCloseTownPanelPressed();

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
};
