#include "CardPackDefinition.h"

#include "Data/CardDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

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

#if WITH_EDITOR
EDataValidationResult UCardPackDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (Price.Gold < 0 || Price.Silver < 0 || Price.Copper < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("Price contains a negative denomination. Gold=%d Silver=%d Copper=%d."), Price.Gold, Price.Silver, Price.Copper));
	}

	if (AmountToGrant <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("AmountToGrant must be greater than 0. Current value: %d."), AmountToGrant));
	}

	if (CardPool.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("CardPool is empty."));
	}

	int32 ValidEntryCount = 0;
	TSet<const UCardDefinition*> UniqueCards;
	for (int32 EntryIndex = 0; EntryIndex < CardPool.Num(); ++EntryIndex)
	{
		const FWeightedCardPackEntry& Entry = CardPool[EntryIndex];
		if (!Entry.CardDefinition)
		{
			JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("CardPool entry %d has no CardDefinition."), EntryIndex));
			continue;
		}

		if (Entry.Weight <= 0)
		{
			JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("CardPool entry %d has weight <= 0."), EntryIndex));
			continue;
		}

		ValidEntryCount++;
		UniqueCards.Add(Entry.CardDefinition);
		if (!Entry.CardDefinition->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("CardPool entry %d references card '%s' that failed IsValidDefinition()."), EntryIndex, *GetNameSafe(Entry.CardDefinition)));
		}
	}

	if (!bAllowDuplicateCardsPerPurchase && AmountToGrant > UniqueCards.Num())
	{
		JargonDataAssetValidation::AddWarning(Context, this, FString::Printf(TEXT("AmountToGrant=%d but only %d unique valid cards are present; purchases may grant fewer cards."), AmountToGrant, UniqueCards.Num()));
	}

	if (ValidEntryCount <= 0 && CardPool.Num() > 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("No valid rollable CardPool entries."));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

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
