#include "Town/UI/CardShopWidget.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Progression/CardPackDefinition.h"

void UCardShopWidget::RefreshFromRunState(UJargonGameInstance* JargonGameInstance)
{
	AvailablePackOffers.Empty();

	if (JargonGameInstance)
	{
		AvailablePackOffers = JargonGameInstance->GetAvailableCardPackOffers();
	}

	BP_OnShopDataRefreshed();
}

bool UCardShopWidget::PurchasePack(UJargonGameInstance* JargonGameInstance, UCardPackDefinition* PackDefinition)
{
	LastGrantedCards.Empty();
	LastFailureReason = FText::GetEmpty();

	if (!JargonGameInstance || !PackDefinition)
	{
		LastFailureReason = FText::FromString(TEXT("Invalid shop purchase request."));
		BP_OnPackPurchased(false, LastGrantedCards, LastFailureReason);
		return false;
	}

	TArray<UCardDefinition*> GrantedCards;
	FText FailureReason;

	const bool bPurchasedSuccessfully = JargonGameInstance->PurchaseCardPack(
		PackDefinition,
		GrantedCards,
		FailureReason
	);

	LastFailureReason = FailureReason;

	for (UCardDefinition* GrantedCard : GrantedCards)
	{
		if (GrantedCard)
		{
			LastGrantedCards.Add(GrantedCard);
		}
	}

	BP_OnPackPurchased(bPurchasedSuccessfully, GrantedCards, LastFailureReason);

	if (bPurchasedSuccessfully)
	{
		RefreshFromRunState(JargonGameInstance);
	}

	return bPurchasedSuccessfully;
}