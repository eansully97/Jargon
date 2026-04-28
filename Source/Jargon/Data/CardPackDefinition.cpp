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
	if (Price.GetTotalCopperValue() < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' invalid: negative price."), *GetNameSafe(this));
		return false;
	}

	if (AmountToGrant <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' invalid: AmountToGrant must be greater than 0."), *GetNameSafe(this));
		return false;
	}

	if (CardPool.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' invalid: no possible cards."), *GetNameSafe(this));
		return false;
	}

	int32 ValidEntryCount = 0;
	int32 TotalValidWeight = 0;

	for (int32 Index = 0; Index < CardPool.Num(); ++Index)
	{
		const FWeightedCardPackEntry& Entry = CardPool[Index];

		if (!Entry.CardDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' warning: entry %d has null CardDefinition and will be skipped."),
				*GetNameSafe(this),
				Index);
			continue;
		}

		if (Entry.Weight <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' warning: entry %d card '%s' has invalid weight %d and will be skipped."),
				*GetNameSafe(this),
				Index,
				*GetNameSafe(Entry.CardDefinition),
				Entry.Weight);
			continue;
		}

		if (!Entry.CardDefinition->IsValidDefinition())
		{
			UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' warning: entry %d card '%s' failed CardDefinition validation and will be skipped."),
				*GetNameSafe(this),
				Index,
				*GetNameSafe(Entry.CardDefinition));
			continue;
		}

		ValidEntryCount++;
		TotalValidWeight += Entry.Weight;
	}

	if (ValidEntryCount <= 0 || TotalValidWeight <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' invalid: no valid rollable card entries."), *GetNameSafe(this));
		return false;
	}

	if (!bAllowDuplicateCardsPerPurchase && AmountToGrant > ValidEntryCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardPack '%s' warning: AmountToGrant=%d but only %d valid unique cards are available. Purchase may grant fewer cards."),
			*GetNameSafe(this),
			AmountToGrant,
			ValidEntryCount);
	}

	return true;
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
		if (Entry.CardDefinition && Entry.Weight > 0 && Entry.CardDefinition->IsValidDefinition())
		{
			AvailableEntries.Add(Entry);
		}
	}

	if (AvailableEntries.Num() == 0)
	{
		return false;
	}

	for (int32 CardIndex = 0; CardIndex < AmountToGrant; ++CardIndex)
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
