#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/Widgets/JargonHoverInfoTypes.h"
#include "CombatHoverInfoWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class JARGON_API UCombatHoverInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Combat Hover Info")
	void SetHoverInfo(const FJargonCombatHoverInfo& InHoverInfo);

	UFUNCTION(BlueprintPure, Category = "Combat Hover Info")
	FJargonCombatHoverInfo GetHoverInfo() const
	{
		return CurrentHoverInfo;
	}

protected:
	void RefreshDescriptionText();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Hover Info")
	void BP_OnHoverInfoChanged(const FJargonCombatHoverInfo& HoverInfo);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat Hover Info")
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Hover Info")
	FJargonCombatHoverInfo CurrentHoverInfo;
};
