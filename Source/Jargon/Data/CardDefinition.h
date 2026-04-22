// CardDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "CardDefinition.generated.h"

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Range = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardEffectType EffectType = ECardEffectType::Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	int32 Value = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	ECardTargetType TargetType = ECardTargetType::Unit;
};