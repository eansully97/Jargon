#include "Town/UI/TownHUDWidget.h"

#include "Components/TextBlock.h"
#include "Core/JargonGameInstance.h"
#include "Core/JargonRunStateTypes.h"

void UTownHUDWidget::RefreshFromRunState(UJargonGameInstance* JargonGameInstance)
{
	if (!JargonGameInstance)
	{
		return;
	}

	const auto [Gold, Silver, Copper] = JargonGameInstance->GetRunCurrencies();

	if (GoldText)
	{
		GoldText->SetText(FText::AsNumber(Gold));
	}

	if (SilverText)
	{
		SilverText->SetText(FText::AsNumber(Silver));
	}

	if (CopperText)
	{
		CopperText->SetText(FText::AsNumber(Copper));
	}
}