#include "Core/JargonGameInstance.h"

#include "Data/CardDefinition.h"
#include "Progression/CardPackDefinition.h"

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
	bHasActiveRun = true;
}

void UJargonGameInstance::EnsureRunInitializedFromSeedDeck(const TArray<TObjectPtr<UCardDefinition>>& SeedDeck)
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
	RunReserveCards.Reset();
	RunCurrencies = FJargonCurrencyAmount();
	bHasActiveRun = ActiveRunDeck.Num() > 0;
}

void UJargonGameInstance::ResetRunState()
{
	bHasActiveRun = false;
	ActiveRunDeck.Reset();
	RunReserveCards.Reset();
	RunCurrencies = FJargonCurrencyAmount();
	PendingEncounterData.Reset();
	bReturnToTownAfterCombat = false;
	bReturningFromCombat = false;
	ReturnMapName = NAME_None;
	ReturnTransform = FTransform::Identity;
	ClearedEncounterIds.Reset();
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
	TArray<UCardDefinition*> OwnedCards = ConvertCardArray(ActiveRunDeck);
	OwnedCards.Append(ConvertCardArray(RunReserveCards));
	return OwnedCards;
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
		return false;
	}

	if (!RemoveCardFromCollection(RunReserveCards, Card))
	{
		return false;
	}

	ActiveRunDeck.Add(Card);
	return true;
}

bool UJargonGameInstance::MoveCardFromDeckToReserve(UCardDefinition* Card)
{
	if (!bHasActiveRun || !Card)
	{
		return false;
	}

	if (!RemoveCardFromCollection(ActiveRunDeck, Card))
	{
		return false;
	}

	RunReserveCards.Add(Card);
	return true;
}

void UJargonGameInstance::SetTownMapName(const FName& InTownMapName)
{
	TownMapName = InTownMapName;
}

void UJargonGameInstance::SetAvailableCardPackOffers(const TArray<UCardPackDefinition*>& InPackOffers)
{
	AvailableCardPackOffers.Reset();

	for (UCardPackDefinition* PackOffer : InPackOffers)
	{
		if (PackOffer)
		{
			AvailableCardPackOffers.Add(PackOffer);
		}
	}
}

TArray<UCardPackDefinition*> UJargonGameInstance::GetAvailableCardPackOffers() const
{
	TArray<UCardPackDefinition*> PackOffers;
	PackOffers.Reserve(AvailableCardPackOffers.Num());

	for (UCardPackDefinition* PackOffer : AvailableCardPackOffers)
	{
		if (PackOffer)
		{
			PackOffers.Add(PackOffer);
		}
	}

	return PackOffers;
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
			RunReserveCards.Add(GrantedCard);
		}
	}

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
	AddCurrency(PendingEncounterData.VictoryCurrencyReward);
	PrepareReturnToTownAfterCombat();
}

void UJargonGameInstance::HandleCombatDefeat()
{
	PrepareReturnToTownAfterCombat();
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

void UJargonGameInstance::SetRunDeckInternal(const TArray<UCardDefinition*>& InitialDeck)
{
	ActiveRunDeck.Reset();

	for (UCardDefinition* Card : InitialDeck)
	{
		if (Card)
		{
			ActiveRunDeck.Add(Card);
		}
	}
}

void UJargonGameInstance::NormalizeRunCurrencies()
{
	RunCurrencies.Normalize();
}
