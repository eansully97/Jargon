#include "CardDisplayWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/CardDefinition.h"
#include "Engine/Texture2D.h"

void UCardDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshFromCardDefinition();
}

void UCardDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromCardDefinition();
}

void UCardDisplayWidget::SetCardDefinition(UCardDefinition* InCardDefinition)
{
	CardDefinition = InCardDefinition;
	RefreshFromCardDefinition();
}

void UCardDisplayWidget::RefreshFromCardDefinition()
{
	if (!CardDefinition)
	{
		if (CostText)
		{
			CostText->SetText(FText::FromString(TEXT("-")));
		}

		if (CardNameText)
		{
			CardNameText->SetText(EmptyCardNameText);
		}

		if (TypeText)
		{
			TypeText->SetText(FText::GetEmpty());
		}

		if (DescriptionText)
		{
			DescriptionText->SetText(EmptyDescriptionText);
		}

		if (CardArt)
		{
			CardArt->SetBrushFromTexture(nullptr);
		}

		BP_OnCardDisplayRefreshed();
		return;
	}

	if (CostText)
	{
		CostText->SetText(FText::AsNumber(CardDefinition->Cost));
	}

	if (CardNameText)
	{
		CardNameText->SetText(CardDefinition->DisplayName);
	}

	if (TypeText)
	{
		TypeText->SetText(GetCardCategoryText());
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(CardDefinition->Description);
	}

	if (CardArt)
	{
		CardArt->SetBrushFromTexture(CardDefinition->CardArt);
	}

	BP_OnCardDisplayRefreshed();
}

FText UCardDisplayWidget::GetCardCategoryText() const
{
	if (!CardDefinition)
	{
		return FText::GetEmpty();
	}

	const UEnum* CategoryEnum = StaticEnum<ECardCategory>();
	if (!CategoryEnum)
	{
		return FText::GetEmpty();
	}

	return CategoryEnum->GetDisplayNameTextByValue(static_cast<int64>(CardDefinition->Category));
}