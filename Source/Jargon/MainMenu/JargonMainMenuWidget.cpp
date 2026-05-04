#include "MainMenu/JargonMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Core/JargonGameInstance.h"
#include "Data/JargonHeroDefinition.h"
#include "Jargon.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MainMenu/JargonSaveSlotEntryWidget.h"

namespace
{
constexpr int32 MainMenuPageIndex = 0;
constexpr int32 SaveSelectPageIndex = 1;
constexpr int32 ClassSelectPageIndex = 2;
constexpr int32 SelectedSavePageIndex = 3;
constexpr int32 DeleteConfirmationPageIndex = 4;

const TCHAR* MageHeroDefinitionPath = TEXT("/Game/Jargon/Data/Hero/DA_Mage.DA_Mage");
const TCHAR* PaladinHeroDefinitionPath = TEXT("/Game/Jargon/Data/Hero/DA_Paladin.DA_Paladin");
const TCHAR* RogueHeroDefinitionPath = TEXT("/Game/Jargon/Data/Hero/DA_Rogue.DA_Rogue");

void ApplyMainMenuTextStyle(UTextBlock* TextBlock, int32 FontSize)
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

UTextBlock* CreateMainMenuText(UWidgetTree* WidgetTree, const FName WidgetName, const FText Text, int32 FontSize)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
	TextBlock->SetText(Text);
	ApplyMainMenuTextStyle(TextBlock, FontSize);
	return TextBlock;
}

UButton* CreateMainMenuButton(UWidgetTree* WidgetTree, const FName WidgetName, const FText Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
	UTextBlock* ButtonText = CreateMainMenuText(
		WidgetTree,
		*FString::Printf(TEXT("%sText"), *WidgetName.ToString()),
		Label,
		22);
	Button->SetContent(ButtonText);
	return Button;
}

void AddMenuChild(UVerticalBox* Parent, UWidget* Child, const FMargin Padding, EHorizontalAlignment HorizontalAlignment = HAlign_Center)
{
	if (!Parent || !Child)
	{
		return;
	}

	if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child))
	{
		Slot->SetPadding(Padding);
		Slot->SetHorizontalAlignment(HorizontalAlignment);
	}
}

USizeBox* WrapFixedButton(UWidgetTree* WidgetTree, UButton* Button, const FName WidgetName)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), WidgetName);
	SizeBox->SetWidthOverride(260.f);
	SizeBox->SetHeightOverride(54.f);
	SizeBox->SetContent(Button);
	return SizeBox;
}

FText BuildSaveSummaryText(const FJargonSaveSlotSummary& SaveSlotSummary)
{
	return FText::Format(
		NSLOCTEXT("JargonMainMenu", "SelectedSaveSummaryFormat", "{0}\n{1}\n{2}"),
		SaveSlotSummary.ClassName.IsEmpty() ? FText::FromString(TEXT("Unknown")) : SaveSlotSummary.ClassName,
		SaveSlotSummary.TimeOfSaveText.IsEmpty() ? FText::FromString(TEXT("Unknown")) : SaveSlotSummary.TimeOfSaveText,
		SaveSlotSummary.CurrencyText.IsEmpty() ? FText::FromString(TEXT("0 Copper")) : SaveSlotSummary.CurrencyText);
}
}

void UJargonMainMenuWidget::ShowMainPage()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(MainMenuPageIndex);
	}
}

void UJargonMainMenuWidget::ShowSaveSelectPage()
{
	RefreshSaveList();
	SelectedSaveSlotSummary = FJargonSaveSlotSummary();

	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(SaveSelectPageIndex);
	}
}

void UJargonMainMenuWidget::ShowClassSelectPage()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(ClassSelectPageIndex);
	}
}

void UJargonMainMenuWidget::RefreshSaveList()
{
	if (!SaveListScrollBox)
	{
		return;
	}

	SaveListScrollBox->ClearChildren();

	UWorld* World = GetWorld();
	UJargonGameInstance* JargonGameInstance = World ? World->GetGameInstance<UJargonGameInstance>() : nullptr;
	const TArray<FJargonSaveSlotSummary> SaveSummaries = JargonGameInstance
		? JargonGameInstance->GetSaveSlotSummaries()
		: TArray<FJargonSaveSlotSummary>();

	for (const FJargonSaveSlotSummary& SaveSummary : SaveSummaries)
	{
		UJargonSaveSlotEntryWidget* SaveEntry = CreateWidget<UJargonSaveSlotEntryWidget>(
			GetOwningPlayer(),
			UJargonSaveSlotEntryWidget::StaticClass());
		if (!SaveEntry)
		{
			continue;
		}

		SaveEntry->SetSaveSlotSummary(SaveSummary);
		SaveEntry->OnSaveSlotEntryClicked.AddUObject(this, &UJargonMainMenuWidget::HandleSaveSlotSelected);
		SaveListScrollBox->AddChild(SaveEntry);
	}

	if (EmptySaveListText)
	{
		EmptySaveListText->SetVisibility(SaveSummaries.Num() == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

TSharedRef<SWidget> UJargonMainMenuWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
		Background->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.025f, 1.f));

		if (UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background))
		{
			BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			BackgroundSlot->SetOffsets(FMargin(0.f));
		}

		PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("PageSwitcher"));

		if (UCanvasPanelSlot* PageSlot = RootCanvas->AddChildToCanvas(PageSwitcher))
		{
			PageSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PageSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PageSlot->SetSize(FVector2D(680.f, 620.f));
		}

		UVerticalBox* MainPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainPage"));
		AddMenuChild(MainPage, CreateMainMenuText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Jargon")), 54), FMargin(8.f, 72.f, 8.f, 48.f));

		PlayButton = CreateMainMenuButton(WidgetTree, TEXT("PlayButton"), FText::FromString(TEXT("Play")));
		AddMenuChild(MainPage, WrapFixedButton(WidgetTree, PlayButton, TEXT("PlayButtonBox")), FMargin(8.f, 8.f));

		QuitButton = CreateMainMenuButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quit")));
		AddMenuChild(MainPage, WrapFixedButton(WidgetTree, QuitButton, TEXT("QuitButtonBox")), FMargin(8.f, 8.f));

		UVerticalBox* SavePage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SavePage"));
		AddMenuChild(SavePage, CreateMainMenuText(WidgetTree, TEXT("SaveTitleText"), FText::FromString(TEXT("Saves")), 42), FMargin(8.f, 18.f, 8.f, 16.f));

		NewSaveButton = CreateMainMenuButton(WidgetTree, TEXT("NewSaveButton"), FText::FromString(TEXT("New Save")));
		AddMenuChild(SavePage, WrapFixedButton(WidgetTree, NewSaveButton, TEXT("NewSaveButtonBox")), FMargin(8.f, 6.f, 8.f, 18.f));

		EmptySaveListText = CreateMainMenuText(WidgetTree, TEXT("EmptySaveListText"), FText::FromString(TEXT("No saves found.")), 18);
		AddMenuChild(SavePage, EmptySaveListText, FMargin(8.f, 4.f, 8.f, 8.f));

		SaveListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SaveListScrollBox"));
		USizeBox* SaveListBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveListBox"));
		SaveListBox->SetWidthOverride(460.f);
		SaveListBox->SetHeightOverride(340.f);
		SaveListBox->SetContent(SaveListScrollBox);
		AddMenuChild(SavePage, SaveListBox, FMargin(8.f), HAlign_Center);

		BackButton = CreateMainMenuButton(WidgetTree, TEXT("BackButton"), FText::FromString(TEXT("Back")));
		AddMenuChild(SavePage, WrapFixedButton(WidgetTree, BackButton, TEXT("BackButtonBox")), FMargin(8.f, 18.f, 8.f, 8.f));

		UVerticalBox* ClassSelectPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ClassSelectPage"));
		AddMenuChild(ClassSelectPage, CreateMainMenuText(WidgetTree, TEXT("ClassSelectTitleText"), FText::FromString(TEXT("Choose Class")), 42), FMargin(8.f, 36.f, 8.f, 28.f));

		MageClassButton = CreateMainMenuButton(WidgetTree, TEXT("MageClassButton"), FText::FromString(TEXT("Mage")));
		AddMenuChild(ClassSelectPage, WrapFixedButton(WidgetTree, MageClassButton, TEXT("MageClassButtonBox")), FMargin(8.f, 8.f));

		PaladinClassButton = CreateMainMenuButton(WidgetTree, TEXT("PaladinClassButton"), FText::FromString(TEXT("Paladin")));
		AddMenuChild(ClassSelectPage, WrapFixedButton(WidgetTree, PaladinClassButton, TEXT("PaladinClassButtonBox")), FMargin(8.f, 8.f));

		RogueClassButton = CreateMainMenuButton(WidgetTree, TEXT("RogueClassButton"), FText::FromString(TEXT("Rogue")));
		AddMenuChild(ClassSelectPage, WrapFixedButton(WidgetTree, RogueClassButton, TEXT("RogueClassButtonBox")), FMargin(8.f, 8.f));

		ClassSelectBackButton = CreateMainMenuButton(WidgetTree, TEXT("ClassSelectBackButton"), FText::FromString(TEXT("Back")));
		AddMenuChild(ClassSelectPage, WrapFixedButton(WidgetTree, ClassSelectBackButton, TEXT("ClassSelectBackButtonBox")), FMargin(8.f, 18.f, 8.f, 8.f));

		UVerticalBox* SelectedSavePage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SelectedSavePage"));
		AddMenuChild(SelectedSavePage, CreateMainMenuText(WidgetTree, TEXT("SelectedSaveTitleText"), FText::FromString(TEXT("Save")), 42), FMargin(8.f, 36.f, 8.f, 18.f));

		SelectedSaveSummaryText = CreateMainMenuText(WidgetTree, TEXT("SelectedSaveSummaryText"), FText::GetEmpty(), 20);
		AddMenuChild(SelectedSavePage, SelectedSaveSummaryText, FMargin(8.f, 8.f, 8.f, 28.f));

		LoadSelectedSaveButton = CreateMainMenuButton(WidgetTree, TEXT("LoadSelectedSaveButton"), FText::FromString(TEXT("Load")));
		AddMenuChild(SelectedSavePage, WrapFixedButton(WidgetTree, LoadSelectedSaveButton, TEXT("LoadSelectedSaveButtonBox")), FMargin(8.f, 8.f));

		DeleteSelectedSaveButton = CreateMainMenuButton(WidgetTree, TEXT("DeleteSelectedSaveButton"), FText::FromString(TEXT("Delete")));
		AddMenuChild(SelectedSavePage, WrapFixedButton(WidgetTree, DeleteSelectedSaveButton, TEXT("DeleteSelectedSaveButtonBox")), FMargin(8.f, 8.f));

		CancelSelectedSaveButton = CreateMainMenuButton(WidgetTree, TEXT("CancelSelectedSaveButton"), FText::FromString(TEXT("Back")));
		AddMenuChild(SelectedSavePage, WrapFixedButton(WidgetTree, CancelSelectedSaveButton, TEXT("CancelSelectedSaveButtonBox")), FMargin(8.f, 18.f, 8.f, 8.f));

		UVerticalBox* ConfirmDeletePage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ConfirmDeletePage"));
		AddMenuChild(ConfirmDeletePage, CreateMainMenuText(WidgetTree, TEXT("ConfirmDeleteTitleText"), FText::FromString(TEXT("Are you sure?")), 42), FMargin(8.f, 48.f, 8.f, 20.f));

		DeleteSaveSummaryText = CreateMainMenuText(WidgetTree, TEXT("DeleteSaveSummaryText"), FText::GetEmpty(), 20);
		AddMenuChild(ConfirmDeletePage, DeleteSaveSummaryText, FMargin(8.f, 8.f, 8.f, 28.f));

		ConfirmDeleteButton = CreateMainMenuButton(WidgetTree, TEXT("ConfirmDeleteButton"), FText::FromString(TEXT("Delete")));
		AddMenuChild(ConfirmDeletePage, WrapFixedButton(WidgetTree, ConfirmDeleteButton, TEXT("ConfirmDeleteButtonBox")), FMargin(8.f, 8.f));

		CancelDeleteButton = CreateMainMenuButton(WidgetTree, TEXT("CancelDeleteButton"), FText::FromString(TEXT("Cancel")));
		AddMenuChild(ConfirmDeletePage, WrapFixedButton(WidgetTree, CancelDeleteButton, TEXT("CancelDeleteButtonBox")), FMargin(8.f, 8.f));

		PageSwitcher->AddChild(MainPage);
		PageSwitcher->AddChild(SavePage);
		PageSwitcher->AddChild(ClassSelectPage);
		PageSwitcher->AddChild(SelectedSavePage);
		PageSwitcher->AddChild(ConfirmDeletePage);
		PageSwitcher->SetActiveWidgetIndex(MainMenuPageIndex);

		WidgetTree->RootWidget = RootCanvas;
	}

	return Super::RebuildWidget();
}

void UJargonMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveAll(this);
		PlayButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandlePlayClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
		QuitButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleQuitClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveAll(this);
		BackButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleBackClicked);
	}

	if (NewSaveButton)
	{
		NewSaveButton->OnClicked.RemoveAll(this);
		NewSaveButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleNewSaveClicked);
	}

	if (MageClassButton)
	{
		MageClassButton->OnClicked.RemoveAll(this);
		MageClassButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleMageClassClicked);
	}

	if (PaladinClassButton)
	{
		PaladinClassButton->OnClicked.RemoveAll(this);
		PaladinClassButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandlePaladinClassClicked);
	}

	if (RogueClassButton)
	{
		RogueClassButton->OnClicked.RemoveAll(this);
		RogueClassButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleRogueClassClicked);
	}

	if (ClassSelectBackButton)
	{
		ClassSelectBackButton->OnClicked.RemoveAll(this);
		ClassSelectBackButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleClassSelectBackClicked);
	}

	if (LoadSelectedSaveButton)
	{
		LoadSelectedSaveButton->OnClicked.RemoveAll(this);
		LoadSelectedSaveButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleLoadSelectedSaveClicked);
	}

	if (DeleteSelectedSaveButton)
	{
		DeleteSelectedSaveButton->OnClicked.RemoveAll(this);
		DeleteSelectedSaveButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleDeleteSelectedSaveClicked);
	}

	if (CancelSelectedSaveButton)
	{
		CancelSelectedSaveButton->OnClicked.RemoveAll(this);
		CancelSelectedSaveButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleCancelSelectedSaveClicked);
	}

	if (ConfirmDeleteButton)
	{
		ConfirmDeleteButton->OnClicked.RemoveAll(this);
		ConfirmDeleteButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleConfirmDeleteClicked);
	}

	if (CancelDeleteButton)
	{
		CancelDeleteButton->OnClicked.RemoveAll(this);
		CancelDeleteButton->OnClicked.AddDynamic(this, &UJargonMainMenuWidget::HandleCancelDeleteClicked);
	}

	ShowMainPage();
}

void UJargonMainMenuWidget::HandlePlayClicked()
{
	ShowSaveSelectPage();
}

void UJargonMainMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UJargonMainMenuWidget::HandleBackClicked()
{
	ShowMainPage();
}

void UJargonMainMenuWidget::HandleNewSaveClicked()
{
	ShowClassSelectPage();
}

void UJargonMainMenuWidget::HandleMageClassClicked()
{
	StartNewSaveWithHeroDefinition(MageHeroDefinitionPath, TEXT("Mage"));
}

void UJargonMainMenuWidget::HandlePaladinClassClicked()
{
	StartNewSaveWithHeroDefinition(PaladinHeroDefinitionPath, TEXT("Paladin"));
}

void UJargonMainMenuWidget::HandleRogueClassClicked()
{
	StartNewSaveWithHeroDefinition(RogueHeroDefinitionPath, TEXT("Rogue"));
}

void UJargonMainMenuWidget::HandleClassSelectBackClicked()
{
	ShowSaveSelectPage();
}

void UJargonMainMenuWidget::HandleLoadSelectedSaveClicked()
{
	if (!SelectedSaveSlotSummary.bIsValid || SelectedSaveSlotSummary.SlotName.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	UJargonGameInstance* JargonGameInstance = World ? World->GetGameInstance<UJargonGameInstance>() : nullptr;
	if (!JargonGameInstance)
	{
		return;
	}

	if (JargonGameInstance->LoadSavedRunFromSlot(SelectedSaveSlotSummary.SlotName))
	{
		OpenTownMap();
	}
}

void UJargonMainMenuWidget::HandleDeleteSelectedSaveClicked()
{
	ShowDeleteConfirmationPage();
}

void UJargonMainMenuWidget::HandleCancelSelectedSaveClicked()
{
	ShowSaveSelectPage();
}

void UJargonMainMenuWidget::HandleConfirmDeleteClicked()
{
	if (!SelectedSaveSlotSummary.bIsValid || SelectedSaveSlotSummary.SlotName.IsEmpty())
	{
		ShowSaveSelectPage();
		return;
	}

	UWorld* World = GetWorld();
	UJargonGameInstance* JargonGameInstance = World ? World->GetGameInstance<UJargonGameInstance>() : nullptr;
	if (JargonGameInstance)
	{
		JargonGameInstance->DeleteSavedRunFromSlot(SelectedSaveSlotSummary.SlotName);
	}

	ShowSaveSelectPage();
}

void UJargonMainMenuWidget::HandleCancelDeleteClicked()
{
	ShowSelectedSavePage();
}

void UJargonMainMenuWidget::HandleSaveSlotSelected(const FJargonSaveSlotSummary& SaveSlotSummary)
{
	SelectedSaveSlotSummary = SaveSlotSummary;
	ShowSelectedSavePage();
}

void UJargonMainMenuWidget::ShowSelectedSavePage()
{
	ApplySelectedSaveSummaryText();

	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(SelectedSavePageIndex);
	}
}

void UJargonMainMenuWidget::ShowDeleteConfirmationPage()
{
	ApplySelectedSaveSummaryText();

	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(DeleteConfirmationPageIndex);
	}
}

void UJargonMainMenuWidget::ApplySelectedSaveSummaryText()
{
	const FText SummaryText = BuildSaveSummaryText(SelectedSaveSlotSummary);

	if (SelectedSaveSummaryText)
	{
		SelectedSaveSummaryText->SetText(SummaryText);
	}

	if (DeleteSaveSummaryText)
	{
		DeleteSaveSummaryText->SetText(SummaryText);
	}
}

void UJargonMainMenuWidget::StartNewSaveWithHeroDefinition(const TCHAR* HeroDefinitionPath, const TCHAR* ClassDisplayName)
{
	if (!HeroDefinitionPath || !ClassDisplayName)
	{
		UE_LOG(LogJargon, Warning, TEXT("Main menu could not start a new save because the selected class data was missing."));
		return;
	}

	const FSoftObjectPath HeroDefinitionObjectPath(HeroDefinitionPath);
	TSoftObjectPtr<UJargonHeroDefinition> HeroDefinitionReference(HeroDefinitionObjectPath);
	UJargonHeroDefinition* HeroDefinition = HeroDefinitionReference.LoadSynchronous();
	if (!HeroDefinition)
	{
		UE_LOG(LogJargon, Warning, TEXT("Main menu failed to load %s hero definition at '%s'."), ClassDisplayName, HeroDefinitionPath);
		return;
	}

	if (!HeroDefinition->IsValidDefinition())
	{
		UE_LOG(LogJargon, Warning, TEXT("Main menu found an invalid %s hero definition at '%s'."), ClassDisplayName, HeroDefinitionPath);
		return;
	}

	UWorld* World = GetWorld();
	UJargonGameInstance* JargonGameInstance = World ? World->GetGameInstance<UJargonGameInstance>() : nullptr;
	if (!JargonGameInstance)
	{
		UE_LOG(LogJargon, Warning, TEXT("Main menu could not start a %s save because the Jargon game instance was unavailable."), ClassDisplayName);
		return;
	}

	FString NewSaveSlotName;
	if (!JargonGameInstance->CreateNewSaveSlot(NewSaveSlotName))
	{
		UE_LOG(LogJargon, Warning, TEXT("Main menu could not create a new save slot for %s."), ClassDisplayName);
		return;
	}

	JargonGameInstance->SetActiveHeroDefinition(HeroDefinition);
	OpenTownMap();
}

void UJargonMainMenuWidget::OpenTownMap()
{
	const FName DestinationMapName = TownMapName.IsNone() ? FName(TEXT("L_TownMap")) : TownMapName;
	UGameplayStatics::OpenLevel(this, DestinationMapName);
}
