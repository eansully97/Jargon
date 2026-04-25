#include "Town/JargonTownPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Core/JargonGameInstance.h"
#include "Town/UI/CardShopWidget.h"
#include "Town/UI/DeckEditWidget.h"
#include "Town/UI/TownHUDWidget.h"

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
	RefreshAllTownUI();
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

void AJargonTownPlayerController::SetTownInputModeGameOnly()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;
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
	if (!CardShopWidget && CardShopWidgetClass)
	{
		CardShopWidget = CreateWidget<UCardShopWidget>(this, CardShopWidgetClass);
	}

	if (!CardShopWidget)
	{
		return;
	}

	CloseDeckEdit();

	if (!CardShopWidget->IsInViewport())
	{
		CardShopWidget->AddToViewport(20);
	}

	SetWorldClickMovementEnabled(false);
	
	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	CardShopWidget->RefreshFromRunState(JargonGI);

	ActiveModalWidget = CardShopWidget;
	SetTownInputModeUI(CardShopWidget);
}

void AJargonTownPlayerController::CloseCardShop()
{
	if (CardShopWidget && CardShopWidget->IsInViewport())
	{
		CardShopWidget->RemoveFromParent();
	}

	if (ActiveModalWidget == CardShopWidget)
	{
		ActiveModalWidget = nullptr;
		SetTownInputModeGameOnly();
		SetWorldClickMovementEnabled(true);
	}
}

void AJargonTownPlayerController::OpenDeckEdit()
{
	if (!DeckEditWidget && DeckEditWidgetClass)
	{
		DeckEditWidget = CreateWidget<UDeckEditWidget>(this, DeckEditWidgetClass);
	}

	if (!DeckEditWidget)
	{
		return;
	}

	CloseCardShop();

	if (!DeckEditWidget->IsInViewport())
	{
		DeckEditWidget->AddToViewport(20);
	}

	SetWorldClickMovementEnabled(false);

	UJargonGameInstance* JargonGI = GetGameInstance<UJargonGameInstance>();
	DeckEditWidget->RefreshFromRunState(JargonGI);

	ActiveModalWidget = DeckEditWidget;
	SetTownInputModeUI(DeckEditWidget);
}

void AJargonTownPlayerController::CloseDeckEdit()
{
	if (DeckEditWidget && DeckEditWidget->IsInViewport())
	{
		DeckEditWidget->RemoveFromParent();
	}

	if (ActiveModalWidget == DeckEditWidget)
	{
		ActiveModalWidget = nullptr;
		SetTownInputModeGameOnly();
		SetWorldClickMovementEnabled(true);
	}
}

void AJargonTownPlayerController::CloseActiveTownPanel()
{
	if (ActiveModalWidget == CardShopWidget)
	{
		CloseCardShop();
		if (!ActiveModalWidget)
		{
			SetWorldClickMovementEnabled(true);
		}
		return;
	}

	if (ActiveModalWidget == DeckEditWidget)
	{
		CloseDeckEdit();
		if (!ActiveModalWidget)
		{
			SetWorldClickMovementEnabled(true);
		}
		return;
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