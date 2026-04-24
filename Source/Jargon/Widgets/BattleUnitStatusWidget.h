#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleUnitStatusWidget.generated.h"

class UTextBlock;
class ABattleUnit;

UCLASS()
class JARGON_API UBattleUnitStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Battle Unit Status")
	void SetObservedUnit(ABattleUnit* InUnit);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit Status")
	void RefreshFromObservedUnit();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HPText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttackText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battle Unit Status")
	TObjectPtr<ABattleUnit> ObservedUnit = nullptr;
};