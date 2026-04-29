#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Engine/DataAsset.h"
#include "JargonRelicDefinition.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EJargonRelicRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Uncommon UMETA(DisplayName = "Uncommon"),
	Rare UMETA(DisplayName = "Rare"),
	Legendary UMETA(DisplayName = "Legendary")
};

/**
 * Run-persistent boon that resolves shared Jargon effects during combat.
 *
 * Relics are current-run rewards, not permanent progression. Keep effects
 * data-authored and let FJargonEffectResolver handle the gameplay payload.
 */
UCLASS(BlueprintType)
class JARGON_API UJargonRelicDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic")
	EJargonRelicRarity Rarity = EJargonRelicRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic|Effects", meta = (ToolTip = "Shared effects resolved once after combatants spawn and before the first player turn starts. The player unit is the source/self target."))
	TArray<FJargonEffectSpec> OnCombatStartEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic|Effects", meta = (ToolTip = "Shared effects resolved during player turn start. The player unit is the source/self target."))
	TArray<FJargonEffectSpec> OnPlayerTurnStartEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic|Effects", meta = (ToolTip = "Shared effects resolved after an enemy dies. The player unit is the source, and the enemy death tile is the primary tile target."))
	TArray<FJargonEffectSpec> OnEnemyDeathEffects;

	UFUNCTION(BlueprintPure, Category = "Relic")
	bool HasAnyEffects() const;

	UFUNCTION(BlueprintPure, Category = "Relic")
	bool IsValidDefinition() const;
};
