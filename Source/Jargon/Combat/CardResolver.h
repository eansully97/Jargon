// CardResolver.h

#pragma once

#include "CoreMinimal.h"
#include "Data/CardDefinition.h"
#include "CardResolver.generated.h"

class AJargonCombatGameMode;
class ABattleUnit;
class AGridTile;
struct FJargonEffectTrace;

USTRUCT(BlueprintType)
struct JARGON_API FCardResolveContext
{
	GENERATED_BODY()

	/** Combat GameMode that owns card costs, hands, element charges, units, and spawned effects for this resolve. */
	UPROPERTY()
	TObjectPtr<AJargonCombatGameMode> GameMode = nullptr;

	/** Unit playing the card; normally the selected player-controlled battle unit. */
	UPROPERTY()
	TObjectPtr<ABattleUnit> SourceUnit = nullptr;

	/** Explicit unit chosen by the player for unit-targeted cards. */
	UPROPERTY()
	TObjectPtr<ABattleUnit> UnitTarget = nullptr;

	/** Explicit tile chosen by the player, or the tile derived from the selected unit target. */
	UPROPERTY()
	TObjectPtr<AGridTile> TileTarget = nullptr;

	/** Manual elemental bonus choices confirmed by the player before card resolution. */
	UPROPERTY()
	TArray<int32> SelectedElementalBonusIndices;
};

USTRUCT(BlueprintType)
struct JARGON_API FCardResolveResult
{
	GENERATED_BODY()

	/** Whether the caller should spend normal Energy after this resolve succeeds. */
	UPROPERTY()
	bool bConsumeEnergy = true;

	/** Whether the played card should leave the hand after this resolve succeeds. */
	UPROPERTY()
	bool bConsumeCard = true;

	/** Whether the card consumed the player's move action, such as MoveSource cards. */
	UPROPERTY()
	bool bConsumePlayerMove = false;

	/** True when presentation or movement continues after card logic has started resolving. */
	UPROPERTY()
	bool bContinuesAsynchronously = false;

	/** Energy gained after the cost is paid; CardResolver reports it here so callers keep cost ordering consistent. */
	UPROPERTY()
	int32 EnergyGainAfterCost = 0;
};

/** Adapter that converts card-authored scripts into the shared Jargon effect executor/runtime. */
class JARGON_API FCardResolver
{
public:
	/** Resolves a card from C++ gameplay code without collecting an effect trace. */
	static bool ResolveCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	/** Resolves a card and optionally fills a trace for debugging card/effect authoring. */
	static bool ResolveCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult,
		FJargonEffectTrace* OutTrace
	);

private:
	/** Builds base and selected elemental bonus specs before routing them through FJargonEffectExecutor. */
	static bool ResolveEffectSpecCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveEffectSpecCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult,
		FJargonEffectTrace* OutTrace
	);

	/** Normalizes the card target into the shared effect context's primary tile role. */
	static AGridTile* GetResolvedTargetTile(const FCardResolveContext& Context);

	/** Normalizes the card target into the shared effect context's primary unit role. */
	static ABattleUnit* GetResolvedTargetUnit(const FCardResolveContext& Context);
};
