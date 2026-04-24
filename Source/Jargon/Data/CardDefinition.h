// CardDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "Grid/BattleTileEffect.h"
#include "CardDefinition.generated.h"

class ABattleUnit;

UCLASS(BlueprintType)
class JARGON_API UCardDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Cost = 1;

	/** Current cast/targeting range from the source unit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Range = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardCategory Category = ECardCategory::Spell;

	/** Primary immediate effect type. Persistent Aura/Trap cards may leave this as None. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardEffectType EffectType = ECardEffectType::Damage;

	/** Primary effect value (damage, heal amount, push distance, etc.). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Value = 2;

	/** Area radius around the targeted or placed center tile. Aura cards use this as their board radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Radius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardTargetType TargetType = ECardTargetType::Unit;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Persistent")
	TSubclassOf<ABattleTileEffect> PersistentTileEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Summon")
	TSubclassOf<ABattleUnit> SummonedUnitClass;

	UFUNCTION(BlueprintPure, Category = "Card")
	bool UsesBoardTileTargeting() const
	{
		return TargetType != ECardTargetType::Self;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool RequiresUnitOnTargetTile() const
	{
		return TargetType == ECardTargetType::Unit;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool CreatesPersistentTileEffect() const
	{
		return Category == ECardCategory::Trap || Category == ECardCategory::Aura;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool RequiresEmptyTargetTile() const
	{
		return Category == ECardCategory::Summon || Category == ECardCategory::Trap;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool UsesRadiusField() const
	{
		return Category == ECardCategory::Aura || EffectType == ECardEffectType::AOE_Damage;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	int32 GetConfiguredAreaRadius() const
	{
		return FMath::Max(0, Radius);
	}
};
