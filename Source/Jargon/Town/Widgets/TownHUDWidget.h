#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TownHUDWidget.generated.h"

class UJargonGameInstance;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class JARGON_API UTownHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Town HUD")
	virtual void RefreshFromRunState(UJargonGameInstance* JargonGameInstance);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SilverText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CopperText = nullptr;
};