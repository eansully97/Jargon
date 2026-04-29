// CardResolver.cpp

#include "Combat/CardResolver.h"

#include "Combat/Effects/JargonEffectResolver.h"
#include "Combat/JargonCombatGameMode.h"
#include "Data/CardDefinition.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"
#include "Units/BattleUnit.h"

namespace
{
	void AppendValidUnitsFromArray(
		const TArray<TObjectPtr<ABattleUnit>>& SourceArray,
		TArray<ABattleUnit*>& OutUnits)
	{
		for (const TObjectPtr<ABattleUnit>& UnitPtr : SourceArray)
		{
			ABattleUnit* Unit = UnitPtr.Get();
			if (IsValid(Unit) && !Unit->IsDead())
			{
				OutUnits.Add(Unit);
			}
		}
	}

	TArray<ABattleUnit*> GetAllLivingCombatUnits(const AJargonCombatGameMode* GameMode)
	{
		TArray<ABattleUnit*> Units;

		if (!GameMode)
		{
			return Units;
		}

		AppendValidUnitsFromArray(GameMode->GetFriendlyUnits(), Units);
		AppendValidUnitsFromArray(GameMode->GetEnemyUnits(), Units);

		return Units;
	}

	FJargonEffectSpec ConvertCardEffectSpecToJargonEffectSpec(
		const UCardDefinition* Card,
		const FCardEffectSpec& CardEffectSpec)
	{
		FJargonEffectSpec JargonEffectSpec;
		JargonEffectSpec.Value = Card ? Card->GetConfiguredValueForEffect(CardEffectSpec) : FMath::Max(0, CardEffectSpec.Value);
		JargonEffectSpec.Radius = Card ? Card->GetConfiguredRadiusForEffect(CardEffectSpec) : FMath::Max(0, CardEffectSpec.EffectRadius);
		JargonEffectSpec.ChainCount = FMath::Max(1, CardEffectSpec.ChainCount);
		JargonEffectSpec.MoveDistance = Card ? Card->GetConfiguredRangeForEffect(CardEffectSpec) : FMath::Max(0, CardEffectSpec.MoveDistance);
		JargonEffectSpec.PushDistance = Card ? Card->GetConfiguredPushDistanceForEffect(CardEffectSpec) : FMath::Max(0, CardEffectSpec.PushDistance);
		JargonEffectSpec.PullDistance = FMath::Max(0, CardEffectSpec.PullDistance);
		JargonEffectSpec.CollisionDamage = Card ? Card->GetConfiguredCollisionDamageForEffect(CardEffectSpec) : FMath::Max(0, CardEffectSpec.CollisionDamage);
		JargonEffectSpec.UnitClass = CardEffectSpec.UnitClass;
		JargonEffectSpec.bSummonEntersWithAttackExhausted = CardEffectSpec.bSummonEntersWithAttackExhausted;
		JargonEffectSpec.TileEffectClass = CardEffectSpec.TileEffectClass;
		JargonEffectSpec.TileEffectDuration = FMath::Max(0, CardEffectSpec.TileEffectDuration);
		JargonEffectSpec.TileEffectCategory = Card ? Card->Category : ECardCategory::Trap;

		switch (CardEffectSpec.Operation)
		{
		case ECardEffectOperation::DealDamage:
			JargonEffectSpec.Operation = EJargonEffectOperation::DealDamage;
			JargonEffectSpec.Delivery = JargonEffectSpec.Radius > 0
				? EJargonEffectDelivery::UnitsInRadius
				: EJargonEffectDelivery::ExplicitUnit;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::Heal:
			JargonEffectSpec.Operation = EJargonEffectOperation::Heal;
			JargonEffectSpec.Delivery = JargonEffectSpec.Radius > 0
				? EJargonEffectDelivery::UnitsInRadius
				: EJargonEffectDelivery::ExplicitUnit;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::FriendlyToSource;
			break;

		case ECardEffectOperation::ApplyShield:
			JargonEffectSpec.Operation = EJargonEffectOperation::ApplyShield;
			JargonEffectSpec.Delivery = JargonEffectSpec.Radius > 0
				? EJargonEffectDelivery::UnitsInRadius
				: EJargonEffectDelivery::ExplicitUnit;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::FriendlyToSource;
			break;

		case ECardEffectOperation::ApplyStun:
			JargonEffectSpec.Operation = EJargonEffectOperation::ApplyStun;
			JargonEffectSpec.Delivery = JargonEffectSpec.Radius > 0
				? EJargonEffectDelivery::UnitsInRadius
				: EJargonEffectDelivery::ExplicitUnit;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::MoveSelf:
			JargonEffectSpec.Operation = EJargonEffectOperation::MoveSource;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ExplicitTile;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
			break;

		case ECardEffectOperation::PushTarget:
			JargonEffectSpec.Operation = EJargonEffectOperation::PushTarget;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ExplicitUnit;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::PullTarget:
			JargonEffectSpec.Operation = EJargonEffectOperation::PullTarget;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ExplicitUnit;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::SummonUnit:
			JargonEffectSpec.Operation = EJargonEffectOperation::SummonUnit;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ExplicitTile;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::None;
			break;

		case ECardEffectOperation::PlaceTileEffect:
			JargonEffectSpec.Operation = EJargonEffectOperation::PlaceTileEffect;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ExplicitTile;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::None;
			break;

		case ECardEffectOperation::DestroyTileEffect:
			JargonEffectSpec.Operation = EJargonEffectOperation::DestroyTileEffect;
			JargonEffectSpec.Delivery = JargonEffectSpec.Radius > 0
				? EJargonEffectDelivery::TilesInRadius
				: EJargonEffectDelivery::ExplicitTile;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::DrawCards:
			JargonEffectSpec.Operation = EJargonEffectOperation::DrawCards;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::Self;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
			break;

		case ECardEffectOperation::GainEnergy:
			JargonEffectSpec.Operation = EJargonEffectOperation::GainEnergy;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::Self;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::SourceOnly;
			break;

		case ECardEffectOperation::ChainDamage:
			JargonEffectSpec.Operation = EJargonEffectOperation::DealDamage;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ChainUnits;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::ChainHeal:
			JargonEffectSpec.Operation = EJargonEffectOperation::Heal;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ChainUnits;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::FriendlyToSource;
			break;

		case ECardEffectOperation::ChainStun:
			JargonEffectSpec.Operation = EJargonEffectOperation::ApplyStun;
			JargonEffectSpec.Delivery = EJargonEffectDelivery::ChainUnits;
			JargonEffectSpec.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
			break;

		case ECardEffectOperation::None:
		default:
			JargonEffectSpec.Operation = EJargonEffectOperation::None;
			break;
		}

		return JargonEffectSpec;
	}

	TArray<FJargonEffectSpec> ConvertCardEffectsToJargonEffectSpecs(const UCardDefinition* Card)
	{
		TArray<FJargonEffectSpec> JargonEffectSpecs;
		if (!Card)
		{
			return JargonEffectSpecs;
		}

		JargonEffectSpecs.Reserve(Card->Effects.Num());
		for (const FCardEffectSpec& CardEffectSpec : Card->Effects)
		{
			JargonEffectSpecs.Add(ConvertCardEffectSpecToJargonEffectSpec(Card, CardEffectSpec));
		}

		return JargonEffectSpecs;
	}

	FJargonEffectContext BuildJargonEffectContextFromCardContext(
		const UCardDefinition* Card,
		const FCardResolveContext& Context)
	{
		FJargonEffectContext JargonContext;
		JargonContext.GameMode = Context.GameMode;
		JargonContext.SourceObject = const_cast<UCardDefinition*>(Card);
		JargonContext.SourceUnit = Context.SourceUnit;
		JargonContext.SourceTeam = Context.SourceUnit ? Context.SourceUnit->GetTeam() : ETeam::Player;
		JargonContext.SourceTile = Context.SourceUnit ? Context.SourceUnit->GetCurrentTile() : nullptr;
		JargonContext.PrimaryUnitTarget = Context.UnitTarget;
		JargonContext.PrimaryTileTarget = Context.TileTarget;
		JargonContext.SourceCard = const_cast<UCardDefinition*>(Card);
		return JargonContext;
	}
}

bool FCardResolver::ResolveCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	if (!Card->UsesEffectSpecs())
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has no Effects authored. Cards now resolve through Effects[] only."),
			*Card->DisplayName.ToString());
		return false;
	}

	return ResolveEffectSpecCard(Card, Context, OutResult);
}

bool FCardResolver::ResolveEffectSpecCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Card->UsesEffectSpecs())
	{
		return false;
	}

	const TArray<FJargonEffectSpec> JargonEffectSpecs = ConvertCardEffectsToJargonEffectSpecs(Card);
	const FJargonEffectContext JargonContext = BuildJargonEffectContextFromCardContext(Card, Context);

	FJargonEffectResult JargonResult;
	const bool bResolved = FJargonEffectResolver::ResolveEffects(JargonEffectSpecs, JargonContext, JargonResult);

	OutResult.bConsumePlayerMove |= JargonResult.bConsumePlayerMove;
	OutResult.bContinuesAsynchronously |= JargonResult.bContinuesAsynchronously;
	OutResult.EnergyGainAfterCost += JargonResult.EnergyGainAfterCost;

	return bResolved;
}

bool FCardResolver::ResolveEffectSpec(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card)
	{
		return false;
	}

	switch (EffectSpec.Operation)
	{
	case ECardEffectOperation::DealDamage:
		return ResolveDealDamageEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::Heal:
		return ResolveHealEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::ApplyShield:
		return ResolveApplyShieldEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::MoveSelf:
		return ResolveMoveSelfEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::PushTarget:
		return ResolvePushTargetEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::PullTarget:
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' uses PullTarget, but PullTarget is intentionally deferred in this pass."),
			*Card->DisplayName.ToString());
		return false;

	case ECardEffectOperation::SummonUnit:
		return ResolveSummonUnitEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::PlaceTileEffect:
		return ResolvePlaceTileEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::DestroyTileEffect:
		return ResolveDestroyTileEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::DrawCards:
		return ResolveDrawCardsEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::GainEnergy:
		return ResolveGainEnergyEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::ApplyStun:
		return ResolveApplyStunEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::ChainDamage:
		return ResolveChainEffect(Card, EffectSpec, Context, OutResult);
		
	case ECardEffectOperation::ChainHeal:
		return ResolveChainEffect(Card, EffectSpec, Context, OutResult);
		
	case ECardEffectOperation::ChainStun:
		return ResolveChainEffect(Card, EffectSpec, Context, OutResult);

	case ECardEffectOperation::None:
	default:
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has an unsupported or empty effect operation."),
			*Card->DisplayName.ToString());
		return false;
	}
	
}

bool FCardResolver::ResolveDealDamageEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	const int32 DamageAmount = Card->GetConfiguredValueForEffect(EffectSpec);
	if (DamageAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a DealDamage effect with no positive damage value."),
			*Card->DisplayName.ToString());
		return false;
	}

	const int32 EffectRadius = Card->GetConfiguredRadiusForEffect(EffectSpec);
	if (EffectRadius > 0)
	{
		AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
		AGridTile* CenterTile = GetResolvedTargetTile(Context);
		if (!GridBoard || !CenterTile)
		{
			return false;
		}

		TArray<TWeakObjectPtr<ABattleUnit>> TargetsToDamage;
		const TArray<TObjectPtr<ABattleUnit>>& EnemyUnits = Context.GameMode->GetEnemyUnits();
		TargetsToDamage.Reserve(EnemyUnits.Num());

		for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
		{
			if (!IsValid(EnemyUnit) || EnemyUnit->IsDead() || EnemyUnit->GetTeam() == Context.SourceUnit->GetTeam())
			{
				continue;
			}

			if (EnemyUnit->GetCurrentTile() &&
				GridBoard->AreTilesWithinRange(EnemyUnit->GetCurrentTile(), CenterTile, EffectRadius))
			{
				TargetsToDamage.Add(EnemyUnit);
			}
		}

		for (const TWeakObjectPtr<ABattleUnit>& TargetPtr : TargetsToDamage)
		{
			ABattleUnit* TargetUnit = TargetPtr.Get();
			if (!IsValid(TargetUnit) || TargetUnit->IsDead())
			{
				continue;
			}

			TargetUnit->ApplyDamage(DamageAmount);

			if (!IsValid(Context.GameMode) ||
				Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Victory ||
				Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Defeat)
			{
				break;
			}
		}

		return true;
	}

	ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context);
	if (!TargetUnit || TargetUnit->IsDead())
	{
		return false;
	}

	if (TargetUnit->GetTeam() == Context.SourceUnit->GetTeam())
	{
		return false;
	}

	TargetUnit->ApplyDamage(DamageAmount);
	return true;
}

bool FCardResolver::ResolveHealEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	const int32 HealAmount = Card->GetConfiguredValueForEffect(EffectSpec);
	if (HealAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a Heal effect with no positive heal value."),
			*Card->DisplayName.ToString());
		return false;
	}

	const int32 EffectRadius = Card->GetConfiguredRadiusForEffect(EffectSpec);
	if (EffectRadius > 0)
	{
		AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
		AGridTile* CenterTile = GetResolvedTargetTile(Context);
		if (!CenterTile)
		{
			CenterTile = Context.SourceUnit->GetCurrentTile();
		}

		if (!GridBoard || !CenterTile)
		{
			return false;
		}

		const TArray<TObjectPtr<ABattleUnit>>& FriendlyUnits = Context.GameMode->GetFriendlyUnits();
		for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
		{
			if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead() || FriendlyUnit->GetTeam() != Context.SourceUnit->GetTeam())
			{
				continue;
			}

			if (FriendlyUnit->GetCurrentTile() &&
				GridBoard->AreTilesWithinRange(FriendlyUnit->GetCurrentTile(), CenterTile, EffectRadius))
			{
				FriendlyUnit->ApplyHeal(HealAmount);
			}
		}

		return true;
	}

	ABattleUnit* ActualTarget = GetResolvedTargetUnit(Context);
	if (!ActualTarget)
	{
		ActualTarget = Context.SourceUnit.Get();
	}

	if (!ActualTarget || ActualTarget->IsDead() || ActualTarget->GetTeam() != Context.SourceUnit->GetTeam())
	{
		return false;
	}

	ActualTarget->ApplyHeal(HealAmount);
	return true;
}

bool FCardResolver::ResolveApplyShieldEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	const int32 ShieldAmount = Card->GetConfiguredValueForEffect(EffectSpec);
	if (ShieldAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has an ApplyShield effect with no positive shield value."),
			*Card->DisplayName.ToString());
		return false;
	}

	const int32 EffectRadius = Card->GetConfiguredRadiusForEffect(EffectSpec);
	if (EffectRadius > 0)
	{
		AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
		AGridTile* CenterTile = GetResolvedTargetTile(Context);
		if (!CenterTile)
		{
			CenterTile = Context.SourceUnit->GetCurrentTile();
		}

		if (!GridBoard || !CenterTile)
		{
			return false;
		}

		const TArray<TObjectPtr<ABattleUnit>>& FriendlyUnits = Context.GameMode->GetFriendlyUnits();
		for (const TObjectPtr<ABattleUnit>& FriendlyUnit : FriendlyUnits)
		{
			if (!IsValid(FriendlyUnit) || FriendlyUnit->IsDead() || FriendlyUnit->GetTeam() != Context.SourceUnit->GetTeam())
			{
				continue;
			}

			if (FriendlyUnit->GetCurrentTile() &&
				GridBoard->AreTilesWithinRange(FriendlyUnit->GetCurrentTile(), CenterTile, EffectRadius))
			{
				FriendlyUnit->AddTemporaryShield(ShieldAmount);
			}
		}

		return true;
	}

	ABattleUnit* ActualTarget = GetResolvedTargetUnit(Context);
	if (!ActualTarget)
	{
		ActualTarget = Context.SourceUnit.Get();
	}

	if (!ActualTarget || ActualTarget->IsDead() || ActualTarget->GetTeam() != Context.SourceUnit->GetTeam())
	{
		return false;
	}

	ActualTarget->AddTemporaryShield(ShieldAmount);
	return true;
}

bool FCardResolver::ResolveApplyStunEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	const int32 StunTurns = Card->GetConfiguredValueForEffect(EffectSpec);
	if (StunTurns <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has an ApplyStun effect with no positive stun duration."),
			*Card->DisplayName.ToString());
		return false;
	}

	const int32 EffectRadius = Card->GetConfiguredRadiusForEffect(EffectSpec);

	if (EffectRadius > 0)
	{
		AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
		AGridTile* CenterTile = GetResolvedTargetTile(Context);

		if (!GridBoard || !CenterTile)
		{
			return false;
		}

		bool bAppliedAnyStun = false;
		const TArray<ABattleUnit*> AllUnits = GetAllLivingCombatUnits(Context.GameMode);

		for (ABattleUnit* CandidateUnit : AllUnits)
		{
			if (!IsValid(CandidateUnit) || CandidateUnit->IsDead())
			{
				continue;
			}

			// Stun is treated as a hostile effect for now.
			if (CandidateUnit->GetTeam() == Context.SourceUnit->GetTeam())
			{
				continue;
			}

			if (CandidateUnit->GetCurrentTile() &&
				GridBoard->AreTilesWithinRange(CandidateUnit->GetCurrentTile(), CenterTile, EffectRadius))
			{
				CandidateUnit->ApplyStun(StunTurns);
				bAppliedAnyStun = true;
			}
		}

		return bAppliedAnyStun;
	}

	ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context);
	if (!TargetUnit || TargetUnit->IsDead())
	{
		return false;
	}

	// Stun is treated as a hostile effect for now.
	if (TargetUnit->GetTeam() == Context.SourceUnit->GetTeam())
	{
		return false;
	}

	TargetUnit->ApplyStun(StunTurns);
	return true;
}

bool FCardResolver::ResolveMoveSelfEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit || !Context.TileTarget)
	{
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* StartTile = Context.SourceUnit->GetCurrentTile();
	AGridTile* DestinationTile = Context.TileTarget.Get();

	if (!GridBoard || !StartTile || !DestinationTile || DestinationTile == StartTile || !DestinationTile->IsWalkable())
	{
		return false;
	}

	const TArray<AGridTile*> Path = GridBoard->BuildPath(StartTile, DestinationTile);
	if (Path.Num() < 2)
	{
		return false;
	}

	const int32 StepsRequired = Path.Num() - 1;
	const int32 MaxSteps = Card->GetConfiguredRangeForEffect(EffectSpec);
	if (StepsRequired > MaxSteps)
	{
		return false;
	}

	if (!Context.GameMode->StartPlayerControlledMoveSequence(Context.SourceUnit, Path, false))
	{
		return false;
	}

	OutResult.bContinuesAsynchronously = true;
	return true;
}

bool FCardResolver::ResolvePushTargetEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context);
	if (!TargetUnit || TargetUnit->IsDead() || TargetUnit->GetTeam() == Context.SourceUnit->GetTeam())
	{
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* SourceTile = Context.SourceUnit->GetCurrentTile();
	AGridTile* TargetTile = TargetUnit->GetCurrentTile();

	if (!GridBoard || !SourceTile || !TargetTile)
	{
		return false;
	}

	const int32 PushDistance = FMath::Max(1, Card->GetConfiguredPushDistanceForEffect(EffectSpec));
	const int32 CollisionDamage = Card->GetConfiguredCollisionDamageForEffect(EffectSpec);

	AGridTile* BestDestination = nullptr;
	bool bCollidedWithObstruction = false;

	for (int32 Step = 1; Step <= PushDistance; ++Step)
	{
		AGridTile* CandidateTile = GridBoard->GetTileInPushDirection(SourceTile, TargetTile, Step);
		if (!CandidateTile || !CandidateTile->IsWalkable())
		{
			bCollidedWithObstruction = true;
			break;
		}

		BestDestination = CandidateTile;
	}

	bool bResolvedAnyPushEffect = false;

	if (BestDestination)
	{
		TargetUnit->PlaceOnTile(BestDestination);
		bResolvedAnyPushEffect = true;
	}

	if (bCollidedWithObstruction && CollisionDamage > 0 && !TargetUnit->IsDead())
	{
		TargetUnit->ApplyDamage(CollisionDamage);
		bResolvedAnyPushEffect = true;
	}

	return bResolvedAnyPushEffect;
}

bool FCardResolver::ResolveSummonUnitEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	if (!TargetTile)
	{
		return false;
	}

	if (!EffectSpec.UnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a SummonUnit effect with no UnitClass."),
			*Card->DisplayName.ToString());
		return false;
	}

	return Context.GameMode->SpawnSummonedUnitFromClass(
		EffectSpec.UnitClass,
		Context.SourceUnit,
		TargetTile,
		EffectSpec.bSummonEntersWithAttackExhausted) != nullptr;
}

bool FCardResolver::ResolvePlaceTileEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	if (!TargetTile)
	{
		return false;
	}

	if (!EffectSpec.TileEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a PlaceTileEffect effect with no TileEffectClass."),
			*Card->DisplayName.ToString());
		return false;
	}

	if (Card->Category != ECardCategory::Trap && Card->Category != ECardCategory::Aura)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' places a tile effect but its Category is not Trap or Aura. The effect will use the card category as authored."),
			*Card->DisplayName.ToString());
	}

	return Context.GameMode->SpawnPersistentTileEffectFromClass(
	EffectSpec.TileEffectClass,
	Card,
	Context.SourceUnit,
	TargetTile,
	Card->Category,
	Card->GetConfiguredValueForEffect(EffectSpec),
	Card->GetConfiguredRadiusForEffect(EffectSpec),
	EffectSpec.TileEffectDuration) != nullptr;
}

bool FCardResolver::ResolveDestroyTileEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* CenterTile = GetResolvedTargetTile(Context);
	if (!GridBoard || !CenterTile)
	{
		return false;
	}

	const int32 EffectRadius = Card->GetConfiguredRadiusForEffect(EffectSpec);

	TArray<AGridTile*> TargetTiles;
	if (EffectRadius > 0)
	{
		TargetTiles = GridBoard->GetTilesWithinRadius(CenterTile, EffectRadius);
	}
	else
	{
		TargetTiles.Add(CenterTile);
	}

	bool bDestroyedAny = false;

	for (AGridTile* Tile : TargetTiles)
	{
		if (!Tile)
		{
			continue;
		}

		TArray<TObjectPtr<ABattleTileEffect>> TileEffects = Tile->GetTileEffects();

		for (const TObjectPtr<ABattleTileEffect>& TileEffectPtr : TileEffects)
		{
			ABattleTileEffect* TileEffect = TileEffectPtr.Get();
			if (!IsValid(TileEffect))
			{
				continue;
			}

			// First version: destroy opposing tile effects only.
			if (TileEffect->GetSourceTeam() == Context.SourceUnit->GetTeam())
			{
				continue;
			}

			TileEffect->Destroy();
			bDestroyedAny = true;
		}
	}

	return bDestroyedAny;
}

bool FCardResolver::ResolveDrawCardsEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode)
	{
		return false;
	}

	const int32 CardCount = Card->GetConfiguredValueForEffect(EffectSpec);
	if (CardCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a DrawCards effect with no positive card count."),
			*Card->DisplayName.ToString());
		return false;
	}

	return Context.GameMode->DrawCardsForPlayer(CardCount);
}

bool FCardResolver::ResolveGainEnergyEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode)
	{
		return false;
	}

	const int32 EnergyAmount = Card->GetConfiguredValueForEffect(EffectSpec);
	if (EnergyAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a GainEnergy effect with no positive energy amount."),
			*Card->DisplayName.ToString());
		return false;
	}

	OutResult.EnergyGainAfterCost += EnergyAmount;
	return true;
}

bool FCardResolver::ResolveChainEffect(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	if (!GridBoard)
	{
		return false;
	}

	ABattleUnit* CurrentTarget = GetResolvedTargetUnit(Context);
	if (!CurrentTarget && EffectSpec.Operation == ECardEffectOperation::ChainHeal)
	{
		CurrentTarget = Context.SourceUnit.Get();
	}

	if (!CurrentTarget || CurrentTarget->IsDead())
	{
		return false;
	}

	const int32 EffectValue = Card->GetConfiguredValueForEffect(EffectSpec);
	const int32 ChainCount = FMath::Max(1, EffectSpec.ChainCount);
	const int32 ChainSearchRadius = FMath::Max(1, Card->GetConfiguredRadiusForEffect(EffectSpec));

	if (EffectValue <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has a chain effect with no positive value."),
			*Card->DisplayName.ToString());
		return false;
	}

	auto IsValidChainTarget = [&](ABattleUnit* CandidateUnit) -> bool
	{
		if (!IsValid(CandidateUnit) || CandidateUnit->IsDead())
		{
			return false;
		}

		switch (EffectSpec.Operation)
		{
		case ECardEffectOperation::ChainDamage:
		case ECardEffectOperation::ChainStun:
			return CandidateUnit->GetTeam() != Context.SourceUnit->GetTeam();

		case ECardEffectOperation::ChainHeal:
			return CandidateUnit->GetTeam() == Context.SourceUnit->GetTeam();

		default:
			return false;
		}
	};

	auto ApplyChainPayload = [&](ABattleUnit* TargetUnit)
	{
		if (!IsValid(TargetUnit) || TargetUnit->IsDead())
		{
			return;
		}

		switch (EffectSpec.Operation)
		{
		case ECardEffectOperation::ChainDamage:
			TargetUnit->ApplyDamage(EffectValue);
			break;

		case ECardEffectOperation::ChainHeal:
			TargetUnit->ApplyHeal(EffectValue);
			break;

		case ECardEffectOperation::ChainStun:
			TargetUnit->ApplyStun(EffectValue);
			break;

		default:
			break;
		}
	};

	auto AppendChainCandidates = [&](
		const TArray<TObjectPtr<ABattleUnit>>& Units,
		AGridTile* OriginTile,
		const TArray<ABattleUnit*>& AlreadyHitUnits,
		TArray<ABattleUnit*>& OutCandidates)
	{
		if (!OriginTile)
		{
			return;
		}

		for (const TObjectPtr<ABattleUnit>& UnitPtr : Units)
		{
			ABattleUnit* CandidateUnit = UnitPtr.Get();
			if (!IsValidChainTarget(CandidateUnit))
			{
				continue;
			}

			if (AlreadyHitUnits.Contains(CandidateUnit))
			{
				continue;
			}

			AGridTile* CandidateTile = CandidateUnit->GetCurrentTile();
			if (!CandidateTile)
			{
				continue;
			}

			if (GridBoard->AreTilesWithinRange(OriginTile, CandidateTile, ChainSearchRadius))
			{
				OutCandidates.Add(CandidateUnit);
			}
		}
	};

	if (!IsValidChainTarget(CurrentTarget))
	{
		return false;
	}

	TArray<ABattleUnit*> HitUnits;
	HitUnits.Reserve(ChainCount);

	bool bResolvedAnyEffect = false;

	for (int32 ChainIndex = 0; ChainIndex < ChainCount; ++ChainIndex)
	{
		if (!IsValidChainTarget(CurrentTarget))
		{
			break;
		}

		// Capture this before applying damage, because death may clear/alter occupancy.
		AGridTile* ChainOriginTile = CurrentTarget->GetCurrentTile();
		if (!ChainOriginTile)
		{
			break;
		}

		ApplyChainPayload(CurrentTarget);
		HitUnits.Add(CurrentTarget);
		bResolvedAnyEffect = true;

		if (!IsValid(Context.GameMode) ||
			Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Victory ||
			Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Defeat)
		{
			break;
		}

		TArray<ABattleUnit*> CandidateUnits;
		AppendChainCandidates(Context.GameMode->GetFriendlyUnits(), ChainOriginTile, HitUnits, CandidateUnits);
		AppendChainCandidates(Context.GameMode->GetEnemyUnits(), ChainOriginTile, HitUnits, CandidateUnits);

		if (CandidateUnits.Num() == 0)
		{
			break;
		}

		CurrentTarget = CandidateUnits[FMath::RandRange(0, CandidateUnits.Num() - 1)];
	}

	return bResolvedAnyEffect;
}

AGridTile* FCardResolver::GetResolvedTargetTile(const FCardResolveContext& Context)
{
	if (Context.TileTarget)
	{
		return Context.TileTarget.Get();
	}

	if (Context.UnitTarget)
	{
		return Context.UnitTarget->GetCurrentTile();
	}

	return nullptr;
}

ABattleUnit* FCardResolver::GetResolvedTargetUnit(const FCardResolveContext& Context)
{
	if (Context.UnitTarget)
	{
		return Context.UnitTarget.Get();
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	return TargetTile ? TargetTile->GetOccupyingUnit() : nullptr;
}
