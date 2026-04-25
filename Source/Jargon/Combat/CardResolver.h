// CardResolver.h

#pragma once

#include "CoreMinimal.h"
#include "CardResolver.generated.h"

class AJargonCombatGameMode;
class ABattleUnit;
class AGridTile;
class UCardDefinition;

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
	static bool ResolveSpellCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolvePersistentTileCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveSummonCard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveDamage(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveHeal(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveAOEDamage(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolvePush(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveMoveSelf(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static bool ResolveGuard(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		FCardResolveResult& OutResult
	);

	static AGridTile* GetResolvedTargetTile(const FCardResolveContext& Context);
	static ABattleUnit* GetResolvedTargetUnit(const FCardResolveContext& Context);
};
