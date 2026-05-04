#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateColor.h"
#include "BattleUnitStatusWidget.generated.h"

class UTextBlock;
class ABattleUnit;

UCLASS()
class JARGON_API UBattleUnitStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Sets the runtime unit this widget mirrors. The widget does not own the unit. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit Status")
	void SetObservedUnit(ABattleUnit* InUnit);

	/** Pulls HP, attack, shield, and status counters from the observed unit into bound text widgets. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit Status")
	void RefreshFromObservedUnit();

protected:
	/** Required TextBlock binding named HPText. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HPText = nullptr;

	/** Required TextBlock binding named AttackText. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttackText = nullptr;

	/** Required TextBlock binding named ShieldText. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	/** Optional status TextBlock bindings; missing bindings simply omit that status from native text refresh. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StunText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FreezeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BurnText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RootText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VulnerableText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RegenText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeakText = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit Status")
	FSlateColor AttackReadyColor = FSlateColor(FLinearColor(0.95f, 0.95f, 0.05f, 1.f));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit Status")
	FSlateColor AttackUnavailableColor = FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 0.85f));

	/** Runtime unit mirrored by this widget component or HUD entry. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battle Unit Status")
	TObjectPtr<ABattleUnit> ObservedUnit = nullptr;
};
