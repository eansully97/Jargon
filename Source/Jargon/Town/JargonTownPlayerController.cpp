#include "Town/JargonTownPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Combat/Widgets/CombatHoverInfoWidget.h"
#include "Core/JargonGameInstance.h"
#include "Jargon.h"
#include "Town/Widgets/CardShopWidget.h"
#include "Town/Widgets/DeckEditWidget.h"
#include "Town/Widgets/PostMatchReportWidget.h"
#include "Town/Widgets/TownHUDWidget.h"

AJargonTownPlayerController::AJargonTownPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AJargonTownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CreateTownHUD();
	InitializeTownHoverInfoWidget();
	RefreshAllTownUI();

	TryOpenPendingPostCombatReport();

	if (!HasBlockingModalOpen())
	{
		ApplyTownModalInputState();
	}
}

void AJargonTownPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindAction(TEXT("OpenShop"), IE_Pressed, this, &AJargonTownPlayerController::HandleOpenShopPressed);
	InputComponent->BindAction(TEXT("OpenDeckEdit"), IE_Pressed, this, &AJargonTownPlayerController::HandleOpenDeckEditPressed);
	InputComponent->BindAction(TEXT("CloseTownPanel"), IE_Pressed, this, &AJargonTownPlayerController::HandleCloseTownPanelPressed);
}

void AJargonTownPlayerController::CreateTownHUD()
{
	if (TownHUDWidget || !TownHUDWidgetClass)
	{
		return;
	}

	TownHUDWidget = CreateWidget<UTownHUDWidget>(this, TownHUDWidgetClass);
	if (TownHUDWidget)
	{
		TownHUDWidget->AddToViewport(0);
	}
}

void AJargonTownPlayerController::InitializeTownHoverInfoWidget()
{
	if (TownHoverInfoWidget || !TownHoverInfoWidgetClass)
	{
		return;
	}

	TownHoverInfoWidget = CreateWidget<UCombatHoverInfoWidget>(this, TownHoverInfoWidgetClass);
	if (!TownHoverInfoWidget)
	{
		UE_LOG(LogJargon, Warning, TEXT("TownPlayerController failed to create TownHoverInfoWidget."));
		return;
	}

	TownHoverInfoWidget->AddToViewport(TownHoverInfoWidgetZOrder);
	
	PositionTownHoverInfoWidgetAtMouse();
	TownHoverInfoWidget->SetHoverInfo(CurrentTownHoverInfo);
}

void AJargonTownPlayerController::RefreshTownHUD()
{
	if (!TownHUDWidget)
	{
		return;
	}

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	if (!JargonGI)
	{
		return;
	}

	TownHUDWidget->RefreshFromRunState(JargonGI);
}

void AJargonTownPlayerController::RestoreTownWorldInputNextTick()
{
	SetWorldClickMovementEnabled(false);
	FlushPressedKeys();
	SetTownInputModeGameOnly();

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (HasBlockingModalOpen())
		{
			SetWorldClickMovementEnabled(false);
			FlushPressedKeys();
			return;
		}

		SetWorldClickMovementEnabled(true);
		FlushPressedKeys();
	}));
}

void AJargonTownPlayerController::SetTownInputModeGameOnly()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
	bShowMouseCursor = true;

	FlushPressedKeys();
}

void AJargonTownPlayerController::SetTownInputModeUI(UUserWidget* FocusWidget)
{
	FInputModeGameAndUI InputMode;
	if (FocusWidget)
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}

	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

UUserWidget* AJargonTownPlayerController::GetTopmostTownModalWidget() const
{
	if (PostMatchReportWidget && PostMatchReportWidget->IsInViewport())
	{
		return PostMatchReportWidget;
	}

	if (DeckEditWidget && DeckEditWidget->IsInViewport())
	{
		return DeckEditWidget;
	}

	if (CardShopWidget && CardShopWidget->IsInViewport())
	{
		return CardShopWidget;
	}

	return nullptr;
}

void AJargonTownPlayerController::ApplyTownModalInputState(UUserWidget* PreferredFocusWidget)
{
	UUserWidget* FocusWidget = PreferredFocusWidget ? PreferredFocusWidget : GetTopmostTownModalWidget();

	if (HasBlockingModalOpen())
	{
		ActiveModalWidget = FocusWidget;

		SetWorldClickMovementEnabled(false);
		FlushPressedKeys();
		SetTownInputModeUI(FocusWidget);
		return;
	}

	ActiveModalWidget = nullptr;
	RestoreTownWorldInputNextTick();
}

void AJargonTownPlayerController::HideCardShopWithoutInputUpdate()
{
	if (CardShopWidget && CardShopWidget->IsInViewport())
	{
		CardShopWidget->RemoveFromParent();
	}
}

void AJargonTownPlayerController::HideDeckEditWithoutInputUpdate()
{
	if (DeckEditWidget && DeckEditWidget->IsInViewport())
	{
		DeckEditWidget->RemoveFromParent();
		SetCurrentTownHoverInfo(FJargonCombatHoverInfo());
	}
}

void AJargonTownPlayerController::HidePostMatchReportWithoutInputUpdate()
{
	if (PostMatchReportWidget && PostMatchReportWidget->IsInViewport())
	{
		PostMatchReportWidget->RemoveFromParent();
	}
}

void AJargonTownPlayerController::PositionTownHoverInfoWidgetAtMouse()
{
	if (!TownHoverInfoWidget)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D MouseScreenPosition(MouseX, MouseY);
	const FVector2D Offset(16.0f, 16.0f);
	TownHoverInfoWidget->SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
	TownHoverInfoWidget->SetPositionInViewport(MouseScreenPosition + Offset, true);
}

bool AJargonTownPlayerController::HasBlockingModalOpen() const
{
	const bool bShopOpen = CardShopWidget && CardShopWidget->IsInViewport();
	const bool bDeckEditOpen = DeckEditWidget && DeckEditWidget->IsInViewport();
	const bool bPostMatchReportOpen = PostMatchReportWidget && PostMatchReportWidget->IsInViewport();

	return bShopOpen || bDeckEditOpen || bPostMatchReportOpen;
}

void AJargonTownPlayerController::HandleOpenShopPressed()
{
	if (CardShopWidget && CardShopWidget->IsInViewport())
	{
		CloseCardShop();
		return;
	}

	OpenCardShop();
}

void AJargonTownPlayerController::HandleOpenDeckEditPressed()
{
	if (DeckEditWidget && DeckEditWidget->IsInViewport())
	{
		CloseDeckEdit();
		return;
	}

	OpenDeckEdit();
}

void AJargonTownPlayerController::HandleCloseTownPanelPressed()
{
	CloseActiveTownPanel();
}

void AJargonTownPlayerController::OpenCardShop()
{
	SetWorldClickMovementEnabled(false);
	FlushPressedKeys();

	if (!CardShopWidget && CardShopWidgetClass)
	{
		CardShopWidget = CreateWidget<UCardShopWidget>(this, CardShopWidgetClass);
	}

	if (!CardShopWidget)
	{
		UE_LOG(LogJargon, Warning, TEXT("OpenCardShop failed because CardShopWidgetClass is not assigned."));
		ApplyTownModalInputState();
		return;
	}

	HideDeckEditWithoutInputUpdate();
	HidePostMatchReportWithoutInputUpdate();

	if (!CardShopWidget->IsInViewport())
	{
		CardShopWidget->AddToViewport(20);
	}

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	CardShopWidget->RefreshFromRunState(JargonGI);

	ApplyTownModalInputState(CardShopWidget);
}

void AJargonTownPlayerController::CloseCardShop()
{
	HideCardShopWithoutInputUpdate();

	if (ActiveModalWidget == CardShopWidget)
	{
		ActiveModalWidget = nullptr;
	}

	ApplyTownModalInputState();
}

void AJargonTownPlayerController::OpenDeckEdit()
{
	SetWorldClickMovementEnabled(false);
	FlushPressedKeys();

	if (!DeckEditWidget && DeckEditWidgetClass)
	{
		DeckEditWidget = CreateWidget<UDeckEditWidget>(this, DeckEditWidgetClass);
	}

	if (!DeckEditWidget)
	{
		UE_LOG(LogJargon, Warning, TEXT("OpenDeckEdit failed because DeckEditWidgetClass is not assigned."));
		ApplyTownModalInputState();
		return;
	}

	HideCardShopWithoutInputUpdate();
	HidePostMatchReportWithoutInputUpdate();

	if (!DeckEditWidget->IsInViewport())
	{
		DeckEditWidget->AddToViewport(20);
	}

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	DeckEditWidget->RefreshFromRunState(JargonGI);

	ApplyTownModalInputState(DeckEditWidget);
}

void AJargonTownPlayerController::SetCurrentTownHoverInfo(const FJargonCombatHoverInfo& NewHoverInfo)
{
	const bool bChanged =
		CurrentTownHoverInfo.bHasInfo != NewHoverInfo.bHasInfo ||
		CurrentTownHoverInfo.InfoType != NewHoverInfo.InfoType ||
		CurrentTownHoverInfo.SourceActor != NewHoverInfo.SourceActor ||
		CurrentTownHoverInfo.SourceObject != NewHoverInfo.SourceObject ||
		CurrentTownHoverInfo.DescriptionText.ToString() != NewHoverInfo.DescriptionText.ToString();

	if (!bChanged)
	{
		return;
	}

	CurrentTownHoverInfo = NewHoverInfo;
	InitializeTownHoverInfoWidget();

	if (TownHoverInfoWidget)
	{
		if (CurrentTownHoverInfo.bHasInfo)
		{
			PositionTownHoverInfoWidgetAtMouse();
		}

		TownHoverInfoWidget->SetHoverInfo(CurrentTownHoverInfo);
	}

	OnTownHoverInfoChanged.Broadcast(CurrentTownHoverInfo);
}

void AJargonTownPlayerController::ShowTownHoverInfo(const FJargonCombatHoverInfo& HoverInfo)
{
	if (!bEnableTownHoverInfo)
	{
		SetCurrentTownHoverInfo(FJargonCombatHoverInfo());
		return;
	}

	SetCurrentTownHoverInfo(HoverInfo);
}

void AJargonTownPlayerController::ClearTownHoverInfo(UObject* SourceObject)
{
	if (!SourceObject || CurrentTownHoverInfo.SourceObject == SourceObject)
	{
		SetCurrentTownHoverInfo(FJargonCombatHoverInfo());
	}
}

void AJargonTownPlayerController::CloseDeckEdit()
{
	HideDeckEditWithoutInputUpdate();

	if (ActiveModalWidget == DeckEditWidget)
	{
		ActiveModalWidget = nullptr;
	}

	ApplyTownModalInputState();
}

void AJargonTownPlayerController::OpenPostMatchReport(const FJargonPostCombatReportData& ReportData)
{
	SetWorldClickMovementEnabled(false);
	FlushPressedKeys();

	if (!PostMatchReportWidget && PostMatchReportWidgetClass)
	{
		PostMatchReportWidget = CreateWidget<UPostMatchReportWidget>(this, PostMatchReportWidgetClass);
	}

	if (!PostMatchReportWidget)
	{
		UE_LOG(LogJargon, Warning, TEXT("OpenPostMatchReport failed because PostMatchReportWidgetClass is not assigned."));
		ApplyTownModalInputState();
		return;
	}

	HideCardShopWithoutInputUpdate();
	HideDeckEditWithoutInputUpdate();

	if (!PostMatchReportWidget->IsInViewport())
	{
		PostMatchReportWidget->AddToViewport(30);
	}

	PostMatchReportWidget->RefreshFromReportData(ReportData);

	ApplyTownModalInputState(PostMatchReportWidget);
}

void AJargonTownPlayerController::ClosePostMatchReport()
{
	HidePostMatchReportWithoutInputUpdate();

	if (ActiveModalWidget == PostMatchReportWidget)
	{
		ActiveModalWidget = nullptr;
	}

	ApplyTownModalInputState();
}

void AJargonTownPlayerController::CloseActiveTownPanel()
{
	if (PostMatchReportWidget && PostMatchReportWidget->IsInViewport())
	{
		HidePostMatchReportWithoutInputUpdate();
	}
	else if (DeckEditWidget && DeckEditWidget->IsInViewport())
	{
		HideDeckEditWithoutInputUpdate();
	}
	else if (CardShopWidget && CardShopWidget->IsInViewport())
	{
		HideCardShopWithoutInputUpdate();
	}

	ActiveModalWidget = nullptr;
	ApplyTownModalInputState();
}

void AJargonTownPlayerController::TryOpenPendingPostCombatReport()
{
	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	if (!JargonGI || !JargonGI->HasPendingPostCombatReport())
	{
		return;
	}

	if (!PostMatchReportWidgetClass)
	{
		UE_LOG(LogJargon, Warning, TEXT("Pending post-combat report was available, but PostMatchReportWidgetClass is not assigned."));
		return;
	}

	const FJargonPostCombatReportData ReportData = JargonGI->GetPendingPostCombatReport();
	OpenPostMatchReport(ReportData);

	if (PostMatchReportWidget && PostMatchReportWidget->IsInViewport())
	{
		JargonGI->ClearPendingPostCombatReport();
	}
}

void AJargonTownPlayerController::RefreshAllTownUI()
{
	RefreshTownHUD();

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	if (!JargonGI)
	{
		return;
	}

	if (CardShopWidget && CardShopWidget->IsInViewport())
	{
		CardShopWidget->RefreshFromRunState(JargonGI);
	}

	if (DeckEditWidget && DeckEditWidget->IsInViewport())
	{
		DeckEditWidget->RefreshFromRunState(JargonGI);
	}
}
