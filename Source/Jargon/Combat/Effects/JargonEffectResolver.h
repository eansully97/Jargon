#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"

class ABattleUnit;
class ABattleTileEffect;
class AGridTile;

/**
 * Low-level shared effect runtime.
 *
 * Normal gameplay callers should enter through FJargonEffectExecutor so logging,
 * ability context validation, traces, and no-op handling stay consistent.
 */
class JARGON_API FJargonEffectResolver
{
public:
	/** Resolves an ordered list of specs against a complete runtime context. Prefer FJargonEffectExecutor. */
	static bool ResolveEffects(
		const TArray<FJargonEffectSpec>& Effects,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	/** Resolves an ordered list of specs while collecting validation, targeting, and operation trace events. */
	static bool ResolveEffects(
		const TArray<FJargonEffectSpec>& Effects,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult,
		FJargonEffectTrace* OutTrace);

	/** Resolves one effect spec without trace capture. Prefer the batch executor for authored gameplay. */
	static bool ResolveEffect(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		FJargonEffectResult& OutResult);

	/** Resolves one effect spec with trace capture for editor/debug diagnostics. */
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

	static bool ResolvePullTargetEffect(
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

	static AGridTile* ResolvePlacementTile(
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context);
	static AGridTile* ResolvePlacementAnchorTile(
		EJargonAbilityPlacementAnchor PlacementAnchor,
		const FJargonEffectContext& Context);
	static AGridTile* FindNearestEmptyWalkableTile(
		const FJargonEffectContext& Context,
		AGridTile* AnchorTile);

	static AGridTile* GetResolvedTargetTile(const FJargonEffectContext& Context);
	static ABattleUnit* GetResolvedTargetUnit(const FJargonEffectContext& Context);
	static AGridTile* GetResolvedSourceTile(const FJargonEffectContext& Context);
	static ETeam GetResolvedSourceTeam(const FJargonEffectContext& Context);
};
