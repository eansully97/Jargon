// CardResolver.h

#pragma once

#include "CoreMinimal.h"
#include "Data/CardDefinition.h"
#include "CardResolver.generated.h"

class AJargonCombatGameMode;
class ABattleUnit;
class AGridTile;

USTRUCT(BlueprintType)
struct JARGON_API FCardResolveContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AJargonCombatGameMode> GameMode = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleUnit> SourceUnit = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleUnit> UnitTarget = nullptr;

	UPROPERTY()
	TObjectPtr<AGridTile> TileTarget = nullptr;
};

USTRUCT(BlueprintType)
struct JARGON_API FCardResolveResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bConsumeEnergy = true;

	UPROPERTY()
	bool bConsumeCard = true;

	UPROPERTY()
	bool bConsumePlayerMove = false;

	UPROPERTY()
	bool bContinuesAsynchronously = false;

	UPROPERTY()
	int32 EnergyGainAfterCost = 0;
};

class JARGON_API FCardResolver
{
public:
	static bool ResolveCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

private:
	static bool ResolveEffectSpecCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveEffectSpec(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveDealDamageEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveHealEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveApplyShieldEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveMoveSelfEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolvePushTargetEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveSummonUnitEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolvePlaceTileEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveDrawCardsEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveGainEnergyEffect(
		const UCardDefinition* Card,
		const FCardEffectSpec& EffectSpec,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static AGridTile* GetResolvedTargetTile(const FCardResolveContext& Context);
	static ABattleUnit* GetResolvedTargetUnit(const FCardResolveContext& Context);
};
