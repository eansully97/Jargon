#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/JargonRunStateTypes.h"
#include "PostMatchReportWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPostMatchContinueRequestedSignature);

UCLASS(Blueprintable)
class JARGON_API UPostMatchReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Caches post-combat report data and notifies Blueprint presentation. */
	UFUNCTION(BlueprintCallable, Category = "Post Match Report")
	void RefreshFromReportData(const FJargonPostCombatReportData& InReportData);

	/** Broadcasts the continue request to the combat controller/GameInstance return flow. */
	UFUNCTION(BlueprintCallable, Category = "Post Match Report")
	void RequestContinue();

	UFUNCTION(BlueprintPure, Category = "Post Match Report")
	FJargonPostCombatReportData GetReportData() const
	{
		return ReportData;
	}

	UPROPERTY(BlueprintAssignable, Category = "Post Match Report")
	FOnPostMatchContinueRequestedSignature OnPostMatchContinueRequested;

protected:
	/** Runtime report payload owned by this widget; authoritative post-combat state lives in GameInstance. */
	UPROPERTY(BlueprintReadOnly, Category = "Post Match Report")
	FJargonPostCombatReportData ReportData;

	/** Optional Button binding named ContinueButton. Blueprint can also call RequestContinue directly. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Post Match Report")
	TObjectPtr<UButton> ContinueButton = nullptr;

	UFUNCTION()
	void HandleContinueButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Post Match Report")
	void BP_OnReportDataRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Post Match Report")
	void BP_OnContinueRequested();
};
