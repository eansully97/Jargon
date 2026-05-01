// JargonGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Exploration/Encounters/EncounterTypes.h"
#include "Engine/GameInstance.h"
#include "JargonGameInstance.generated.h"

class UCardDefinition;
class UCardPackDefinition;
class UJargonRelicDefinition;
class UJargonHeroDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveHeroDefinitionChangedSignature, UJargonHeroDefinition*, NewHeroDefinition);

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

	UFUNCTION(BlueprintCallable, Category = "Jargon|Hero")
	void SetActiveHeroDefinition(UJargonHeroDefinition* HeroDefinition);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Hero")
	UJargonHeroDefinition* EnsureActiveHeroDefinition(UJargonHeroDefinition* DefaultHeroDefinition);

	UFUNCTION(BlueprintPure, Category = "Jargon|Hero")
	UJargonHeroDefinition* GetActiveHeroDefinition() const
	{
		return ActiveHeroDefinition;
	}

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

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Relics")
	TArray<UJargonRelicDefinition*> GetRunRelics() const;

	const TArray<TObjectPtr<UJargonRelicDefinition>>& GetRunRelicsRef() const
	{
		return RunRelics;
	}

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Relics")
	bool AddRunRelic(UJargonRelicDefinition* RelicDefinition);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Relics")
	bool HasRunRelic(const UJargonRelicDefinition* RelicDefinition) const;

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

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Shop")
	TArray<UCardPackDefinition*> GetAvailableCardPackOffers() const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Shop")
	bool PurchaseCardPack(UCardPackDefinition* PackDefinition, TArray<UCardDefinition*>& OutGrantedCards, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Encounter")
	void MarkEncounterCleared(const FName& EncounterId);

	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	bool IsEncounterCleared(const FName& EncounterId) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Exploration")
	void MarkExplorationInteractionCompleted(const FName& CompletionId);

	UFUNCTION(BlueprintPure, Category = "Jargon|Exploration")
	bool IsExplorationInteractionCompleted(const FName& CompletionId) const;

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
	void HandleCombatVictory(const FJargonCurrencyAmount& EnemyKillCurrency, int32 EnemiesDefeated);
	void HandleCombatDefeat(const FJargonCurrencyAmount& EnemyKillCurrency, int32 EnemiesDefeated);

	UFUNCTION(BlueprintPure, Category = "Jargon|Post Combat")
	bool HasPendingPostCombatReport() const
	{
		return bHasPendingPostCombatReport;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Post Combat")
	FJargonPostCombatReportData GetPendingPostCombatReport() const
	{
		return PendingPostCombatReport;
	}

	UPROPERTY(BlueprintAssignable, Category = "Jargon|Hero")
	FOnActiveHeroDefinitionChangedSignature OnActiveHeroDefinitionChanged;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Post Combat")
	void ClearPendingPostCombatReport();

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
	void StorePostCombatReport(
		EJargonPostCombatResult Result,
		const FJargonCurrencyAmount& EnemyKillCurrency,
		const FJargonCurrencyAmount& VictoryBonusCurrency,
		int32 EnemiesDefeated
	);

	int32 CountCardCopiesInCollection(const TArray<TObjectPtr<UCardDefinition>>& Collection, const UCardDefinition* Card) const;

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

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Exploration")
	TSet<FName> CompletedExplorationInteractionIds;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run")
	bool bHasActiveRun = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Town")
	FName TownMapName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Deck", meta = (ClampMin = "1"))
	int32 MaxCopiesPerDeckCard = 3;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> ActiveRunDeck;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> RunReserveCards;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount RunCurrencies;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Relics")
	TArray<TObjectPtr<UJargonRelicDefinition>> RunRelics;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Hero")
	TObjectPtr<UJargonHeroDefinition> ActiveHeroDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Post Combat")
	bool bHasPendingPostCombatReport = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Post Combat")
	FJargonPostCombatReportData PendingPostCombatReport;
};
