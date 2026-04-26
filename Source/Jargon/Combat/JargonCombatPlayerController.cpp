// JargonCombatPlayerController.cpp

#include "Combat/JargonCombatPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Combat/JargonCombatGameMode.h"
#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "GameFramework/PlayerController.h"
#include "Grid/GridTile.h"
#include "InputCoreTypes.h"
#include "Units/BattleUnit.h"
#include "Units/PlayerBattleUnit.h"
#include "Widgets/CombatHUDWidget.h"

AJargonCombatPlayerController::AJargonCombatPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
	bStartingDeckInitialized = false;
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
	CombatHUD->OnEndTurnClicked().AddUObject(this, &AJargonCombatPlayerController::RequestEndTurn);

	CombatGameMode->OnPhaseChanged.RemoveDynamic(this, &AJargonCombatPlayerController::HandleCombatPhaseChanged);
	CombatGameMode->OnPhaseChanged.AddDynamic(this, &AJargonCombatPlayerController::HandleCombatPhaseChanged);

	CombatGameMode->OnEnergyChanged.RemoveDynamic(this, &AJargonCombatPlayerController::HandleCombatEnergyChanged);
	CombatGameMode->OnEnergyChanged.AddDynamic(this, &AJargonCombatPlayerController::HandleCombatEnergyChanged);

	CombatGameMode->OnPlayerActionAvailabilityChanged.RemoveDynamic(this, &AJargonCombatPlayerController::HandlePlayerActionAvailabilityChanged);
	CombatGameMode->OnPlayerActionAvailabilityChanged.AddDynamic(this, &AJargonCombatPlayerController::HandlePlayerActionAvailabilityChanged);

	RefreshCombatStateHUD();

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(CombatHUD->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	RefreshHUD();
	BroadcastCardCounts();
}

void AJargonCombatPlayerController::InitializeStartingDeck()
{
	if (bStartingDeckInitialized)
	{
		RefreshHUD();
		BroadcastCardCounts();
		return;
	}

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
		BroadcastCardCounts();
		return;
	}

	bool bLoadedPersistentRunDeck = false;

	if (UJargonGameInstance* GameInstance = GetGameInstance<UJargonGameInstance>())
	{
		GameInstance->EnsureRunInitializedFromSeedDeck(CombatGameMode->GetStartingDeckDefinitions());

		if (GameInstance->HasActiveRun())
		{
			const TArray<UCardDefinition*> RunDeckCards = GameInstance->GetRunDeckCards();

			UE_LOG(LogTemp, Warning, TEXT("Combat loading persistent run deck. Count: %d"), RunDeckCards.Num());

			if (RunDeckCards.Num() > 0)
			{
				for (UCardDefinition* Card : RunDeckCards)
				{
					if (Card)
					{
						UE_LOG(LogTemp, Warning, TEXT("Run deck card loaded: %s"), *GetNameSafe(Card));
						DrawPile.Add(Card);
					}
				}

				bLoadedPersistentRunDeck = true;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Active run deck was empty at combat start. Falling back to CombatGameMode starter deck."));
			}
		}
	}

	if (!bLoadedPersistentRunDeck)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat loading fallback starting deck."));

		for (UCardDefinition* Card : CombatGameMode->GetStartingDeckDefinitions())
		{
			if (Card)
			{
				DrawPile.Add(Card);
			}
		}
	}

	bStartingDeckInitialized = true;
	ShuffleDrawPile();
	DrawCards(CombatGameMode->GetStartingHandSize());
	RefreshHUD();
	BroadcastCardCounts();
}

void AJargonCombatPlayerController::ShuffleDrawPile()
{
	const int32 LastIndex = DrawPile.Num() - 1;

	for (int32 Index = LastIndex; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		DrawPile.Swap(Index, SwapIndex);
	}
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
	BroadcastCardCounts();
}

void AJargonCombatPlayerController::SelectCard(UCardDefinition* Card)
{
	if (!Card)
	{
		return;
	}

	if (SelectedCard == Card)
	{
		ClearSelectedCard();
		return;
	}

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	if (Card->TargetType == ECardTargetType::Self)
	{
		const bool bPlayedSuccessfully = CombatGameMode->TryPlayCardOnSelf(Card);
		if (bPlayedSuccessfully)
		{
			RemoveCardFromHand(Card);
			ClearSelectedCard();
		}
		else
		{
			RefreshHUD();
		}

		return;
	}

	SelectedCard = Card;
	bCardTargetingMode = true;

	CombatGameMode->RefreshCardTargetHighlights(CombatGameMode->GetPlayerUnit(), SelectedCard);
	RefreshHUD();
}

void AJargonCombatPlayerController::ClearSelectedCard()
{
	SelectedCard = nullptr;
	bCardTargetingMode = false;

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (CombatGameMode)
	{
		CombatGameMode->RefreshPlayerMovementHighlights();
	}

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
	BroadcastCardCounts();
}

void AJargonCombatPlayerController::HandleCombatPhaseChanged(ECombatPhase)
{
	RefreshCombatStateHUD();
}

void AJargonCombatPlayerController::HandleCombatEnergyChanged(int32)
{
	RefreshCombatStateHUD();
}

void AJargonCombatPlayerController::HandlePlayerActionAvailabilityChanged(bool /*bCanMove*/, bool /*bCanAttack*/)
{
	RefreshCombatStateHUD();
}

void AJargonCombatPlayerController::RequestSelectFriendlyUnit(ABattleUnit* Unit)
{
	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode || !Unit)
	{
		return;
	}

	CombatGameMode->TrySelectFriendlyUnit(Unit);
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

void AJargonCombatPlayerController::RequestBasicAttackOnUnit(ABattleUnit* Unit)
{
	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode || !Unit)
	{
		return;
	}

	CombatGameMode->TryBasicAttackWithPlayerUnit(Unit);
}

void AJargonCombatPlayerController::RequestPlayCardOnUnit(ABattleUnit* Unit)
{
	if (!SelectedCard || !Unit)
	{
		return;
	}

	AGridTile* TargetTile = Unit->GetCurrentTile();
	if (!TargetTile)
	{
		return;
	}

	RequestPlayCardOnTile(TargetTile);
}

void AJargonCombatPlayerController::RequestPlayCardOnTile(AGridTile* Tile)
{
	if (!SelectedCard || !Tile)
	{
		return;
	}

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	const bool bPlayedSuccessfully = CombatGameMode->TryPlayCardOnTile(SelectedCard, Tile);
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

	ABattleUnit* HitUnit = Cast<ABattleUnit>(HitActor);
	AGridTile* HitTile = Cast<AGridTile>(HitActor);

	if (SelectedCard)
	{
		if (SelectedCard->TargetType == ECardTargetType::Self)
		{
			return;
		}

		if (SelectedCard->UsesBoardTileTargeting())
		{
			AGridTile* ResolvedTargetTile = HitTile;
			if (!ResolvedTargetTile && HitUnit)
			{
				ResolvedTargetTile = HitUnit->GetCurrentTile();
			}

			if (ResolvedTargetTile)
			{
				RequestPlayCardOnTile(ResolvedTargetTile);
			}
			return;
		}

		return;
	}

	if (HitUnit)
	{
		if (HitUnit->GetTeam() == ETeam::Player)
		{
			RequestSelectFriendlyUnit(HitUnit);
		}
		else
		{
			RequestBasicAttackOnUnit(HitUnit);
		}
		return;
	}

	if (HitTile)
	{
		if (ABattleUnit* OccupyingUnit = HitTile->GetOccupyingUnit())
		{
			if (OccupyingUnit->GetTeam() == ETeam::Player)
			{
				RequestSelectFriendlyUnit(OccupyingUnit);
				return;
			}
		}

		RequestMoveToTile(HitTile);
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

void AJargonCombatPlayerController::RefreshCombatStateHUD()
{
	if (!CombatHUD)
	{
		return;
	}

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (!CombatGameMode)
	{
		return;
	}

	const ECombatPhase CurrentPhase = CombatGameMode->GetCurrentCombatPhase();
	CombatHUD->SetPhaseText(CurrentPhase);
	CombatHUD->SetEnergyValues(CombatGameMode->GetCurrentEnergy(), CombatGameMode->GetCurrentMaxEnergy());
	CombatHUD->SetActionAvailability(
		CurrentPhase,
		CombatGameMode->HasPlayerMoveRemaining(),
		CombatGameMode->HasPlayerAttackRemaining()
	);
}

void AJargonCombatPlayerController::BroadcastCardCounts()
{
	OnDeckChanged.Broadcast(DrawPile.Num());
	OnHandChanged.Broadcast(Hand.Num());
}
