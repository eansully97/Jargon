#include "CardPackDefinition.h"

#include "Data/CardDefinition.h"

namespace
{
	UCardDefinition* RollSingleWeightedCard(const TArray<FWeightedCardPackEntry>& Entries)
	{
		int32 TotalWeight = 0;
		for (const FWeightedCardPackEntry& Entry : Entries)
		{
			if (Entry.IsValid())
			{
				TotalWeight += Entry.Weight;
			}
		}

		if (TotalWeight <= 0)
		{
			return nullptr;
		}

		int32 Roll = FMath::RandRange(1, TotalWeight);
		for (const FWeightedCardPackEntry& Entry : Entries)
		{
			if (!Entry.IsValid())
			{
				continue;
			}

			Roll -= Entry.Weight;
			if (Roll <= 0)
			{
				return Entry.CardDefinition;
			}
		}

		return nullptr;
	}
}

bool UCardPackDefinition::IsValidDefinition() const
{
	if (MinCardsGranted <= 0 || MaxCardsGranted < MinCardsGranted)
	{
		return false;
	}

	for (const FWeightedCardPackEntry& Entry : CardPool)
	{
		if (Entry.IsValid())
		{
			return true;
		}
	}

	return false;
}

bool UCardPackDefinition::RollGrantedCards(TArray<UCardDefinition*>& OutGrantedCards) const
{
	OutGrantedCards.Reset();

	if (!IsValidDefinition())
	{
		return false;
	}

	TArray<FWeightedCardPackEntry> AvailableEntries;
	for (const FWeightedCardPackEntry& Entry : CardPool)
	{
		if (Entry.IsValid())
		{
			AvailableEntries.Add(Entry);
		}
	}

	if (AvailableEntries.Num() == 0)
	{
		return false;
	}

	const int32 CardsToGrant = FMath::Clamp(
		FMath::RandRange(MinCardsGranted, MaxCardsGranted),
		1,
		bAllowDuplicateCardsPerPurchase ? MaxCardsGranted : AvailableEntries.Num());

	for (int32 CardIndex = 0; CardIndex < CardsToGrant; ++CardIndex)
	{
		UCardDefinition* GrantedCard = RollSingleWeightedCard(AvailableEntries);
		if (!GrantedCard)
		{
			break;
		}

		OutGrantedCards.Add(GrantedCard);

		if (!bAllowDuplicateCardsPerPurchase)
		{
			AvailableEntries.RemoveAllSwap([GrantedCard](const FWeightedCardPackEntry& Entry)
			{
				return Entry.CardDefinition == GrantedCard;
			});

			if (AvailableEntries.Num() == 0)
			{
				break;
			}
		}
	}

	return OutGrantedCards.Num() > 0;
}
