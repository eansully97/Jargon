#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JargonMainMenuPlayerController.generated.h"

class UJargonMainMenuWidget;

UCLASS()
class JARGON_API AJargonMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJargonMainMenuPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Main Menu")
	TSubclassOf<UJargonMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UJargonMainMenuWidget> MainMenuWidget = nullptr;
};
