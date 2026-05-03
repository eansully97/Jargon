#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JargonHoverInfoTypes.generated.h"

UENUM(BlueprintType)
enum class EJargonCombatHoverInfoType : uint8
{
	None UMETA(DisplayName = "None"),
	Unit UMETA(DisplayName = "Unit"),
	TileEffect UMETA(DisplayName = "Tile Effect"),
	Tile UMETA(DisplayName = "Tile")
};

USTRUCT(BlueprintType)
struct FJargonCombatHoverInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hover Info")
	bool bHasInfo = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hover Info")
	EJargonCombatHoverInfoType InfoType = EJargonCombatHoverInfoType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Hover Info")
	FText DescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "Hover Info")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Hover Info")
	TObjectPtr<UObject> SourceObject = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatHoverInfoChangedSignature, const FJargonCombatHoverInfo&, HoverInfo);
