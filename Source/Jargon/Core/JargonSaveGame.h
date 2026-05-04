#pragma once

#include "CoreMinimal.h"
#include "Core/JargonHeroTypes.h"
#include "Core/JargonRunStateTypes.h"
#include "GameFramework/SaveGame.h"
#include "JargonSaveGame.generated.h"

class UCardDefinition;
class UJargonHeroDefinition;
class UJargonRelicDefinition;

USTRUCT(BlueprintType)
struct JARGON_API FJargonSaveSlotSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FText ClassName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime TimeOfSave;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FText TimeOfSaveText;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FJargonCurrencyAmount CurrencyAmount;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FText CurrencyText;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bIsValid = false;
};

UCLASS()
class JARGON_API UJargonSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FString SaveSlotName;

	UPROPERTY(SaveGame)
	FString HeroClassName;

	UPROPERTY(SaveGame)
	EJargonHeroClass HeroClass = EJargonHeroClass::None;

	UPROPERTY(SaveGame)
	FDateTime TimeOfSave;

	UPROPERTY(SaveGame)
	bool bHasActiveRun = false;

	UPROPERTY(SaveGame)
	FName TownMapName = NAME_None;

	UPROPERTY(SaveGame)
	FJargonCurrencyAmount RunCurrencies;

	UPROPERTY(SaveGame)
	TArray<TSoftObjectPtr<UCardDefinition>> ActiveRunDeck;

	UPROPERTY(SaveGame)
	TArray<TSoftObjectPtr<UCardDefinition>> RunOwnedCards;

	UPROPERTY(SaveGame)
	TArray<TSoftObjectPtr<UCardDefinition>> RunReserveCards;

	UPROPERTY(SaveGame)
	TArray<TSoftObjectPtr<UJargonRelicDefinition>> RunRelics;

	UPROPERTY(SaveGame)
	TSoftObjectPtr<UJargonHeroDefinition> ActiveHeroDefinition;

	UPROPERTY(SaveGame)
	TSet<FName> ClearedEncounterIds;

	UPROPERTY(SaveGame)
	TSet<FName> CompletedExplorationInteractionIds;

	UPROPERTY(SaveGame)
	bool bHasPendingPostCombatReport = false;

	UPROPERTY(SaveGame)
	FJargonPostCombatReportData PendingPostCombatReport;
};

UCLASS()
class JARGON_API UJargonSaveIndex : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	TArray<FString> SaveSlotNames;

	UPROPERTY(SaveGame)
	int32 NextSaveSlotNumber = 1;
};
