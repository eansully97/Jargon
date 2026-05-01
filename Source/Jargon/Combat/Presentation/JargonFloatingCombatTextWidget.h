#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "JargonFloatingCombatTextWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable)
class JARGON_API UJargonFloatingCombatTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Floating Combat Text")
	void InitializeFromCue(const FJargonCombatCueEvent& InCue, const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, Category = "Floating Combat Text")
	const FJargonCombatCueEvent& GetCue() const
	{
		return Cue;
	}

	UFUNCTION(BlueprintPure, Category = "Floating Combat Text")
	FText GetDisplayText() const
	{
		return DisplayText;
	}

	UFUNCTION(BlueprintPure, Category = "Floating Combat Text")
	FLinearColor GetDisplayColor() const
	{
		return DisplayColor;
	}

	UFUNCTION(BlueprintPure, Category = "Floating Combat Text")
	FVector GetCueWorldLocation() const
	{
		return Cue.WorldLocation;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Floating Combat Text")
	void BP_OnInitializedFromCue(const FJargonCombatCueEvent& InCue);

	UFUNCTION()
	void HandleAutoRemoveElapsed();

	void ApplyTextToBoundWidgets();

	static FText BuildDefaultTextForCue(const FJargonCombatCueEvent& InCue);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Floating Combat Text|Widgets")
	TObjectPtr<UTextBlock> FloatingTextText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Floating Combat Text|Widgets")
	TObjectPtr<UTextBlock> ValueText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Combat Text", meta = (ToolTip = "If enabled, the widget removes itself after AutoRemoveDelay seconds. Disable this if a Blueprint animation handles removal."))
	bool bAutoRemoveAfterDelay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Combat Text", meta = (EditCondition = "bAutoRemoveAfterDelay", ClampMin = "0.05", ToolTip = "Seconds before this floating text widget removes itself when auto-remove is enabled."))
	float AutoRemoveDelay = 1.75f;

	UPROPERTY(BlueprintReadOnly, Category = "Floating Combat Text")
	FJargonCombatCueEvent Cue;

	UPROPERTY(BlueprintReadOnly, Category = "Floating Combat Text")
	FText DisplayText;

	UPROPERTY(BlueprintReadOnly, Category = "Floating Combat Text")
	FLinearColor DisplayColor = FLinearColor::White;

	FTimerHandle AutoRemoveTimerHandle;
};
