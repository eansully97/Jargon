#include "MainMenu/JargonMainMenuGameMode.h"

#include "MainMenu/JargonMainMenuPlayerController.h"

AJargonMainMenuGameMode::AJargonMainMenuGameMode()
{
	PlayerControllerClass = AJargonMainMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}
