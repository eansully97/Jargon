// JargonCombatPlayerController.cpp

#include "Combat/JargonCombatPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Data/CardDefinition.h"
#include "Units/BattleUnit.h"
#include "Combat/JargonCombatGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Grid/GridTile.h"
#include "InputCoreTypes.h"
#include "Widgets/CombatHUDWidget.h"

AJargonCombatPlayerController::AJargonCombatPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AJargonCombatPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeCombatUI();
	InitializeStartingDeck();
}

void AJargonCombatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AJargonCombatPlayerController::HandleLeftClick);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AJargonCombatPlayerController::HandleRightClick);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AJargonCombatPlayerController::HandleCancelSelection);
		InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AJargonCombatPlayerController::HandleEndTurnInput);
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AJargonCombatPlayerController::HandleEndTurnInput);
	}
}

void AJargonCombatPlayerController::InitializeCombatUI()
{
	if (CombatHUD)
	{
		return;
	}

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatPlayerController could not initialize HUD because CombatGameMode was null."));
		return;
	}

	const TSubclassOf<UCombatHUDWidget> HUDClass = CombatGameMode->GetCombatHUDClass();
	if (!HUDClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatPlayerController could not initialize HUD because CombatHUDClass was not assigned."));
		return;
	}

	CombatHUD = CreateWidget<UCombatHUDWidget>(this, HUDClass);
	if (!CombatHUD)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatPlayerController failed to create CombatHUD widget."));
		return;
	}

	CombatHUD->AddToViewport();
	CombatHUD->OnHandCardClicked().AddUObject(this, &AJargonCombatPlayerController::SelectCard);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(CombatHUD->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	RefreshHUD();
}

void AJargonCombatPlayerController::InitializeStartingDeck()
{
	DrawPile.Reset();
	Hand.Reset();
	DiscardPile.Reset();
	SelectedCard = nullptr;
	bCardTargetingMode = false;

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatPlayerController could not initialize starting deck because CombatGameMode was null."));
		RefreshHUD();
		return;
	}

	for (UCardDefinition* Card : CombatGameMode->GetStartingDeckDefinitions())
	{
		if (Card)
		{
			DrawPile.Add(Card);
		}
	}

	DrawCards(CombatGameMode->GetStartingHandSize());
	RefreshHUD();
}

void AJargonCombatPlayerController::DrawCards(int32 Count)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (DrawPile.Num() == 0)
		{
			break;
		}

		UCardDefinition* DrawnCard = DrawPile[0];
		DrawPile.RemoveAt(0);

		if (DrawnCard)
		{
			Hand.Add(DrawnCard);
		}
	}

	RefreshHUD();
}

void AJargonCombatPlayerController::SelectCard(UCardDefinition* Card)
{
	if (SelectedCard == Card)
	{
		ClearSelectedCard();
		return;
	}

	SelectedCard = Card;
	bCardTargetingMode = (SelectedCard != nullptr);
	RefreshHUD();
}

void AJargonCombatPlayerController::ClearSelectedCard()
{
	SelectedCard = nullptr;
	bCardTargetingMode = false;
	RefreshHUD();
}

void AJargonCombatPlayerController::RemoveCardFromHand(UCardDefinition* Card)
{
	if (!Card)
	{
		return;
	}

	const int32 RemovedCount = Hand.RemoveSingle(Card);
	if (RemovedCount > 0)
	{
		DiscardPile.Add(Card);
	}

	RefreshHUD();
}

void AJargonCombatPlayerController::RequestMoveToTile(AGridTile* Tile)
{
	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode || !Tile)
	{
		return;
	}

	CombatGameMode->TryMovePlayerUnitToTile(Tile);
}

void AJargonCombatPlayerController::RequestPlayCardOnUnit(ABattleUnit* Unit)
{
	if (!SelectedCard || !Unit)
	{
		return;
	}

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	const bool bPlayedSuccessfully = CombatGameMode->TryPlayCardOnTarget(SelectedCard, Unit);
	if (bPlayedSuccessfully)
	{
		RemoveCardFromHand(SelectedCard);
		ClearSelectedCard();
	}
}

void AJargonCombatPlayerController::RequestEndTurn()
{
	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	CombatGameMode->RequestEndPlayerTurn();
}

void AJargonCombatPlayerController::HandleLeftClick()
{
	FHitResult HitResult;
	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (!bHit)
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return;
	}

	if (ABattleUnit* HitUnit = Cast<ABattleUnit>(HitActor))
	{
		if (SelectedCard)
		{
			RequestPlayCardOnUnit(HitUnit);
		}

		return;
	}

	if (AGridTile* HitTile = Cast<AGridTile>(HitActor))
	{
		if (!SelectedCard)
		{
			RequestMoveToTile(HitTile);
		}

		return;
	}
}

void AJargonCombatPlayerController::HandleRightClick()
{
	HandleCancelSelection();
}

void AJargonCombatPlayerController::HandleCancelSelection()
{
	if (SelectedCard)
	{
		ClearSelectedCard();
	}
}

void AJargonCombatPlayerController::HandleEndTurnInput()
{
	RequestEndTurn();
}

void AJargonCombatPlayerController::RefreshHUD()
{
	if (!CombatHUD)
	{
		return;
	}

	CombatHUD->RefreshHand(Hand);
	CombatHUD->SetSelectedCard(SelectedCard);
}