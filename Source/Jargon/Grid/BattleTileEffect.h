// BattleTileEffect.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "GameFramework/Actor.h"
#include "BattleTileEffect.generated.h"

class USceneComponent;
class AGridTile;
class UCardDefinition;
class AJargonCombatGameMode;
class ABattleUnit;

UCLASS(Blueprintable)
class JARGON_API ABattleTileEffect : public AActor
{
	GENERATED_BODY()

public:
	ABattleTileEffect();

	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	void InitializeFromCard(
		UCardDefinition* InSourceCard,
		ETeam InSourceTeam,
		ECardCategory InCardCategory,
		int32 InEffectValue,
		int32 InEffectRadius);

	UFUNCTION(BlueprintCallable, Category = "Tile Effect")
	void PlaceOnTile(AGridTile* Tile);

	virtual void HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode);
	virtual void HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit);

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	AGridTile* GetCurrentTile() const
	{
		return CurrentTile;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	UCardDefinition* GetSourceCard() const
	{
		return SourceCard;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	ETeam GetSourceTeam() const
	{
		return SourceTeam;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	ECardCategory GetCardCategory() const
	{
		return CardCategory;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	int32 GetEffectRadius() const
	{
		return EffectRadius;
	}

	UFUNCTION(BlueprintPure, Category = "Tile Effect")
	int32 GetEffectValue() const
	{
		return EffectValue;
	}

protected:
	TArray<AGridTile*> GetTilesInEffectRadius(const AJargonCombatGameMode* CombatGameMode) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Tile Effect")
	TObjectPtr<AGridTile> CurrentTile = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Tile Effect")
	TObjectPtr<UCardDefinition> SourceCard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Tile Effect")
	ETeam SourceTeam = ETeam::Player;

	UPROPERTY(VisibleInstanceOnly, Category = "Tile Effect")
	ECardCategory CardCategory = ECardCategory::Spell;

	UPROPERTY(VisibleInstanceOnly, Category = "Tile Effect|Area")
	int32 EffectRadius = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Tile Effect")
	int32 EffectValue = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Tile Effect", meta = (ClampMin = "0.0"))
	float TileEffectZOffset = 15.f;
};
