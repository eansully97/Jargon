// PlayerBattleUnit.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Units/BattleUnit.h"
#include "PlayerBattleUnit.generated.h"

class UJargonHeroDefinition;

UCLASS(HideCategories = ("Battle Unit|Stats"))
class JARGON_API APlayerBattleUnit : public ABattleUnit
{
	GENERATED_BODY()

public:
	APlayerBattleUnit();

	UFUNCTION(BlueprintCallable, Category = "Hero")
	virtual void InitializeFromHeroDefinition(UJargonHeroDefinition* HeroDefinition);

	UFUNCTION(BlueprintPure, Category = "Hero")
	UJargonHeroDefinition* GetAppliedHeroDefinition() const
	{
		return AppliedHeroDefinition;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Hero")
	void BP_OnHeroDefinitionApplied(UJargonHeroDefinition* HeroDefinition);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Hero")
	TObjectPtr<UJargonHeroDefinition> AppliedHeroDefinition = nullptr;
};
