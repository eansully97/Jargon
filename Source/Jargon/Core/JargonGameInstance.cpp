#include "Core/JargonGameInstance.h"

#include "Data/CardDefinition.h"
#include "Jargon.h"
#include "Core/JargonSaveGame.h"
#include "Data/CardPackDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonArtifactDefinition.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Town/JargonTownGameMode.h"

namespace
{
FJargonCurrencyAmount CombineCurrencyAmounts(const FJargonCurrencyAmount& First, const FJargonCurrencyAmount& Second)
{
	return FJargonCurrencyAmount::FromTotalCopper(First.GetTotalCopperValue() + Second.GetTotalCopperValue());
}

bool CardCollectionContains(const TArray<TObjectPtr<UCardDefinition>>& CardCollection, const UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	for (const TObjectPtr<UCardDefinition>& ExistingCard : CardCollection)
	{
		if (ExistingCard.Get() == Card)
		{
			return true;
		}
	}

	return false;
}

void AddUniqueCardToCollection(TArray<TObjectPtr<UCardDefinition>>& CardCollection, UCardDefinition* Card)
{
	if (Card && !CardCollectionContains(CardCollection, Card))
	{
		CardCollection.Add(Card);
	}
}

FText GetCardFacingElementText(EJargonElementType CardElement)
{
	if (CardElement == EJargonElementType::None)
	{
		return NSLOCTEXT("JargonDeck", "NeutralCardElement", "Neutral");
	}

	const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
	return ElementEnum
		? ElementEnum->GetDisplayNameTextByValue(static_cast<int64>(CardElement))
		: FText::AsNumber(static_cast<int32>(CardElement));
}

template <typename AssetType>
void StoreSoftAssetReferences(
	const TArray<TObjectPtr<AssetType>>& SourceAssets,
	TArray<TSoftObjectPtr<AssetType>>& TargetAssets)
{
	TargetAssets.Reset();
	TargetAssets.Reserve(SourceAssets.Num());

	for (AssetType* SourceAsset : SourceAssets)
	{
		if (SourceAsset)
		{
			TargetAssets.Add(TSoftObjectPtr<AssetType>(SourceAsset));
		}
	}
}

template <typename AssetType>
void LoadSoftAssetReferences(
	const TArray<TSoftObjectPtr<AssetType>>& SourceAssets,
	TArray<TObjectPtr<AssetType>>& TargetAssets)
{
	TargetAssets.Reset();
	TargetAssets.Reserve(SourceAssets.Num());

	for (const TSoftObjectPtr<AssetType>& SourceAsset : SourceAssets)
	{
		AssetType* LoadedAsset = SourceAsset.LoadSynchronous();
		if (LoadedAsset)
		{
			TargetAssets.Add(LoadedAsset);
		}
	}
}
}

UJargonGameInstance::UJargonGameInstance()
{
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;
	PendingEncounterData.Reset();
	bReturningFromCombat = false;
	bHasActiveRun = false;
	TownMapName = NAME_None;
	RunCurrencies = FJargonCurrencyAmount();
	CardRecycleValue = FJargonCurrencyAmount();
	CardRecycleValue.Copper = 20;
	bHasPendingPostCombatReport = false;
	PendingPostCombatReport.Reset();
	ActiveHeroDefinition = nullptr;
}

void UJargonGameInstance::Init()
{
	Super::Init();
	LoadSavedRun();
}

void UJargonGameInstance::StartEncounter(
	const FName& InEncounterId,
	const FName& InSourceMapName,
	const FTransform& InSourceTransform
)
{
	// Legacy milestone-1 path.
	// We keep this so the old trigger-based loop still works until the real
	// exploration enemy actor replaces it.
	PendingEncounterData.Reset();
	PendingEncounterData.EncounterId = InEncounterId;

	ReturnMapName = InSourceMapName;
	ReturnTransform = InSourceTransform;
	bReturningFromCombat = false;
	SaveCurrentRunIfActive();
}

void UJargonGameInstance::StartEncounterWithRuntimeData(
	const FPendingEncounterRuntimeData& InPendingEncounter,
	const FName& InSourceMapName,
	const FTransform& InSourceTransform
)
{
	PendingEncounterData.Reset();
	PendingEncounterData.EncounterId = InPendingEncounter.EncounterId;
	PendingEncounterData.CombatMapName = InPendingEncounter.CombatMapName;
	PendingEncounterData.VictoryCurrencyReward = InPendingEncounter.VictoryCurrencyReward;

	for (const FEncounterEnemySpawn& SpawnEntry : InPendingEncounter.EnemySpawns)
	{
		if (SpawnEntry.IsValid())
		{
			PendingEncounterData.EnemySpawns.Add(SpawnEntry);
		}
	}

	ReturnMapName = InSourceMapName;
	ReturnTransform = InSourceTransform;
	bReturningFromCombat = false;
	SaveCurrentRunIfActive();
}

void UJargonGameInstance::StartNewRun(const TArray<UCardDefinition*>& InitialDeck, const FJargonCurrencyAmount& StartingCurrency)
{
	ResetRunState();
	SetRunDeckInternal(InitialDeck);
	SeedDefaultClassArtifactForActiveHero();
	RunCurrencies = StartingCurrency;
	NormalizeRunCurrencies();
	RefreshRunReserveCardsFromAvailableShopPacks();
	bHasActiveRun = true;

	UE_LOG(
		LogJargon,
		Log,
		TEXT("StartNewRun initialized active deck with %d cards, owned copies with %d cards, and reserve catalog with %d cards."),
		ActiveRunDeck.Num(),
		RunOwnedCards.Num(),
		RunReserveCards.Num()
	);

	SaveCurrentRun();
}

void UJargonGameInstance::EnsureRunInitializedFromSeedDeck(const TArray<UCardDefinition*>& SeedDeck)
{
	if (bHasActiveRun)
	{
		return;
	}

	TArray<UCardDefinition*> SeedCards;
	for (UCardDefinition* Card : SeedDeck)
	{
		if (Card)
		{
			SeedCards.Add(Card);
		}
	}

	if (SeedCards.Num() == 0)
	{
		return;
	}

	SetRunDeckInternal(SeedCards);
	SeedDefaultClassArtifactForActiveHero();
	RunCurrencies = FJargonCurrencyAmount();
	bHasActiveRun = ActiveRunDeck.Num() > 0;
	RefreshRunReserveCardsFromAvailableShopPacks();
	SaveCurrentRunIfActive();
}

void UJargonGameInstance::ResetRunState()
{
	ClearRuntimeRunState(false);
	DeleteSavedRun();
}

bool UJargonGameInstance::SaveCurrentRun()
{
	if (!bHasActiveRun)
	{
		return DeleteSavedRun();
	}

	UJargonSaveGame* SaveGame = Cast<UJargonSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UJargonSaveGame::StaticClass()));
	if (!SaveGame)
	{
		UE_LOG(LogJargon, Warning, TEXT("SaveCurrentRun failed because the save object could not be created."));
		return false;
	}

	SaveGame->SaveSlotName = RunSaveSlotName;
	SaveGame->HeroClassName = BuildActiveHeroClassName();
	SaveGame->HeroClass = ActiveHeroDefinition ? ActiveHeroDefinition->HeroClass : EJargonHeroClass::None;
	SaveGame->TimeOfSave = FDateTime::Now();
	SaveGame->bHasActiveRun = bHasActiveRun;
	SaveGame->TownMapName = TownMapName;
	SaveGame->RunCurrencies = RunCurrencies;
	SaveGame->RunCurrencies.Normalize();
	StoreSoftAssetReferences(ActiveRunDeck, SaveGame->ActiveRunDeck);
	StoreSoftAssetReferences(RunOwnedCards, SaveGame->RunOwnedCards);
	StoreSoftAssetReferences(RunReserveCards, SaveGame->RunReserveCards);
	StoreSoftAssetReferences(RunArtifacts, SaveGame->RunArtifacts);
	SaveGame->ActiveHeroDefinition = ActiveHeroDefinition
		? TSoftObjectPtr<UJargonHeroDefinition>(ActiveHeroDefinition.Get())
		: TSoftObjectPtr<UJargonHeroDefinition>();
	SaveGame->ClearedEncounterIds = ClearedEncounterIds;
	SaveGame->CompletedExplorationInteractionIds = CompletedExplorationInteractionIds;
	SaveGame->bHasPendingPostCombatReport = bHasPendingPostCombatReport;
	SaveGame->PendingPostCombatReport = PendingPostCombatReport;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, RunSaveSlotName, RunSaveUserIndex);
	UE_CLOG(!bSaved, LogJargon, Warning, TEXT("SaveCurrentRun failed for slot '%s' user index %d."),
		*RunSaveSlotName,
		RunSaveUserIndex);
	if (bSaved)
	{
		RegisterSaveSlot(RunSaveSlotName);
	}
	return bSaved;
}

bool UJargonGameInstance::LoadSavedRun()
{
	if (!HasSavedRun())
	{
		return false;
	}

	UJargonSaveGame* SaveGame = Cast<UJargonSaveGame>(
		UGameplayStatics::LoadGameFromSlot(RunSaveSlotName, RunSaveUserIndex));
	if (!SaveGame || !SaveGame->bHasActiveRun)
	{
		UE_LOG(LogJargon, Warning, TEXT("LoadSavedRun found no valid active run in slot '%s' user index %d."),
			*RunSaveSlotName,
			RunSaveUserIndex);
		return false;
	}

	LoadSoftAssetReferences(SaveGame->ActiveRunDeck, ActiveRunDeck);
	LoadSoftAssetReferences(SaveGame->RunOwnedCards, RunOwnedCards);
	LoadSoftAssetReferences(SaveGame->RunReserveCards, RunReserveCards);
	LoadSoftAssetReferences(SaveGame->RunArtifacts, RunArtifacts);

	ActiveHeroDefinition = SaveGame->ActiveHeroDefinition.LoadSynchronous();
	RunCurrencies = SaveGame->RunCurrencies;
	NormalizeRunCurrencies();
	TownMapName = SaveGame->TownMapName;
	ClearedEncounterIds = SaveGame->ClearedEncounterIds;
	CompletedExplorationInteractionIds = SaveGame->CompletedExplorationInteractionIds;
	bHasPendingPostCombatReport = SaveGame->bHasPendingPostCombatReport;
	PendingPostCombatReport = SaveGame->PendingPostCombatReport;
	bHasActiveRun = ActiveRunDeck.Num() > 0;

	PendingEncounterData.Reset();
	bReturningFromCombat = false;
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;

	if (!bHasActiveRun)
	{
		UE_LOG(LogJargon, Warning, TEXT("LoadSavedRun resolved no active deck cards from slot '%s'. Starting fresh will be required."),
			*RunSaveSlotName);
		return false;
	}

	UE_LOG(LogJargon, Log, TEXT("Loaded saved run from slot '%s'. Deck=%d Owned=%d ReserveCatalog=%d Artifacts=%d Currency=%d"),
		*RunSaveSlotName,
		ActiveRunDeck.Num(),
		RunOwnedCards.Num(),
		RunReserveCards.Num(),
		RunArtifacts.Num(),
		RunCurrencies.GetTotalCopperValue());
	return true;
}

bool UJargonGameInstance::LoadSavedRunFromSlot(const FString& SaveSlotName)
{
	if (SaveSlotName.IsEmpty())
	{
		UE_LOG(LogJargon, Warning, TEXT("LoadSavedRunFromSlot rejected an empty save slot name."));
		return false;
	}

	RunSaveSlotName = SaveSlotName;
	return LoadSavedRun();
}

bool UJargonGameInstance::CreateNewSaveSlot(FString& OutSaveSlotName)
{
	UJargonSaveIndex* SaveIndex = LoadOrCreateSaveIndex();
	if (!SaveIndex)
	{
		OutSaveSlotName.Reset();
		return false;
	}

	AdoptLegacySaveSlot(SaveIndex);
	OutSaveSlotName = GenerateNewSaveSlotName(SaveIndex);
	if (OutSaveSlotName.IsEmpty())
	{
		return false;
	}

	RunSaveSlotName = OutSaveSlotName;
	ClearRuntimeRunState(true);

	UE_LOG(LogJargon, Log, TEXT("Created new pending run save slot '%s'."), *RunSaveSlotName);
	return true;
}

TArray<FJargonSaveSlotSummary> UJargonGameInstance::GetSaveSlotSummaries()
{
	TArray<FJargonSaveSlotSummary> Summaries;

	UJargonSaveIndex* SaveIndex = LoadOrCreateSaveIndex();
	if (!SaveIndex)
	{
		return Summaries;
	}

	bool bIndexChanged = AdoptLegacySaveSlot(SaveIndex);
	TArray<FString> UniqueSlotNames;
	UniqueSlotNames.Reserve(SaveIndex->SaveSlotNames.Num());

	for (const FString& SaveSlotName : SaveIndex->SaveSlotNames)
	{
		if (SaveSlotName.IsEmpty() || UniqueSlotNames.Contains(SaveSlotName))
		{
			bIndexChanged = true;
			continue;
		}

		if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, RunSaveUserIndex))
		{
			bIndexChanged = true;
			continue;
		}

		FJargonSaveSlotSummary Summary = BuildSaveSlotSummary(SaveSlotName);
		if (!Summary.bIsValid)
		{
			bIndexChanged = true;
			continue;
		}

		UniqueSlotNames.Add(SaveSlotName);
		Summaries.Add(Summary);
	}

	if (bIndexChanged || UniqueSlotNames.Num() != SaveIndex->SaveSlotNames.Num())
	{
		SaveIndex->SaveSlotNames = UniqueSlotNames;
		SaveSaveIndex(SaveIndex);
	}

	Summaries.Sort([](const FJargonSaveSlotSummary& Left, const FJargonSaveSlotSummary& Right)
	{
		return Left.TimeOfSave > Right.TimeOfSave;
	});

	return Summaries;
}

bool UJargonGameInstance::HasSavedRun() const
{
	return UGameplayStatics::DoesSaveGameExist(RunSaveSlotName, RunSaveUserIndex);
}

bool UJargonGameInstance::DeleteSavedRun()
{
	return DeleteSavedRunFromSlot(RunSaveSlotName);
}

bool UJargonGameInstance::DeleteSavedRunFromSlot(const FString& SaveSlotName)
{
	if (SaveSlotName.IsEmpty())
	{
		return false;
	}

	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, RunSaveUserIndex))
	{
		UnregisterSaveSlot(SaveSlotName);
		return true;
	}

	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SaveSlotName, RunSaveUserIndex);
	UE_CLOG(!bDeleted, LogJargon, Warning, TEXT("DeleteSavedRun failed for slot '%s' user index %d."),
		*SaveSlotName,
		RunSaveUserIndex);
	if (bDeleted)
	{
		UnregisterSaveSlot(SaveSlotName);

		if (SaveSlotName == RunSaveSlotName)
		{
			ClearRuntimeRunState(true);
		}
	}
	return bDeleted;
}

void UJargonGameInstance::SetActiveHeroDefinition(UJargonHeroDefinition* HeroDefinition)
{
	if (ActiveHeroDefinition == HeroDefinition)
	{
		return;
	}

	ActiveHeroDefinition = HeroDefinition;
	OnActiveHeroDefinitionChanged.Broadcast(ActiveHeroDefinition);
	SaveCurrentRunIfActive();
}

UJargonHeroDefinition* UJargonGameInstance::EnsureActiveHeroDefinition(UJargonHeroDefinition* DefaultHeroDefinition)
{
	if (!ActiveHeroDefinition && DefaultHeroDefinition)
	{
		SetActiveHeroDefinition(DefaultHeroDefinition);
	}

	return ActiveHeroDefinition;
}

TArray<UCardDefinition*> UJargonGameInstance::GetRunDeckCards() const
{
	return ConvertCardArray(ActiveRunDeck);
}

TArray<UCardDefinition*> UJargonGameInstance::GetRunReserveCards() const
{
	return ConvertCardArray(RunReserveCards);
}

TArray<UCardDefinition*> UJargonGameInstance::GetOwnedRunCards() const
{
	return ConvertCardArray(RunOwnedCards);
}

int32 UJargonGameInstance::GetRunDeckElementCount() const
{
	return CountUniqueNonNeutralElements(ActiveRunDeck);
}

TArray<EJargonElementType> UJargonGameInstance::GetRunDeckElements() const
{
	return GatherUniqueNonNeutralElements(ActiveRunDeck);
}

FJargonDeckElementSummary UJargonGameInstance::GetRunDeckElementSummary() const
{
	FJargonDeckElementSummary Summary;
	Summary.ActiveElements = GetRunDeckElements();
	Summary.CurrentElementCount = Summary.ActiveElements.Num();
	Summary.MaxElementCount = GetMaxRunDeckElements();
	Summary.bHasAnyNonNeutralElements = Summary.CurrentElementCount > 0;
	Summary.bIsWithinLimit = Summary.CurrentElementCount <= Summary.MaxElementCount;

	for (const EJargonElementType Element : Summary.ActiveElements)
	{
		Summary.ActiveElementTexts.Add(GetCardElementDisplayText(Element));
	}

	if (Summary.ActiveElementTexts.Num() == 0)
	{
		Summary.SummaryText = FText::Format(
			NSLOCTEXT("JargonDeck", "DeckElementSummaryNeutralOnlyWithLimit", "Deck elements: Neutral only ({0} / {1})"),
			FText::AsNumber(Summary.CurrentElementCount),
			FText::AsNumber(Summary.MaxElementCount));
	}
	else
	{
		TArray<FString> ElementNames;
		ElementNames.Reserve(Summary.ActiveElementTexts.Num());
		for (const FText& ElementText : Summary.ActiveElementTexts)
		{
			ElementNames.Add(ElementText.ToString());
		}

		Summary.SummaryText = FText::Format(
			NSLOCTEXT("JargonDeck", "DeckElementSummaryWithElementsAndLimit", "Deck elements: {0} ({1} / {2})"),
			FText::FromString(FString::Join(ElementNames, TEXT(", "))),
			FText::AsNumber(Summary.CurrentElementCount),
			FText::AsNumber(Summary.MaxElementCount));
	}

	return Summary;
}

FText UJargonGameInstance::GetCardElementDisplayText(EJargonElementType CardElement) const
{
	return GetCardFacingElementText(CardElement);
}

bool UJargonGameInstance::WouldRunDeckRespectElementLimitWithCard(const UCardDefinition* Card) const
{
	if (!Card)
	{
		return DoesCardCollectionRespectElementLimit(ActiveRunDeck);
	}

	TArray<TObjectPtr<UCardDefinition>> CandidateDeck = ActiveRunDeck;
	CandidateDeck.Add(const_cast<UCardDefinition*>(Card));
	return DoesCardCollectionRespectElementLimit(CandidateDeck);
}

bool UJargonGameInstance::CanAffordCurrency(const FJargonCurrencyAmount& Cost) const
{
	return RunCurrencies.CanAfford(Cost);
}

void UJargonGameInstance::AddCurrency(const FJargonCurrencyAmount& Amount)
{
	RunCurrencies = FJargonCurrencyAmount::FromTotalCopper(
		RunCurrencies.GetTotalCopperValue() + Amount.GetTotalCopperValue());
	SaveCurrentRunIfActive();
}

FJargonCurrencyAmount UJargonGameInstance::GetCardRecycleValue(const UCardDefinition* Card) const
{
	FJargonCurrencyAmount RecycleValue = Card ? CardRecycleValue : FJargonCurrencyAmount();
	RecycleValue.Normalize();
	return RecycleValue;
}

TArray<UJargonArtifactDefinition*> UJargonGameInstance::GetRunArtifacts() const
{
	TArray<UJargonArtifactDefinition*> Artifacts;
	Artifacts.Reserve(RunArtifacts.Num());

	for (UJargonArtifactDefinition* ArtifactDefinition : RunArtifacts)
	{
		if (ArtifactDefinition)
		{
			Artifacts.Add(ArtifactDefinition);
		}
	}

	return Artifacts;
}

bool UJargonGameInstance::AddRunArtifact(UJargonArtifactDefinition* ArtifactDefinition)
{
	if (!ArtifactDefinition)
	{
		UE_LOG(LogJargon, Warning, TEXT("AddRunArtifact rejected a null artifact definition."));
		return false;
	}

	if (!bHasActiveRun)
	{
		UE_LOG(LogJargon, Warning, TEXT("AddRunArtifact accepted '%s' while no active run is marked. It will still be stored until run state resets."),
			*GetNameSafe(ArtifactDefinition));
	}

	if (HasRunArtifact(ArtifactDefinition))
	{
		UE_LOG(LogJargon, Log, TEXT("AddRunArtifact skipped duplicate artifact '%s'."), *GetNameSafe(ArtifactDefinition));
		return false;
	}

	RunArtifacts.Add(ArtifactDefinition);

	UE_LOG(LogJargon, Log, TEXT("Added run artifact '%s'. Total artifacts=%d"),
		*GetNameSafe(ArtifactDefinition),
		RunArtifacts.Num());

	SaveCurrentRunIfActive();
	return true;
}

bool UJargonGameInstance::HasRunArtifact(const UJargonArtifactDefinition* ArtifactDefinition) const
{
	if (!ArtifactDefinition)
	{
		return false;
	}

	for (const TObjectPtr<UJargonArtifactDefinition>& RunArtifact : RunArtifacts)
	{
		if (RunArtifact == ArtifactDefinition)
		{
			return true;
		}
	}

	return false;
}

bool UJargonGameInstance::TrySpendCurrency(const FJargonCurrencyAmount& Cost)
{
	if (!CanAffordCurrency(Cost))
	{
		return false;
	}

	RunCurrencies = FJargonCurrencyAmount::FromTotalCopper(
		RunCurrencies.GetTotalCopperValue() - Cost.GetTotalCopperValue());
	SaveCurrentRunIfActive();
	return true;
}

bool UJargonGameInstance::MoveCardFromReserveToDeck(UCardDefinition* Card)
{
	if (!bHasActiveRun || !Card)
	{
		UE_LOG(LogJargon, Warning, TEXT("MoveCardFromReserveToDeck rejected. ActiveRun=%s Card=%s"),
			bHasActiveRun ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Card));
		return false;
	}

	const int32 MaxDeckSize = GetMaxRunDeckSize();
	if (ActiveRunDeck.Num() >= MaxDeckSize)
	{
		UE_LOG(LogJargon, Log, TEXT("MoveCardFromReserveToDeck rejected. Deck has %d cards and max deck size is %d."),
			ActiveRunDeck.Num(),
			MaxDeckSize);
		return false;
	}

	if (!WouldRunDeckRespectElementLimitWithCard(Card))
	{
		UE_LOG(LogJargon, Log, TEXT("MoveCardFromReserveToDeck rejected. Adding '%s' would exceed the deck element limit of %d unique non-neutral elements."),
			*GetNameSafe(Card),
			GetMaxRunDeckElements());
		return false;
	}

	const int32 MaxCopiesPerCard = GetMaxCopiesPerDeckCard();
	const int32 CurrentDeckCopies = CountCardCopiesInCollection(ActiveRunDeck, Card);

	if (CurrentDeckCopies >= MaxCopiesPerCard)
	{
		UE_LOG(LogJargon, Log, TEXT("MoveCardFromReserveToDeck rejected. Deck already has %d copies of '%s'. Max=%d"),
			CurrentDeckCopies,
			*GetNameSafe(Card),
			MaxCopiesPerCard);
		return false;
	}

	const int32 OwnedCopies = CountCardCopiesInCollection(RunOwnedCards, Card);
	const int32 AvailableOwnedCopies = FMath::Max(0, OwnedCopies - CurrentDeckCopies);
	if (AvailableOwnedCopies <= 0)
	{
		UE_LOG(LogJargon, Log, TEXT("MoveCardFromReserveToDeck rejected. No owned reserve copies of '%s' are available. Owned=%d Deck=%d"),
			*GetNameSafe(Card),
			OwnedCopies,
			CurrentDeckCopies);
		return false;
	}

	ActiveRunDeck.Add(Card);
	AddUniqueCardToCollection(RunReserveCards, Card);

	UE_LOG(LogJargon, Log, TEXT("Moved card '%s' from owned reserve to deck. Deck=%d Owned=%d ReserveCatalog=%d"),
		*GetNameSafe(Card),
		ActiveRunDeck.Num(),
		RunOwnedCards.Num(),
		RunReserveCards.Num());

	SaveCurrentRunIfActive();
	return true;
}

bool UJargonGameInstance::MoveCardFromDeckToReserve(UCardDefinition* Card)
{
	if (!bHasActiveRun || !Card)
	{
		UE_LOG(LogJargon, Warning, TEXT("MoveCardFromDeckToReserve rejected. ActiveRun=%s Card=%s"),
			bHasActiveRun ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Card));
		return false;
	}

	if (!RemoveCardFromCollection(ActiveRunDeck, Card))
	{
		UE_LOG(LogJargon, Warning, TEXT("MoveCardFromDeckToReserve could not find card '%s' in active deck."),
			*GetNameSafe(Card));
		return false;
	}

	AddUniqueCardToCollection(RunReserveCards, Card);

	UE_LOG(LogJargon, Log, TEXT("Moved card '%s' from deck to owned reserve. Deck=%d Owned=%d ReserveCatalog=%d"),
		*GetNameSafe(Card),
		ActiveRunDeck.Num(),
		RunOwnedCards.Num(),
		RunReserveCards.Num());

	SaveCurrentRunIfActive();
	return true;
}

int32 UJargonGameInstance::GetRunDeckCardCopyCount(const UCardDefinition* Card) const
{
	return CountCardCopiesInCollection(ActiveRunDeck, Card);
}

int32 UJargonGameInstance::GetOwnedRunCardCopyCount(const UCardDefinition* Card) const
{
	return CountCardCopiesInCollection(RunOwnedCards, Card);
}

int32 UJargonGameInstance::GetOwnedRunReserveCardCopyCount(const UCardDefinition* Card) const
{
	return FMath::Max(0, GetOwnedRunCardCopyCount(Card) - GetRunDeckCardCopyCount(Card));
}

bool UJargonGameInstance::CanRecycleOwnedRunCard(const UCardDefinition* Card, FText& OutBlockedReason) const
{
	OutBlockedReason = FText::GetEmpty();

	if (!bHasActiveRun)
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleBlockedNoActiveRun", "No active run is available.");
		return false;
	}

	if (!Card)
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleBlockedMissingCard", "No card is selected.");
		return false;
	}

	if (GetCardRecycleValue(Card).IsZero())
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleBlockedNoValue", "Card recycling has no currency value configured.");
		return false;
	}

	if (GetRecyclableOwnedRunCardCopyCount(Card) <= 0)
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleBlockedNoExtraUsefulCopies", "No owned copies above the useful deck copy limit are available to recycle.");
		return false;
	}

	return true;
}

bool UJargonGameInstance::RecycleOwnedRunCard(
	UCardDefinition* Card,
	FJargonCurrencyAmount& OutCurrencyAwarded,
	FText& OutFailureReason)
{
	OutCurrencyAwarded = FJargonCurrencyAmount();
	OutFailureReason = FText::GetEmpty();

	if (!CanRecycleOwnedRunCard(Card, OutFailureReason))
	{
		UE_LOG(LogJargon, Log, TEXT("RecycleOwnedRunCard rejected '%s': %s"),
			*GetNameSafe(Card),
			*OutFailureReason.ToString());
		return false;
	}

	if (!RemoveCardFromCollection(RunOwnedCards, Card))
	{
		OutFailureReason = NSLOCTEXT("JargonDeck", "RecycleBlockedOwnedCopyMissing", "Owned card copy could not be found.");
		UE_LOG(LogJargon, Warning, TEXT("RecycleOwnedRunCard could not remove owned reserve copy '%s' after validation passed."),
			*GetNameSafe(Card));
		return false;
	}

	OutCurrencyAwarded = GetCardRecycleValue(Card);
	AddCurrency(OutCurrencyAwarded);
	RefreshRunReserveCardsFromAvailableShopPacks();

	UE_LOG(LogJargon, Log, TEXT("Recycled one reserve copy of '%s' for %d copper. Owned=%d Deck=%d ReserveCopies=%d Currency=%d"),
		*GetNameSafe(Card),
		OutCurrencyAwarded.GetTotalCopperValue(),
		GetOwnedRunCardCopyCount(Card),
		GetRunDeckCardCopyCount(Card),
		GetOwnedRunReserveCardCopyCount(Card),
		RunCurrencies.GetTotalCopperValue());

	SaveCurrentRunIfActive();
	return true;
}

int32 UJargonGameInstance::GetRecycleAllExtraReserveCardCopyCount() const
{
	TArray<UCardDefinition*> RecyclableCards;
	GatherRecyclableExtraOwnedRunCardCopies(RecyclableCards);
	return RecyclableCards.Num();
}

FJargonCurrencyAmount UJargonGameInstance::GetRecycleAllExtraReserveCardsValue() const
{
	TArray<UCardDefinition*> RecyclableCards;
	GatherRecyclableExtraOwnedRunCardCopies(RecyclableCards);

	int32 TotalCopper = 0;
	for (const UCardDefinition* Card : RecyclableCards)
	{
		TotalCopper += GetCardRecycleValue(Card).GetTotalCopperValue();
	}

	return FJargonCurrencyAmount::FromTotalCopper(TotalCopper);
}

bool UJargonGameInstance::CanRecycleAllExtraReserveCards(FText& OutBlockedReason) const
{
	OutBlockedReason = FText::GetEmpty();

	if (!bHasActiveRun)
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleAllBlockedNoActiveRun", "No active run is available.");
		return false;
	}

	if (GetRecycleAllExtraReserveCardCopyCount() <= 0)
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleAllBlockedNoExtraUsefulCopies", "No owned copies above the useful deck copy limit are available to recycle.");
		return false;
	}

	if (GetRecycleAllExtraReserveCardsValue().IsZero())
	{
		OutBlockedReason = NSLOCTEXT("JargonDeck", "RecycleAllBlockedNoValue", "Card recycling has no currency value configured.");
		return false;
	}

	return true;
}

bool UJargonGameInstance::RecycleAllExtraReserveCards(
	int32& OutCardsRecycled,
	FJargonCurrencyAmount& OutCurrencyAwarded,
	FText& OutFailureReason)
{
	OutCardsRecycled = 0;
	OutCurrencyAwarded = FJargonCurrencyAmount();
	OutFailureReason = FText::GetEmpty();

	if (!CanRecycleAllExtraReserveCards(OutFailureReason))
	{
		UE_LOG(LogJargon, Log, TEXT("RecycleAllExtraReserveCards rejected: %s"), *OutFailureReason.ToString());
		return false;
	}

	TArray<UCardDefinition*> RecyclableCards;
	GatherRecyclableExtraOwnedRunCardCopies(RecyclableCards);

	OutCurrencyAwarded = GetRecycleAllExtraReserveCardsValue();
	for (UCardDefinition* Card : RecyclableCards)
	{
		if (RemoveCardFromCollection(RunOwnedCards, Card))
		{
			++OutCardsRecycled;
		}
	}

	if (OutCardsRecycled <= 0)
	{
		OutCurrencyAwarded = FJargonCurrencyAmount();
		OutFailureReason = NSLOCTEXT("JargonDeck", "RecycleAllBlockedNoOwnedCopiesRemoved", "No owned reserve copies could be removed.");
		UE_LOG(LogJargon, Warning, TEXT("RecycleAllExtraReserveCards found recyclable cards but removed none."));
		return false;
	}

	AddCurrency(OutCurrencyAwarded);
	RefreshRunReserveCardsFromAvailableShopPacks();

	UE_LOG(LogJargon, Log, TEXT("Recycled %d owned card copies above the useful copy limit for %d copper. Owned=%d Deck=%d Currency=%d"),
		OutCardsRecycled,
		OutCurrencyAwarded.GetTotalCopperValue(),
		RunOwnedCards.Num(),
		ActiveRunDeck.Num(),
		RunCurrencies.GetTotalCopperValue());

	SaveCurrentRunIfActive();
	return true;
}

void UJargonGameInstance::SetTownMapName(const FName& InTownMapName)
{
	TownMapName = InTownMapName;
	SaveCurrentRunIfActive();
}

TArray<UCardPackDefinition*> UJargonGameInstance::GetAvailableCardPackOffers() const
{
	TArray<UCardPackDefinition*> PackOffers;

	const UWorld* World = GetWorld();
	const AJargonTownGameMode* TownGameMode = World ? World->GetAuthGameMode<AJargonTownGameMode>() : nullptr;
	if (!TownGameMode)
	{
		return PackOffers;
	}

	const TArray<TObjectPtr<UCardPackDefinition>>& TownPackOffers = TownGameMode->GetTownShopPackOffers();
	PackOffers.Reserve(TownPackOffers.Num());

	for (UCardPackDefinition* PackOffer : TownPackOffers)
	{
		if (PackOffer)
		{
			PackOffers.Add(PackOffer);
		}
	}

	return PackOffers;
}

void UJargonGameInstance::RefreshRunReserveCardsFromAvailableShopPacks()
{
	TArray<TObjectPtr<UCardDefinition>> NewReserveCatalog;

	const TArray<UCardPackDefinition*> PackOffers = GetAvailableCardPackOffers();
	for (const UCardPackDefinition* PackOffer : PackOffers)
	{
		if (!PackOffer)
		{
			continue;
		}

		for (const FWeightedCardPackEntry& CardEntry : PackOffer->CardPool)
		{
			if (CardEntry.IsValid() && CardEntry.CardDefinition->IsValidDefinition())
			{
				AddUniqueCardToCollection(NewReserveCatalog, CardEntry.CardDefinition.Get());
			}
		}
	}

	for (UCardDefinition* Card : ActiveRunDeck)
	{
		AddUniqueCardToCollection(NewReserveCatalog, Card);
	}

	for (UCardDefinition* Card : RunOwnedCards)
	{
		AddUniqueCardToCollection(NewReserveCatalog, Card);
	}

	RunReserveCards = MoveTemp(NewReserveCatalog);
}

bool UJargonGameInstance::PurchaseCardPack(
	UCardPackDefinition* PackDefinition,
	TArray<UCardDefinition*>& OutGrantedCards,
	FText& OutFailureReason)
{
	OutGrantedCards.Reset();
	OutFailureReason = FText::GetEmpty();

	if (!bHasActiveRun)
	{
		OutFailureReason = FText::FromString(TEXT("No active run is available for purchases."));
		return false;
	}

	if (!PackDefinition || !PackDefinition->IsValidDefinition())
	{
		OutFailureReason = FText::FromString(TEXT("Pack definition is invalid."));
		return false;
	}

	if (!CanAffordCurrency(PackDefinition->Price))
	{
		OutFailureReason = FText::FromString(TEXT("Not enough currency to buy this pack."));
		return false;
	}

	if (!TrySpendCurrency(PackDefinition->Price))
	{
		OutFailureReason = FText::FromString(TEXT("Unable to spend currency for this pack."));
		return false;
	}

	if (!PackDefinition->RollGrantedCards(OutGrantedCards))
	{
		AddCurrency(PackDefinition->Price);
		OutFailureReason = FText::FromString(TEXT("Pack failed to roll any cards."));
		return false;
	}

	for (UCardDefinition* GrantedCard : OutGrantedCards)
	{
		if (GrantedCard)
		{
			RunOwnedCards.Add(GrantedCard);
			AddUniqueCardToCollection(RunReserveCards, GrantedCard);
		}
	}

	RefreshRunReserveCardsFromAvailableShopPacks();

	UE_LOG(LogJargon, Log, TEXT("Purchased pack '%s'. Granted=%d Deck=%d Owned=%d ReserveCatalog=%d"), *GetNameSafe(PackDefinition), OutGrantedCards.Num(), ActiveRunDeck.Num(), RunOwnedCards.Num(), RunReserveCards.Num());

	SaveCurrentRunIfActive();
	return true;
}

void UJargonGameInstance::MarkEncounterCleared(const FName& EncounterId)
{
	if (!EncounterId.IsNone())
	{
		ClearedEncounterIds.Add(EncounterId);
		SaveCurrentRunIfActive();
	}
}

bool UJargonGameInstance::IsEncounterCleared(const FName& EncounterId) const
{
	if (EncounterId.IsNone())
	{
		return false;
	}

	return ClearedEncounterIds.Contains(EncounterId);
}

void UJargonGameInstance::MarkExplorationInteractionCompleted(const FName& CompletionId)
{
	if (!CompletionId.IsNone())
	{
		CompletedExplorationInteractionIds.Add(CompletionId);
		SaveCurrentRunIfActive();
	}
}

bool UJargonGameInstance::IsExplorationInteractionCompleted(const FName& CompletionId) const
{
	if (CompletionId.IsNone())
	{
		return false;
	}

	return CompletedExplorationInteractionIds.Contains(CompletionId);
}

void UJargonGameInstance::PrepareReturnToExploration()
{
	bReturningFromCombat = true;
	SaveCurrentRunIfActive();
}

void UJargonGameInstance::HandleCombatVictory()
{
	HandleCombatVictory(FJargonCurrencyAmount(), 0);
}

void UJargonGameInstance::HandleCombatDefeat()
{
	HandleCombatDefeat(FJargonCurrencyAmount(), 0);
}

void UJargonGameInstance::HandleCombatVictory(const FJargonCurrencyAmount& EnemyKillCurrency, int32 EnemiesDefeated)
{
	const FJargonCurrencyAmount VictoryBonusCurrency = PendingEncounterData.VictoryCurrencyReward;
	const FJargonCurrencyAmount TotalCurrencyEarned = CombineCurrencyAmounts(EnemyKillCurrency, VictoryBonusCurrency);

	AddCurrency(TotalCurrencyEarned);
	StorePostCombatReport(EJargonPostCombatResult::Victory, EnemyKillCurrency, VictoryBonusCurrency, EnemiesDefeated);
	PrepareReturnToExploration();
}

void UJargonGameInstance::HandleCombatDefeat(const FJargonCurrencyAmount& EnemyKillCurrency, int32 EnemiesDefeated)
{
	const FJargonCurrencyAmount VictoryBonusCurrency = FJargonCurrencyAmount();
	const FJargonCurrencyAmount TotalCurrencyEarned = EnemyKillCurrency;

	AddCurrency(TotalCurrencyEarned);
	StorePostCombatReport(EJargonPostCombatResult::Defeat, EnemyKillCurrency, VictoryBonusCurrency, EnemiesDefeated);
	PrepareReturnToExploration();
}

void UJargonGameInstance::ClearPendingPostCombatReport()
{
	bHasPendingPostCombatReport = false;
	PendingPostCombatReport.Reset();
	SaveCurrentRunIfActive();
}

void UJargonGameInstance::CompleteReturnToExploration()
{
	CompletePostCombatReturn();
}

void UJargonGameInstance::CompletePostCombatReturn()
{
	bReturningFromCombat = false;
	ClearPendingEncounter();
	SaveCurrentRunIfActive();
}

void UJargonGameInstance::ClearPendingEncounter()
{
	PendingEncounterData.Reset();
	SaveCurrentRunIfActive();
}

FName UJargonGameInstance::GetPostCombatDestinationMapName() const
{
	return ReturnMapName;
}

TArray<UCardDefinition*> UJargonGameInstance::ConvertCardArray(const TArray<TObjectPtr<UCardDefinition>>& SourceCards)
{
	TArray<UCardDefinition*> ConvertedCards;
	ConvertedCards.Reserve(SourceCards.Num());

	for (UCardDefinition* Card : SourceCards)
	{
		if (Card)
		{
			ConvertedCards.Add(Card);
		}
	}

	return ConvertedCards;
}

bool UJargonGameInstance::RemoveCardFromCollection(TArray<TObjectPtr<UCardDefinition>>& CardCollection, UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	return CardCollection.RemoveSingle(Card) > 0;
}

int32 UJargonGameInstance::CountUniqueNonNeutralElements(const TArray<TObjectPtr<UCardDefinition>>& CardCollection) const
{
	return GatherUniqueNonNeutralElements(CardCollection).Num();
}

TArray<EJargonElementType> UJargonGameInstance::GatherUniqueNonNeutralElements(const TArray<TObjectPtr<UCardDefinition>>& CardCollection) const
{
	TSet<EJargonElementType> UniqueElements;

	for (const TObjectPtr<UCardDefinition>& Card : CardCollection)
	{
		if (!Card || Card->CardElement == EJargonElementType::None)
		{
			continue;
		}

		UniqueElements.Add(Card->CardElement);
	}

	TArray<EJargonElementType> SortedElements = UniqueElements.Array();
	SortedElements.Sort([](const EJargonElementType Left, const EJargonElementType Right)
	{
		return static_cast<uint8>(Left) < static_cast<uint8>(Right);
	});

	return SortedElements;
}

bool UJargonGameInstance::DoesCardCollectionRespectElementLimit(const TArray<TObjectPtr<UCardDefinition>>& CardCollection) const
{
	return CountUniqueNonNeutralElements(CardCollection) <= GetMaxRunDeckElements();
}

void UJargonGameInstance::SetRunDeckInternal(const TArray<UCardDefinition*>& InitialDeck)
{
	ActiveRunDeck.Reset();
	RunOwnedCards.Reset();

	for (UCardDefinition* Card : InitialDeck)
	{
		if (Card)
		{
			ActiveRunDeck.Add(Card);
			RunOwnedCards.Add(Card);
		}
	}

	if (ActiveRunDeck.Num() > GetMaxRunDeckSize())
	{
		UE_LOG(LogJargon, Warning, TEXT("Run deck initialized with %d cards, exceeding MaxRunDeckSize=%d. Existing contents are preserved, but additional deck adds are blocked until the deck is under the limit."),
			ActiveRunDeck.Num(),
			GetMaxRunDeckSize());
	}

	if (!DoesCardCollectionRespectElementLimit(ActiveRunDeck))
	{
		UE_LOG(LogJargon, Warning, TEXT("Run deck initialized with %d unique non-neutral elements, exceeding MaxRunDeckElements=%d. Existing contents are preserved, but additional deck adds are blocked until the deck is within the element limit."),
			GetRunDeckElementCount(),
			GetMaxRunDeckElements());
	}
}

void UJargonGameInstance::NormalizeRunCurrencies()
{
	RunCurrencies.Normalize();
}

void UJargonGameInstance::StorePostCombatReport(
	EJargonPostCombatResult Result,
	const FJargonCurrencyAmount& EnemyKillCurrency,
	const FJargonCurrencyAmount& VictoryBonusCurrency,
	int32 EnemiesDefeated
)
{
	PendingPostCombatReport.Reset();
	PendingPostCombatReport.Result = Result;
	PendingPostCombatReport.EncounterId = PendingEncounterData.EncounterId;
	PendingPostCombatReport.EnemiesDefeated = FMath::Max(0, EnemiesDefeated);
	PendingPostCombatReport.EnemyKillCurrency = EnemyKillCurrency;
	PendingPostCombatReport.EnemyKillCurrency.Normalize();
	PendingPostCombatReport.VictoryBonusCurrency = VictoryBonusCurrency;
	PendingPostCombatReport.VictoryBonusCurrency.Normalize();
	PendingPostCombatReport.TotalCurrencyEarned = CombineCurrencyAmounts(
		PendingPostCombatReport.EnemyKillCurrency,
		PendingPostCombatReport.VictoryBonusCurrency
	);
	bHasPendingPostCombatReport = (Result != EJargonPostCombatResult::None);
	SaveCurrentRunIfActive();
}

int32 UJargonGameInstance::CountCardCopiesInCollection(
	const TArray<TObjectPtr<UCardDefinition>>& Collection,
	const UCardDefinition* Card) const
{
	if (!Card)
	{
		return 0;
	}

	int32 Count = 0;

	for (const TObjectPtr<UCardDefinition>& CardDefinition : Collection)
	{
		if (CardDefinition == Card)
		{
			Count++;
		}
	}

	return Count;
}

int32 UJargonGameInstance::GetUsefulOwnedRunCardCopyFloor(const UCardDefinition* Card) const
{
	return Card
		? FMath::Max(GetMaxCopiesPerDeckCard(), GetRunDeckCardCopyCount(Card))
		: GetMaxCopiesPerDeckCard();
}

int32 UJargonGameInstance::GetRecyclableOwnedRunCardCopyCount(const UCardDefinition* Card) const
{
	if (!Card)
	{
		return 0;
	}

	return FMath::Max(0, GetOwnedRunCardCopyCount(Card) - GetUsefulOwnedRunCardCopyFloor(Card));
}

void UJargonGameInstance::GatherRecyclableExtraOwnedRunCardCopies(TArray<UCardDefinition*>& OutCards) const
{
	OutCards.Reset();

	TMap<UCardDefinition*, int32> CopiesToRecycleByCard;

	for (UCardDefinition* OwnedCard : RunOwnedCards)
	{
		if (!OwnedCard)
		{
			continue;
		}

		int32* CopiesToRecyclePtr = CopiesToRecycleByCard.Find(OwnedCard);
		if (!CopiesToRecyclePtr)
		{
			CopiesToRecyclePtr = &CopiesToRecycleByCard.Add(OwnedCard, GetRecyclableOwnedRunCardCopyCount(OwnedCard));
		}

		if (*CopiesToRecyclePtr > 0)
		{
			OutCards.Add(OwnedCard);
			--(*CopiesToRecyclePtr);
		}
	}
}

void UJargonGameInstance::SeedDefaultClassArtifactForActiveHero()
{
	if (!ActiveHeroDefinition)
	{
		UE_LOG(LogJargon, Warning, TEXT("Cannot seed a default class Artifact because no active hero definition is assigned."));
		return;
	}

	UJargonArtifactDefinition* DefaultClassArtifact = ActiveHeroDefinition->DefaultClassArtifact;
	if (!DefaultClassArtifact)
	{
		UE_LOG(LogJargon, Warning, TEXT("Hero '%s' has no DefaultClassArtifact; class Artifact hooks will be empty for this run."),
			*GetNameSafe(ActiveHeroDefinition.Get()));
		return;
	}

	if (!DefaultClassArtifact->IsEligibleForHeroDefinition(ActiveHeroDefinition))
	{
		UE_LOG(LogJargon, Warning, TEXT("Hero '%s' default class Artifact '%s' is not eligible for that hero class and was not seeded."),
			*GetNameSafe(ActiveHeroDefinition.Get()),
			*GetNameSafe(DefaultClassArtifact));
		return;
	}

	if (HasRunArtifact(DefaultClassArtifact))
	{
		return;
	}

	RunArtifacts.Insert(DefaultClassArtifact, 0);
	UE_LOG(LogJargon, Log, TEXT("Seeded default class Artifact '%s' for hero '%s'."),
		*GetNameSafe(DefaultClassArtifact),
		*GetNameSafe(ActiveHeroDefinition.Get()));
}

void UJargonGameInstance::ClearRuntimeRunState(bool bResetHeroDefinition)
{
	bHasActiveRun = false;
	ActiveRunDeck.Reset();
	RunOwnedCards.Reset();
	RunReserveCards.Reset();
	RunArtifacts.Reset();
	RunCurrencies = FJargonCurrencyAmount();
	PendingEncounterData.Reset();
	bReturningFromCombat = false;
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;
	ClearedEncounterIds.Reset();
	CompletedExplorationInteractionIds.Reset();
	bHasPendingPostCombatReport = false;
	PendingPostCombatReport.Reset();

	if (bResetHeroDefinition && ActiveHeroDefinition)
	{
		ActiveHeroDefinition = nullptr;
		OnActiveHeroDefinitionChanged.Broadcast(nullptr);
	}
}

UJargonSaveIndex* UJargonGameInstance::LoadOrCreateSaveIndex() const
{
	if (UGameplayStatics::DoesSaveGameExist(SaveIndexSlotName, RunSaveUserIndex))
	{
		if (UJargonSaveIndex* SaveIndex = Cast<UJargonSaveIndex>(
			UGameplayStatics::LoadGameFromSlot(SaveIndexSlotName, RunSaveUserIndex)))
		{
			SaveIndex->NextSaveSlotNumber = FMath::Max(1, SaveIndex->NextSaveSlotNumber);
			return SaveIndex;
		}

		UE_LOG(LogJargon, Warning, TEXT("Save index slot '%s' exists but could not be loaded as UJargonSaveIndex. A fresh index will be used."), *SaveIndexSlotName);
	}

	UJargonSaveIndex* SaveIndex = Cast<UJargonSaveIndex>(
		UGameplayStatics::CreateSaveGameObject(UJargonSaveIndex::StaticClass()));
	if (!SaveIndex)
	{
		UE_LOG(LogJargon, Warning, TEXT("Failed to create save index object."));
	}

	return SaveIndex;
}

bool UJargonGameInstance::SaveSaveIndex(UJargonSaveIndex* SaveIndex) const
{
	if (!SaveIndex)
	{
		return false;
	}

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveIndex, SaveIndexSlotName, RunSaveUserIndex);
	UE_CLOG(!bSaved, LogJargon, Warning, TEXT("Failed to save run save index slot '%s'."), *SaveIndexSlotName);
	return bSaved;
}

bool UJargonGameInstance::AdoptLegacySaveSlot(UJargonSaveIndex* SaveIndex) const
{
	if (!SaveIndex || LegacyRunSaveSlotName.IsEmpty())
	{
		return false;
	}

	if (!UGameplayStatics::DoesSaveGameExist(LegacyRunSaveSlotName, RunSaveUserIndex))
	{
		return false;
	}

	if (SaveIndex->SaveSlotNames.Contains(LegacyRunSaveSlotName))
	{
		return false;
	}

	SaveIndex->SaveSlotNames.Add(LegacyRunSaveSlotName);
	return true;
}

bool UJargonGameInstance::RegisterSaveSlot(const FString& SaveSlotName)
{
	if (SaveSlotName.IsEmpty())
	{
		return false;
	}

	UJargonSaveIndex* SaveIndex = LoadOrCreateSaveIndex();
	if (!SaveIndex)
	{
		return false;
	}

	bool bIndexChanged = AdoptLegacySaveSlot(SaveIndex);
	if (!SaveIndex->SaveSlotNames.Contains(SaveSlotName))
	{
		SaveIndex->SaveSlotNames.Add(SaveSlotName);
		bIndexChanged = true;
	}

	return !bIndexChanged || SaveSaveIndex(SaveIndex);
}

bool UJargonGameInstance::UnregisterSaveSlot(const FString& SaveSlotName)
{
	if (SaveSlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SaveIndexSlotName, RunSaveUserIndex))
	{
		return true;
	}

	UJargonSaveIndex* SaveIndex = LoadOrCreateSaveIndex();
	if (!SaveIndex)
	{
		return false;
	}

	const int32 RemovedCount = SaveIndex->SaveSlotNames.Remove(SaveSlotName);
	return RemovedCount <= 0 || SaveSaveIndex(SaveIndex);
}

FString UJargonGameInstance::GenerateNewSaveSlotName(UJargonSaveIndex* SaveIndex) const
{
	if (!SaveIndex || RunSaveSlotPrefix.IsEmpty())
	{
		return FString();
	}

	int32 CandidateNumber = FMath::Max(1, SaveIndex->NextSaveSlotNumber);
	for (int32 Attempt = 0; Attempt < 10000; ++Attempt)
	{
		const FString CandidateSlotName = FString::Printf(TEXT("%s%d"), *RunSaveSlotPrefix, CandidateNumber);
		++CandidateNumber;

		if (!SaveIndex->SaveSlotNames.Contains(CandidateSlotName) &&
			!UGameplayStatics::DoesSaveGameExist(CandidateSlotName, RunSaveUserIndex))
		{
			SaveIndex->NextSaveSlotNumber = CandidateNumber;
			SaveSaveIndex(SaveIndex);
			return CandidateSlotName;
		}
	}

	UE_LOG(LogJargon, Warning, TEXT("Failed to generate a unique run save slot name with prefix '%s'."), *RunSaveSlotPrefix);
	return FString();
}

FJargonSaveSlotSummary UJargonGameInstance::BuildSaveSlotSummary(const FString& SaveSlotName) const
{
	FJargonSaveSlotSummary Summary;
	Summary.SlotName = SaveSlotName;

	UJargonSaveGame* SaveGame = Cast<UJargonSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, RunSaveUserIndex));
	if (!SaveGame || !SaveGame->bHasActiveRun)
	{
		return Summary;
	}

	FString ClassName = SaveGame->HeroClassName;
	if (ClassName.IsEmpty() && SaveGame->HeroClass != EJargonHeroClass::None)
	{
		ClassName = GetHeroClassDisplayName(SaveGame->HeroClass).ToString();
	}

	if (ClassName.IsEmpty())
	{
		const UJargonHeroDefinition* HeroDefinition = SaveGame->ActiveHeroDefinition.LoadSynchronous();
		ClassName = BuildHeroClassName(HeroDefinition);
	}

	const FDateTime SaveTime = SaveGame->TimeOfSave.GetTicks() > 0
		? SaveGame->TimeOfSave
		: GetSaveFileTimestamp(SaveSlotName);

	Summary.ClassName = FText::FromString(ClassName.IsEmpty() ? FString(TEXT("Unknown")) : ClassName);
	Summary.TimeOfSave = SaveTime;
	Summary.TimeOfSaveText = SaveTime.GetTicks() > 0
		? FText::FromString(SaveTime.ToString(TEXT("%Y-%m-%d %H:%M")))
		: FText::FromString(TEXT("Unknown"));
	Summary.CurrencyAmount = SaveGame->RunCurrencies;
	Summary.CurrencyAmount.Normalize();
	Summary.CurrencyText = FormatCurrencyAmount(Summary.CurrencyAmount);
	Summary.bIsValid = true;
	return Summary;
}

FString UJargonGameInstance::BuildActiveHeroClassName() const
{
	return BuildHeroClassName(ActiveHeroDefinition);
}

FString UJargonGameInstance::BuildHeroClassName(const UJargonHeroDefinition* HeroDefinition) const
{
	if (!HeroDefinition)
	{
		return TEXT("Unknown");
	}

	if (HeroDefinition->HeroClass != EJargonHeroClass::None)
	{
		return GetHeroClassDisplayName(HeroDefinition->HeroClass).ToString();
	}

	if (!HeroDefinition->DisplayName.IsEmpty())
	{
		return HeroDefinition->DisplayName.ToString();
	}

	return TEXT("Unknown");
}

FText UJargonGameInstance::GetHeroClassDisplayName(EJargonHeroClass HeroClass)
{
	const UEnum* HeroClassEnum = StaticEnum<EJargonHeroClass>();
	return HeroClassEnum
		? HeroClassEnum->GetDisplayNameTextByValue(static_cast<int64>(HeroClass))
		: FText::AsNumber(static_cast<int32>(HeroClass));
}

FText UJargonGameInstance::FormatCurrencyAmount(FJargonCurrencyAmount CurrencyAmount)
{
	CurrencyAmount.Normalize();

	TArray<FString> Parts;
	if (CurrencyAmount.Gold > 0)
	{
		Parts.Add(FString::Printf(TEXT("%d Gold"), CurrencyAmount.Gold));
	}

	if (CurrencyAmount.Silver > 0)
	{
		Parts.Add(FString::Printf(TEXT("%d Silver"), CurrencyAmount.Silver));
	}

	if (CurrencyAmount.Copper > 0 || Parts.Num() == 0)
	{
		Parts.Add(FString::Printf(TEXT("%d Copper"), CurrencyAmount.Copper));
	}

	return FText::FromString(FString::Join(Parts, TEXT(" ")));
}

FDateTime UJargonGameInstance::GetSaveFileTimestamp(const FString& SaveSlotName) const
{
	if (SaveSlotName.IsEmpty())
	{
		return FDateTime();
	}

	return IFileManager::Get().GetTimeStamp(*GetSaveGameFilePath(SaveSlotName));
}

FString UJargonGameInstance::GetSaveGameFilePath(const FString& SaveSlotName) const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), SaveSlotName + TEXT(".sav"));
}

void UJargonGameInstance::SaveCurrentRunIfActive()
{
	if (bHasActiveRun)
	{
		SaveCurrentRun();
	}
}
