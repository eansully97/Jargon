#include "Data/JargonDeckDefinition.h"

#include "Data/CardDefinition.h"

#if WITH_EDITOR
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#endif

namespace
{
constexpr int32 RecommendedMaxDeckSize = 30;
constexpr int32 RecommendedMaxCopiesPerCard = 3;
constexpr int32 RecommendedMaxNonNeutralElements = 3;

FString GetCardElementName(EJargonElementType Element)
{
	if (Element == EJargonElementType::None)
	{
		return TEXT("Neutral");
	}

	const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
	return ElementEnum
		? ElementEnum->GetNameStringByValue(static_cast<int64>(Element))
		: FString::FromInt(static_cast<int32>(Element));
}

void GatherDeckStats(
	const TArray<TObjectPtr<UCardDefinition>>& Cards,
	int32& OutResolvedCount,
	TMap<const UCardDefinition*, int32>& OutCardCounts,
	TSet<EJargonElementType>& OutNonNeutralElements)
{
	OutResolvedCount = 0;
	OutCardCounts.Reset();
	OutNonNeutralElements.Reset();

	for (const TObjectPtr<UCardDefinition>& Card : Cards)
	{
		const UCardDefinition* CardDefinition = Card.Get();
		if (!CardDefinition)
		{
			continue;
		}

		++OutResolvedCount;
		OutCardCounts.FindOrAdd(CardDefinition)++;

		if (CardDefinition->CardElement != EJargonElementType::None)
		{
			OutNonNeutralElements.Add(CardDefinition->CardElement);
		}
	}
}
}

TArray<UCardDefinition*> UJargonDeckDefinition::GetResolvedCards() const
{
	TArray<UCardDefinition*> ResolvedCards;
	ResolvedCards.Reserve(Cards.Num());

	for (UCardDefinition* Card : Cards)
	{
		if (Card)
		{
			ResolvedCards.Add(Card);
		}
	}

	return ResolvedCards;
}

bool UJargonDeckDefinition::IsValidDefinition() const
{
	int32 ResolvedCount = 0;
	TMap<const UCardDefinition*, int32> CardCounts;
	TSet<EJargonElementType> NonNeutralElements;
	GatherDeckStats(Cards, ResolvedCount, CardCounts, NonNeutralElements);

	if (DisplayName.IsEmpty())
	{
		return false;
	}

	if (Cards.Num() <= 0 || ResolvedCount != Cards.Num())
	{
		return false;
	}

	if (ResolvedCount > RecommendedMaxDeckSize)
	{
		return false;
	}

	if (NonNeutralElements.Num() > RecommendedMaxNonNeutralElements)
	{
		return false;
	}

	for (const TPair<const UCardDefinition*, int32>& CardCount : CardCounts)
	{
		if (!CardCount.Key || !CardCount.Key->IsValidDefinition() || CardCount.Value > RecommendedMaxCopiesPerCard)
		{
			return false;
		}
	}

	return true;
}

FString UJargonDeckDefinition::GetAuditSummary() const
{
	int32 ResolvedCount = 0;
	TMap<const UCardDefinition*, int32> CardCounts;
	TSet<EJargonElementType> NonNeutralElements;
	GatherDeckStats(Cards, ResolvedCount, CardCounts, NonNeutralElements);

	TArray<FString> ElementNames;
	for (const EJargonElementType Element : NonNeutralElements)
	{
		ElementNames.Add(GetCardElementName(Element));
	}
	ElementNames.Sort();

	return FString::Printf(
		TEXT("DisplayName=%s Cards=%d ResolvedCards=%d UniqueCards=%d Elements=%s IsValidDefinition=%s"),
		DisplayName.IsEmpty() ? *GetNameSafe(this) : *DisplayName.ToString(),
		Cards.Num(),
		ResolvedCount,
		CardCounts.Num(),
		ElementNames.Num() > 0 ? *FString::Join(ElementNames, TEXT("|")) : TEXT("NeutralOnly"),
		IsValidDefinition() ? TEXT("true") : TEXT("false"));
}

#if WITH_EDITOR
EDataValidationResult UJargonDeckDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (Cards.Num() <= 0)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("Cards is empty."));
	}

	int32 ResolvedCount = 0;
	TMap<const UCardDefinition*, int32> CardCounts;
	TSet<EJargonElementType> NonNeutralElements;
	GatherDeckStats(Cards, ResolvedCount, CardCounts, NonNeutralElements);

	if (ResolvedCount > RecommendedMaxDeckSize)
	{
		JargonDataAssetValidation::AddError(
			Context,
			this,
			FString::Printf(TEXT("Deck has %d resolved cards, exceeding recommended max deck size %d."), ResolvedCount, RecommendedMaxDeckSize));
	}

	if (NonNeutralElements.Num() > RecommendedMaxNonNeutralElements)
	{
		TArray<FString> ElementNames;
		for (const EJargonElementType Element : NonNeutralElements)
		{
			ElementNames.Add(GetCardElementName(Element));
		}
		ElementNames.Sort();

		JargonDataAssetValidation::AddError(
			Context,
			this,
			FString::Printf(
				TEXT("Deck uses %d non-neutral elements (%s), exceeding the deck element limit of %d."),
				NonNeutralElements.Num(),
				*FString::Join(ElementNames, TEXT(", ")),
				RecommendedMaxNonNeutralElements));
	}

	for (int32 CardIndex = 0; CardIndex < Cards.Num(); ++CardIndex)
	{
		const UCardDefinition* Card = Cards[CardIndex].Get();
		if (!Card)
		{
			JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("Cards entry %d is null."), CardIndex));
			continue;
		}

		if (!Card->IsValidDefinition())
		{
			JargonDataAssetValidation::AddError(
				Context,
				this,
				FString::Printf(TEXT("Cards entry %d references invalid card '%s'."), CardIndex, *GetPathNameSafe(Card)));
		}
	}

	for (const TPair<const UCardDefinition*, int32>& CardCount : CardCounts)
	{
		if (CardCount.Value > RecommendedMaxCopiesPerCard)
		{
			JargonDataAssetValidation::AddError(
				Context,
				this,
				FString::Printf(
					TEXT("Card '%s' appears %d times, exceeding max copies per deck card %d."),
					*GetPathNameSafe(CardCount.Key),
					CardCount.Value,
					RecommendedMaxCopiesPerCard));
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
