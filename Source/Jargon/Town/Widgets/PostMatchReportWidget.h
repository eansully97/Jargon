#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/JargonRunStateTypes.h"
#include "PostMatchReportWidget.generated.h"

UCLASS(Blueprintable)
class JARGON_API UPostMatchReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Post Match Report")
	void RefreshFromReportData(const FJargonPostCombatReportData& InReportData);

	UFUNCTION(BlueprintPure, Category = "Post Match Report")
	FJargonPostCombatReportData GetReportData() const
	{
		return ReportData;
	}

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Post Match Report")
	FJargonPostCombatReportData ReportData;

	UFUNCTION(BlueprintImplementableEvent, Category = "Post Match Report")
	void BP_OnReportDataRefreshed();
};
