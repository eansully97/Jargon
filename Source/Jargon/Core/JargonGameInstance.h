// JargonGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonSaveGame.h"
#include "Exploration/Encounters/EncounterTypes.h"
#include "Engine/GameInstance.h"
#include "JargonGameInstance.generated.h"

class UCardDefinition;
class UCardPackDefinition;
class UJargonArtifactDefinition;
class UJargonHeroDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveHeroDefinitionChangedSignature, UJargonHeroDefinition*, NewHeroDefinition);

UCLASS()
class JARGON_API UJargonGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UJargonGameInstance();

	virtual void Init() override;

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

	/** Ensures prototype maps can enter the loop with a seeded run deck when no save/run exists yet. */
	void EnsureRunInitializedFromSeedDeck(const TArray<UCardDefinition*>& SeedDeck);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run")
	void ResetRunState();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	bool SaveCurrentRun();

	/** Save/load APIs are Blueprint-safe menu operations; they mutate GameInstance run state, not Data Assets. */
	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	bool LoadSavedRun();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	bool LoadSavedRunFromSlot(const FString& SaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	bool CreateNewSaveSlot(FString& OutSaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	TArray<FJargonSaveSlotSummary> GetSaveSlotSummaries();

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Save")
	bool HasSavedRun() const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	bool DeleteSavedRun();

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Save")
	bool DeleteSavedRunFromSlot(const FString& SaveSlotName);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Save")
	FString GetActiveRunSaveSlotName() const
	{
		return RunSaveSlotName;
	}

	UFUNCTION(BlueprintCallable, Category = "Jargon|Hero")
	void SetActiveHeroDefinition(UJargonHeroDefinition* HeroDefinition);

	/** Returns the selected hero, assigning the supplied default only when no active hero exists. */
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

	/** Returns reserve cards not currently in the active deck. */
	UFUNCTION(BlueprintPure, Category = "Jargon|Run")
	TArray<UCardDefinition*> GetRunReserveCards() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run")
	TArray<UCardDefinition*> GetOwnedRunCards() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetMaxRunDeckSize() const
	{
		return FMath::Max(1, MaxRunDeckSize);
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetMaxCopiesPerDeckCard() const
	{
		return FMath::Max(1, MaxCopiesPerDeckCard);
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetMaxRunDeckElements() const
	{
		return FMath::Max(0, MaxRunDeckElements);
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetRunDeckElementCount() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	TArray<EJargonElementType> GetRunDeckElements() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	FJargonDeckElementSummary GetRunDeckElementSummary() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	FText GetCardElementDisplayText(EJargonElementType CardElement) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	bool WouldRunDeckRespectElementLimitWithCard(const UCardDefinition* Card) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount GetRunCurrencies() const
	{
		return RunCurrencies;
	}

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount GetCardRecycleValue(const UCardDefinition* Card) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Artifacts")
	TArray<UJargonArtifactDefinition*> GetRunArtifacts() const;

	const TArray<TObjectPtr<UJargonArtifactDefinition>>& GetRunArtifactsRef() const
	{
		return RunArtifacts;
	}

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Artifacts")
	bool AddRunArtifact(UJargonArtifactDefinition* ArtifactDefinition);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Artifacts")
	bool HasRunArtifact(const UJargonArtifactDefinition* ArtifactDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Economy")
	bool CanAffordCurrency(const FJargonCurrencyAmount& Cost) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Economy")
	void AddCurrency(const FJargonCurrencyAmount& Amount);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Economy")
	bool TrySpendCurrency(const FJargonCurrencyAmount& Cost);

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Deck")
	bool MoveCardFromReserveToDeck(UCardDefinition* Card);

	/** Deck editing mutators enforce deck size, copy, ownership, and non-neutral element limits. */
	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Deck")
	bool MoveCardFromDeckToReserve(UCardDefinition* Card);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetRunDeckCardCopyCount(const UCardDefinition* Card) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetOwnedRunCardCopyCount(const UCardDefinition* Card) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetOwnedRunReserveCardCopyCount(const UCardDefinition* Card) const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	bool CanRecycleOwnedRunCard(const UCardDefinition* Card, FText& OutBlockedReason) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Deck")
	bool RecycleOwnedRunCard(UCardDefinition* Card, FJargonCurrencyAmount& OutCurrencyAwarded, FText& OutFailureReason);

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	int32 GetRecycleAllExtraReserveCardCopyCount() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount GetRecycleAllExtraReserveCardsValue() const;

	UFUNCTION(BlueprintPure, Category = "Jargon|Run|Deck")
	bool CanRecycleAllExtraReserveCards(FText& OutBlockedReason) const;

	UFUNCTION(BlueprintCallable, Category = "Jargon|Run|Deck")
	bool RecycleAllExtraReserveCards(int32& OutCardsRecycled, FJargonCurrencyAmount& OutCurrencyAwarded, FText& OutFailureReason);

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
	void RefreshRunReserveCardsFromAvailableShopPacks();

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

	/** Completes map return bookkeeping after combat/travel has restored the exploration world. */
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

	/** Returns the map that post-combat flow should load next based on pending encounter/return state. */
	UFUNCTION(BlueprintPure, Category = "Jargon|Encounter")
	FName GetPostCombatDestinationMapName() const;

protected:
	/** Helper conversions keep Blueprint-friendly arrays separate from internal TObjectPtr storage. */
	static TArray<UCardDefinition*> ConvertCardArray(const TArray<TObjectPtr<UCardDefinition>>& SourceCards);
	static bool RemoveCardFromCollection(TArray<TObjectPtr<UCardDefinition>>& CardCollection, UCardDefinition* Card);
	int32 CountUniqueNonNeutralElements(const TArray<TObjectPtr<UCardDefinition>>& CardCollection) const;
	TArray<EJargonElementType> GatherUniqueNonNeutralElements(const TArray<TObjectPtr<UCardDefinition>>& CardCollection) const;
	bool DoesCardCollectionRespectElementLimit(const TArray<TObjectPtr<UCardDefinition>>& CardCollection) const;
	void SetRunDeckInternal(const TArray<UCardDefinition*>& InitialDeck);
	void NormalizeRunCurrencies();
	void StorePostCombatReport(
		EJargonPostCombatResult Result,
		const FJargonCurrencyAmount& EnemyKillCurrency,
		const FJargonCurrencyAmount& VictoryBonusCurrency,
		int32 EnemiesDefeated
	);

	int32 CountCardCopiesInCollection(const TArray<TObjectPtr<UCardDefinition>>& Collection, const UCardDefinition* Card) const;
	int32 GetUsefulOwnedRunCardCopyFloor(const UCardDefinition* Card) const;
	int32 GetRecyclableOwnedRunCardCopyCount(const UCardDefinition* Card) const;
	void GatherRecyclableExtraOwnedRunCardCopies(TArray<UCardDefinition*>& OutCards) const;
	void SeedDefaultClassArtifactForActiveHero();

	/** Clears runtime run state. Static Data Asset definitions are never modified here. */
	void ClearRuntimeRunState(bool bResetHeroDefinition);
	void SaveCurrentRunIfActive();
	UJargonSaveIndex* LoadOrCreateSaveIndex() const;
	bool SaveSaveIndex(UJargonSaveIndex* SaveIndex) const;
	bool AdoptLegacySaveSlot(UJargonSaveIndex* SaveIndex) const;
	bool RegisterSaveSlot(const FString& SaveSlotName);
	bool UnregisterSaveSlot(const FString& SaveSlotName);
	FString GenerateNewSaveSlotName(UJargonSaveIndex* SaveIndex) const;
	FJargonSaveSlotSummary BuildSaveSlotSummary(const FString& SaveSlotName) const;
	FString BuildActiveHeroClassName() const;
	FString BuildHeroClassName(const UJargonHeroDefinition* HeroDefinition) const;
	static FText GetHeroClassDisplayName(EJargonHeroClass HeroClass);
	static FText FormatCurrencyAmount(FJargonCurrencyAmount CurrencyAmount);
	FDateTime GetSaveFileTimestamp(const FString& SaveSlotName) const;
	FString GetSaveGameFilePath(const FString& SaveSlotName) const;

protected:
	/** Active save slot used for the current run; menus may switch this before load/save. */
	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Save")
	FString RunSaveSlotName = TEXT("Jargon_Run");

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Save")
	FString LegacyRunSaveSlotName = TEXT("Jargon_Run");

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Save")
	FString SaveIndexSlotName = TEXT("Jargon_SaveIndex");

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Save")
	FString RunSaveSlotPrefix = TEXT("Jargon_Run_");

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Save", meta = (ClampMin = "0"))
	int32 RunSaveUserIndex = 0;

	/** Exploration map and transform captured before combat travel so victory/defeat can return correctly. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FName ReturnMapName = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FTransform ReturnTransform = FTransform::Identity;

	/** Runtime encounter data passed from the exploration world into the combat world. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	FPendingEncounterRuntimeData PendingEncounterData;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	bool bReturningFromCombat = false;

	/** Run-persistent IDs for defeated exploration encounters. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Encounter")
	TSet<FName> ClearedEncounterIds;

	/** Run-persistent IDs for one-shot exploration rewards or interactions. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Exploration")
	TSet<FName> CompletedExplorationInteractionIds;

	/** True after a new or loaded run has initialized deck, currency, hero, and artifacts. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run")
	bool bHasActiveRun = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Town")
	FName TownMapName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Deck", meta = (ClampMin = "1"))
	int32 MaxCopiesPerDeckCard = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Deck", meta = (ClampMin = "1"))
	int32 MaxRunDeckSize = 40;

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Deck", meta = (ClampMin = "0", ToolTip = "Maximum number of unique non-neutral card elements allowed in the active run deck. Neutral cards do not count."))
	int32 MaxRunDeckElements = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Jargon|Run|Economy", meta = (ToolTip = "Currency awarded when recycling one owned card copy above the useful owned copy limit. Normal decks keep at least MaxCopiesPerDeckCard copies."))
	FJargonCurrencyAmount CardRecycleValue;

	/** Active run deck. Entries are card Data Asset references; duplicate entries represent duplicate copies. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> ActiveRunDeck;

	/** Full owned card collection for the active run. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> RunOwnedCards;

	/** Owned cards not currently assigned to the active run deck. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Cards")
	TArray<TObjectPtr<UCardDefinition>> RunReserveCards;

	/** Run-persistent currencies. Combat-local resources, such as Energy and element charges, live in combat systems. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Economy")
	FJargonCurrencyAmount RunCurrencies;

	/** Run-persistent Artifact Data Asset references, including default class artifacts and earned rewards. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Run|Artifacts")
	TArray<TObjectPtr<UJargonArtifactDefinition>> RunArtifacts;

	/** Selected hero Data Asset for this run/save. Runtime combat state is copied into GameMode per combat. */
	UPROPERTY(VisibleAnywhere, Category = "Jargon|Hero")
	TObjectPtr<UJargonHeroDefinition> ActiveHeroDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Post Combat")
	bool bHasPendingPostCombatReport = false;

	UPROPERTY(VisibleAnywhere, Category = "Jargon|Post Combat")
	FJargonPostCombatReportData PendingPostCombatReport;
};
