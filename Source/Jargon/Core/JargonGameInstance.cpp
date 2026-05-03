#include "Core/JargonGameInstance.h"

#include "Data/CardDefinition.h"
#include "Jargon.h"
#include "Data/CardPackDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonRelicDefinition.h"
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
}

UJargonGameInstance::UJargonGameInstance()
{
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;
	PendingEncounterData.Reset();
	bReturnToTownAfterCombat = false;
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
	bReturnToTownAfterCombat = false;
	bReturningFromCombat = false;
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
	bReturnToTownAfterCombat = false;
	bReturningFromCombat = false;
}

void UJargonGameInstance::StartNewRun(const TArray<UCardDefinition*>& InitialDeck, const FJargonCurrencyAmount& StartingCurrency)
{
	ResetRunState();
	SetRunDeckInternal(InitialDeck);
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
	RunCurrencies = FJargonCurrencyAmount();
	bHasActiveRun = ActiveRunDeck.Num() > 0;
	RefreshRunReserveCardsFromAvailableShopPacks();
}

void UJargonGameInstance::ResetRunState()
{
	bHasActiveRun = false;
	ActiveRunDeck.Reset();
	RunOwnedCards.Reset();
	RunReserveCards.Reset();
	RunRelics.Reset();
	RunCurrencies = FJargonCurrencyAmount();
	PendingEncounterData.Reset();
	bReturnToTownAfterCombat = false;
	bReturningFromCombat = false;
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;
	ClearedEncounterIds.Reset();
	CompletedExplorationInteractionIds.Reset();
	ClearPendingPostCombatReport();
}

void UJargonGameInstance::SetActiveHeroDefinition(UJargonHeroDefinition* HeroDefinition)
{
	if (ActiveHeroDefinition == HeroDefinition)
	{
		return;
	}

	ActiveHeroDefinition = HeroDefinition;
	OnActiveHeroDefinitionChanged.Broadcast(ActiveHeroDefinition);
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
	Summary.MaxElementCount = GetMaxRunDeckElements();
	Summary.ActiveElements = GetRunDeckElements();
	Summary.CurrentElementCount = Summary.ActiveElements.Num();
	Summary.bIsWithinLimit = Summary.CurrentElementCount <= Summary.MaxElementCount;

	for (const EJargonElementType Element : Summary.ActiveElements)
	{
		Summary.ActiveElementTexts.Add(GetCardElementDisplayText(Element));
	}

	if (Summary.ActiveElementTexts.Num() == 0)
	{
		Summary.SummaryText = FText::Format(
			NSLOCTEXT("JargonDeck", "DeckElementSummaryNeutralOnly", "Elements: Neutral only ({0} / {1})"),
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
			NSLOCTEXT("JargonDeck", "DeckElementSummaryWithElements", "Elements: {0} ({1} / {2})"),
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
	TArray<TObjectPtr<UCardDefinition>> CandidateDeck = ActiveRunDeck;
	if (Card)
	{
		CandidateDeck.Add(const_cast<UCardDefinition*>(Card));
	}

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
}

FJargonCurrencyAmount UJargonGameInstance::GetCardRecycleValue(const UCardDefinition* Card) const
{
	FJargonCurrencyAmount RecycleValue = Card ? CardRecycleValue : FJargonCurrencyAmount();
	RecycleValue.Normalize();
	return RecycleValue;
}

TArray<UJargonRelicDefinition*> UJargonGameInstance::GetRunRelics() const
{
	TArray<UJargonRelicDefinition*> Relics;
	Relics.Reserve(RunRelics.Num());

	for (UJargonRelicDefinition* RelicDefinition : RunRelics)
	{
		if (RelicDefinition)
		{
			Relics.Add(RelicDefinition);
		}
	}

	return Relics;
}

TArray<UJargonRelicDefinition*> UJargonGameInstance::GetRunBoons() const
{
	return GetRunRelics();
}

bool UJargonGameInstance::AddRunRelic(UJargonRelicDefinition* RelicDefinition)
{
	if (!RelicDefinition)
	{
		UE_LOG(LogJargon, Warning, TEXT("AddRunRelic rejected a null relic definition."));
		return false;
	}

	if (!bHasActiveRun)
	{
		UE_LOG(LogJargon, Warning, TEXT("AddRunRelic accepted '%s' while no active run is marked. It will still be stored until run state resets."),
			*GetNameSafe(RelicDefinition));
	}

	if (HasRunRelic(RelicDefinition))
	{
		UE_LOG(LogJargon, Log, TEXT("AddRunRelic skipped duplicate relic '%s'."), *GetNameSafe(RelicDefinition));
		return false;
	}

	RunRelics.Add(RelicDefinition);

	UE_LOG(LogJargon, Log, TEXT("Added run relic '%s'. Total relics=%d"),
		*GetNameSafe(RelicDefinition),
		RunRelics.Num());

	return true;
}

bool UJargonGameInstance::AddRunBoon(UJargonRelicDefinition* BoonDefinition)
{
	return AddRunRelic(BoonDefinition);
}

bool UJargonGameInstance::HasRunRelic(const UJargonRelicDefinition* RelicDefinition) const
{
	if (!RelicDefinition)
	{
		return false;
	}

	for (const TObjectPtr<UJargonRelicDefinition>& RunRelic : RunRelics)
	{
		if (RunRelic == RelicDefinition)
		{
			return true;
		}
	}

	return false;
}

bool UJargonGameInstance::HasRunBoon(const UJargonRelicDefinition* BoonDefinition) const
{
	return HasRunRelic(BoonDefinition);
}

bool UJargonGameInstance::TrySpendCurrency(const FJargonCurrencyAmount& Cost)
{
	if (!CanAffordCurrency(Cost))
	{
		return false;
	}

	RunCurrencies = FJargonCurrencyAmount::FromTotalCopper(
		RunCurrencies.GetTotalCopperValue() - Cost.GetTotalCopperValue());
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
		UE_LOG(LogJargon, Log, TEXT("MoveCardFromReserveToDeck rejected. Adding '%s' would exceed the max unique non-neutral deck element count. Current=%d Max=%d CardElement=%s"),
			*GetNameSafe(Card),
			GetRunDeckElementCount(),
			GetMaxRunDeckElements(),
			*Card->GetCardElementDisplayText().ToString());
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

	return true;
}

void UJargonGameInstance::SetTownMapName(const FName& InTownMapName)
{
	TownMapName = InTownMapName;
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

	return true;
}

void UJargonGameInstance::MarkEncounterCleared(const FName& EncounterId)
{
	if (!EncounterId.IsNone())
	{
		ClearedEncounterIds.Add(EncounterId);
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
	bReturnToTownAfterCombat = false;
	bReturningFromCombat = true;
}

void UJargonGameInstance::PrepareReturnToTownAfterCombat()
{
	bReturnToTownAfterCombat = true;
	bReturningFromCombat = true;
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
	PrepareReturnToTownAfterCombat();
}

void UJargonGameInstance::HandleCombatDefeat(const FJargonCurrencyAmount& EnemyKillCurrency, int32 EnemiesDefeated)
{
	const FJargonCurrencyAmount VictoryBonusCurrency = FJargonCurrencyAmount();
	const FJargonCurrencyAmount TotalCurrencyEarned = EnemyKillCurrency;

	AddCurrency(TotalCurrencyEarned);
	StorePostCombatReport(EJargonPostCombatResult::Defeat, EnemyKillCurrency, VictoryBonusCurrency, EnemiesDefeated);
	PrepareReturnToTownAfterCombat();
}

void UJargonGameInstance::ClearPendingPostCombatReport()
{
	bHasPendingPostCombatReport = false;
	PendingPostCombatReport.Reset();
}

void UJargonGameInstance::CompleteReturnToExploration()
{
	CompletePostCombatReturn();
}

void UJargonGameInstance::CompletePostCombatReturn()
{
	bReturnToTownAfterCombat = false;
	bReturningFromCombat = false;
	ClearPendingEncounter();
}

void UJargonGameInstance::ClearPendingEncounter()
{
	PendingEncounterData.Reset();
}

FName UJargonGameInstance::GetPostCombatDestinationMapName() const
{
	if (bReturnToTownAfterCombat && !TownMapName.IsNone())
	{
		return TownMapName;
	}

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
		UE_LOG(LogJargon, Warning, TEXT("Run deck initialized with %d unique non-neutral elements, exceeding MaxRunDeckElements=%d. Existing contents are preserved, but additional deck adds are blocked until the deck is under the limit."),
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
