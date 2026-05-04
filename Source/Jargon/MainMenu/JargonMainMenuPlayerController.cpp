#include "MainMenu/JargonMainMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "MainMenu/JargonMainMenuWidget.h"

AJargonMainMenuPlayerController::AJargonMainMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	MainMenuWidgetClass = UJargonMainMenuWidget::StaticClass();
}

void AJargonMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!MainMenuWidget && MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UJargonMainMenuWidget>(this, MainMenuWidgetClass);
	}

	if (MainMenuWidget && !MainMenuWidget->IsInViewport())
	{
		MainMenuWidget->AddToViewport(0);
	}

	FInputModeUIOnly InputMode;
	if (MainMenuWidget)
	{
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
