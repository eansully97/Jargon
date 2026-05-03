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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Combat Hover Info")
	void SetHoverInfo(const FJargonCombatHoverInfo& InHoverInfo);

	UFUNCTION(BlueprintCallable, Category = "Combat Hover Info|Layout")
	void UpdateHoverPosition();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hover Info|Layout", meta = (ToolTip = "When true, this hover widget follows the owning player's mouse cursor while it has info."))
	bool bFollowMouseCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hover Info|Layout", meta = (ToolTip = "Screen-space offset from the mouse cursor before viewport edge flipping/clamping."))
	FVector2D CursorOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hover Info|Layout", meta = (ClampMin = "0.0", ToolTip = "Minimum screen-space padding kept between the hover widget and viewport edges when clamping is enabled."))
	FVector2D ViewportPadding = FVector2D(16.0f, 16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hover Info|Layout", meta = (ToolTip = "When true, the hover widget flips to the opposite side of the cursor if it would overflow the right or bottom viewport edge."))
	bool bFlipToStayOnScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hover Info|Layout", meta = (ToolTip = "When true, the hover widget is clamped inside the viewport padding after applying cursor offset and optional edge flipping."))
	bool bClampToViewport = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Hover Info")
	FJargonCombatHoverInfo CurrentHoverInfo;
};
