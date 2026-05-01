#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"

class ABattleUnit;
class ABattleTileEffect;
class AGridTile;

class JARGON_API FJargonEffectResolver
{
public:
	static bool ResolveEffects(
		const TArray<FJargonEffectSpec>& Effects,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolveEffects(
		const TArray<FJargonEffectSpec>& Effects,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace);

	static bool ResolveEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolveEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace,
		int32 EffectIndex = INDEX_NONE);

private:
	static bool ResolveMoveSourceEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolvePushTargetEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace = nullptr,
		int32 EffectIndex = INDEX_NONE);

	static bool ResolveSummonUnitEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace = nullptr,
		int32 EffectIndex = INDEX_NONE);

	static bool ResolvePlaceTileEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolveDestroyTileEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace = nullptr,
		int32 EffectIndex = INDEX_NONE);

	static bool ResolveDrawCardsEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolveGainEnergyEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolveGainElementChargeEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	static bool ResolveUnitPayloadEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace = nullptr,
		int32 EffectIndex = INDEX_NONE);

	static TArray<ABattleUnit*> GatherTargetUnits(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectTrace* OutTrace = nullptr,
		int32 EffectIndex = INDEX_NONE);

	static TArray<AGridTile*> GatherTargetTiles(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);

	static TArray<ABattleUnit*> GatherChainTargetUnits(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);

	static bool DoesUnitPassTargetFilter(
		const ABattleUnit* Unit,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);

	static bool DoesTileEffectPassTargetFilter(
		const ABattleTileEffect* TileEffect,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);

	static bool ApplyUnitPayload(
		ABattleUnit* TargetUnit,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);

	static bool CanTreatNoTargetsAsNoOp(const FJargonEffectContext& Context);

	static bool ValidateEffectForContext(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);

	static AGridTile* GetResolvedTargetTile(const FJargonEffectContext& Context);
	static ABattleUnit* GetResolvedTargetUnit(const FJargonEffectContext& Context);
	static AGridTile* GetResolvedSourceTile(const FJargonEffectContext& Context);
	static ETeam GetResolvedSourceTeam(const FJargonEffectContext& Context);
};
