// JargonGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Encounters/EncounterTypes.h"
#include "Engine/GameInstance.h"
#include "JargonGameInstance.generated.h"

class UCardDefinition;
class UCardPackDefinition;

UCLASS()
class JARGON_API UJargonGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UJargonGameInstance();

	/** Legacy milestone-1 path. Keeps current encounter trigger flow working while we refactor forward. */
	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void StartEncounter(const FName& InEncounterId, const FName& InSourceMapName, const FTransform& InSourceTransform);

	/** Milestone-2 path. Stores full runtime encounter data before loading the combat map. */
	void StartEncounterWithRuntimeData(
		const FPendingEncounterRuntimeData& InPendingEncounter,
		const FName& InSourceMapName,
		const FTransform& InSourceTransform
	);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run")
	void StartNewRun(const TArray<UCardDefinition*>& InitialDeck, const FJargonCurrencyAmount& StartingCurrency);

	void EnsureRunInitializedFromSeedDeck(const TArray<TObjectPtr<UCardDefinition>>& SeedDeck);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run")
	void ResetRunState();

	UFUNCTION(BlueprintPure, Category = "Jargon|Run")
	bool HasActiveRun() const
	{
		return bHasActiveRun;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run")
	TArray<UCardDefinition*> GetRunDeckCards() const;

	const TArray<TObjectPtr<UCardDefinition>>& GetRunDeckCardsRef() const
	{
		return ActiveRunDeck;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run")
	TArray<UCardDefinition*> GetRunReserveCards() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run")
	TArray<UCardDefinition*> GetOwnedRunCards() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount GetRunCurrencies() const
	{
		return RunCurrencies;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Economy")
	bool CanAffordCurrency(const FJargonCurrencyAmount& Cost) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Economy")
	void AddCurrency(const FJargonCurrencyAmount& Amount);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Economy")
	bool TrySpendCurrency(const FJargonCurrencyAmount& Cost);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Deck")
	bool MoveCardFromReserveToDeck(UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Deck")
	bool MoveCardFromDeckToReserve(UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Town")
	void SetTownMapName(const FName& InTownMapName);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Town")
	const FName& GetTownMapName() const
	{
		return TownMapName;
	}

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Shop")
	void SetAvailableCardPackOffers(const TArray<UCardPackDefinition*>& InPackOffers);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Shop")
	TArray<UCardPackDefinition*> GetAvailableCardPackOffers() const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Shop")
	bool PurchaseCardPack(UCardPackDefinition* PackDefinition, TArray<UCardDefinition*>& OutGrantedCards, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void MarkEncounterCleared(const FName& EncounterId);

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	bool IsEncounterCleared(const FName& EncounterId) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void PrepareReturnToExploration();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void PrepareReturnToTownAfterCombat();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void CompleteReturnToExploration();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void CompletePostCombatReturn();

	void HandleCombatVictory();
	void HandleCombatDefeat();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void ClearPendingEncounter();

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	bool IsReturningFromCombat() const
	{
		return bReturningFromCombat;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	bool WillReturnToTownAfterCombat() const
	{
		return bReturnToTownAfterCombat && !TownMapName.IsNone();
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	const FName& GetReturnMapName() const
	{
		return ReturnMapName;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	const FTransform& GetReturnTransform() const
	{
		return ReturnTransform;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	const FName& GetPendingEncounterId() const
	{
		return PendingEncounterData.EncounterId;
	}

	const FPendingEncounterRuntimeData& GetPendingEncounterData() const
	{
		return PendingEncounterData;
	}

	bool HasPendingEncounterData() const
	{
		return PendingEncounterData.HasConfiguredCombatEncounter();
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	FName GetPostCombatDestinationMapName() const;

protected:
	static TArray<UCardDefinition*> ConvertCardArray(const TArray<TObjectPtr<UCardDefinition>>& SourceCards);
	static bool RemoveCardFromCollection(TArray<TObjectPtr<UCardDefinition>>& CardCollection, UCardDefinition* Card);
	void SetRunDeckInternal(const TArray<UCardDefinition*>& InitialDeck);
	void NormalizeRunCurrencies();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FName ReturnMapName = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FTransform ReturnTransform = FTransform::Identity;

	/** Runtime encounter data passed from the exploration world into the combat world. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FPendingEncounterRuntimeData PendingEncounterData;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	bool bReturnToTownAfterCombat = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	bool bReturningFromCombat = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	TSet<FName> ClearedEncounterIds;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run")
	bool bHasActiveRun = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Town")
	FName TownMapName = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> ActiveRunDeck;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> RunReserveCards;

	UPROPERTY(EditAnywhere, Category = "Jargon|Run|Shop")
	TArray<TObjectPtr<UCardPackDefinition>> AvailableCardPackOffers;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount RunCurrencies;
};
