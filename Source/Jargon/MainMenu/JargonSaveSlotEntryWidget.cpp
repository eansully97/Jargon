#include "MainMenu/JargonSaveSlotEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
void ApplySaveEntryTextStyle(UTextBlock* TextBlock, int32 FontSize)
{
	if (!TextBlock)
	{
		return;
	}

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TextBlock->SetJustification(ETextJustify::Center);
}
}

void UJargonSaveSlotEntryWidget::SetSaveSlotSummary(const FJargonSaveSlotSummary& InSummary)
{
	SaveSlotSummary = InSummary;
	RefreshVisuals();
}

TSharedRef<SWidget> UJargonSaveSlotEntryWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		SaveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveButton"));
		UVerticalBox* ButtonContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveButtonContent"));
		ClassNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClassNameText"));
		TimeOfSaveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimeOfSaveText"));
		CurrencyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrencyText"));

		ApplySaveEntryTextStyle(ClassNameText, 20);
		ApplySaveEntryTextStyle(TimeOfSaveText, 14);
		ApplySaveEntryTextStyle(CurrencyText, 14);

		if (UVerticalBoxSlot* ClassSlot = ButtonContent->AddChildToVerticalBox(ClassNameText))
		{
			ClassSlot->SetPadding(FMargin(8.f, 8.f, 8.f, 2.f));
			ClassSlot->SetHorizontalAlignment(HAlign_Center);
		}

		if (UVerticalBoxSlot* TimeSlot = ButtonContent->AddChildToVerticalBox(TimeOfSaveText))
		{
			TimeSlot->SetPadding(FMargin(8.f, 2.f, 8.f, 2.f));
			TimeSlot->SetHorizontalAlignment(HAlign_Center);
		}

		if (UVerticalBoxSlot* CurrencySlot = ButtonContent->AddChildToVerticalBox(CurrencyText))
		{
			CurrencySlot->SetPadding(FMargin(8.f, 2.f, 8.f, 8.f));
			CurrencySlot->SetHorizontalAlignment(HAlign_Center);
		}

		SaveButton->SetContent(ButtonContent);
		WidgetTree->RootWidget = SaveButton;
	}

	return Super::RebuildWidget();
}

void UJargonSaveSlotEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SaveButton)
	{
		SaveButton->OnClicked.RemoveAll(this);
		SaveButton->OnClicked.AddDynamic(this, &UJargonSaveSlotEntryWidget::HandleSaveButtonClicked);
	}

	RefreshVisuals();
}

void UJargonSaveSlotEntryWidget::HandleSaveButtonClicked()
{
	if (!SaveSlotSummary.bIsValid || SaveSlotSummary.SlotName.IsEmpty())
	{
		return;
	}

	OnSaveSlotEntryClicked.Broadcast(SaveSlotSummary);
}

void UJargonSaveSlotEntryWidget::RefreshVisuals()
{
	if (ClassNameText)
	{
		ClassNameText->SetText(SaveSlotSummary.ClassName.IsEmpty() ? FText::FromString(TEXT("Unknown")) : SaveSlotSummary.ClassName);
	}

	if (TimeOfSaveText)
	{
		TimeOfSaveText->SetText(SaveSlotSummary.TimeOfSaveText.IsEmpty() ? FText::FromString(TEXT("Unknown")) : SaveSlotSummary.TimeOfSaveText);
	}

	if (CurrencyText)
	{
		CurrencyText->SetText(SaveSlotSummary.CurrencyText.IsEmpty() ? FText::FromString(TEXT("0 Copper")) : SaveSlotSummary.CurrencyText);
	}

	if (SaveButton)
	{
		SaveButton->SetIsEnabled(SaveSlotSummary.bIsValid && !SaveSlotSummary.SlotName.IsEmpty());
		SaveButton->SetToolTipText(FText::FromString(SaveSlotSummary.SlotName));
	}
}
