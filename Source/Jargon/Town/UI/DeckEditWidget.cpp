#include "Town/UI/DeckEditWidget.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"

void UDeckEditWidget::RefreshFromRunState(UJargonGameInstance* JargonGameInstance)
{
	RunDeckCards.Empty();
	RunReserveCards.Empty();

	if (JargonGameInstance)
	{
		RunDeckCards = JargonGameInstance->GetRunDeckCards();
		RunReserveCards = JargonGameInstance->GetRunReserveCards();
	}

	BP_OnDeckDataRefreshed();
}

bool UDeckEditWidget::MoveCardFromReserveToDeck(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card)
{
	if (!JargonGameInstance || !Card)
	{
		return false;
	}

	const bool bMovedSuccessfully = JargonGameInstance->MoveCardFromReserveToDeck(Card);
	if (bMovedSuccessfully)
	{
		RefreshFromRunState(JargonGameInstance);
	}

	return bMovedSuccessfully;
}

bool UDeckEditWidget::MoveCardFromDeckToReserve(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card)
{
	if (!JargonGameInstance || !Card)
	{
		return false;
	}

	const bool bMovedSuccessfully = JargonGameInstance->MoveCardFromDeckToReserve(Card);
	if (bMovedSuccessfully)
	{
		RefreshFromRunState(JargonGameInstance);
	}

	return bMovedSuccessfully;
}