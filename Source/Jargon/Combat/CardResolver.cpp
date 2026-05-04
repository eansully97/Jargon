// CardResolver.cpp

#include "Combat/CardResolver.h"

#include "Combat/Effects/JargonEffectContextBuilder.h"
#include "Combat/Effects/JargonEffectExecutor.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/JargonCombatGameMode.h"
#include "Data/CardDefinition.h"
#include "Data/CardScriptDefinition.h"
#include "Data/JargonTileEffectDefinition.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"
#include "Units/BattleUnit.h"

namespace
{
	TSet<int32> BuildSelectedElementalBonusIndexSet(const FCardResolveContext& Context)
	{
		TSet<int32> SelectedIndices;
		for (const int32 BonusIndex : Context.SelectedElementalBonusIndices)
		{
			SelectedIndices.Add(BonusIndex);
		}
		return SelectedIndices;
	}

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

	FText GetElementDisplayText(EJargonElementType ElementType)
	{
		const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
		return ElementEnum
			? ElementEnum->GetDisplayNameTextByValue(static_cast<int64>(ElementType))
			: FText::FromString(TEXT("Element"));
	}

	void MergeJargonResultIntoCardResult(
		const FJargonEffectResult& JargonResult,
		FCardResolveResult& OutResult)
	{
		OutResult.bConsumePlayerMove |= JargonResult.bConsumePlayerMove;
		OutResult.bContinuesAsynchronously |= JargonResult.bContinuesAsynchronously;
		OutResult.EnergyGainAfterCost += JargonResult.EnergyGainAfterCost;
	}

	void EmitElementalBonusTriggeredCue(
		const UCardDefinition* Card,
		const FCardResolveContext& Context,
		EJargonElementType ElementType,
		int32 BonusIndex,
		int32 RequiredCharges,
		bool bSpentCharges)
	{
		if (!Card || !Context.GameMode)
		{
			return;
		}

		AGridTile* TargetTile = Context.TileTarget.Get();
		if (!TargetTile && Context.UnitTarget)
		{
			TargetTile = Context.UnitTarget->GetCurrentTile();
		}

		AGridTile* SourceTile = Context.SourceUnit ? Context.SourceUnit->GetCurrentTile() : nullptr;

		FJargonCombatCueEvent Cue;
		Cue.CueType = EJargonCombatCueType::ElementalBonusTriggered;
		Cue.Trigger = EJargonEffectTrigger::OnPlayed;
		Cue.SourceObject = const_cast<UCardDefinition*>(Card);
		Cue.SourceCard = const_cast<UCardDefinition*>(Card);
		Cue.SourceUnit = Context.SourceUnit.Get();
		Cue.SourceTile = SourceTile;
		Cue.TargetUnit = Context.UnitTarget.Get();
		Cue.TargetTile = TargetTile;
		Cue.ElementType = ElementType;
		Cue.ElementChargeCount = RequiredCharges;
		Cue.bSpentElementCharges = bSpentCharges;
		Cue.ElementalBonusIndex = BonusIndex;
		Cue.Value = RequiredCharges;
		Cue.TextOverride = FText::Format(
			FText::FromString(TEXT("{0}: {1} Bonus")),
			Card->DisplayName.IsEmpty() ? FText::FromString(GetNameSafe(Card)) : Card->DisplayName,
			GetElementDisplayText(ElementType));

		if (TargetTile)
		{
			Cue.WorldLocation = TargetTile->GetActorLocation();
			Cue.bHasWorldLocation = true;
		}
		else if (Context.SourceUnit)
		{
			Cue.WorldLocation = Context.SourceUnit->GetActorLocation();
			Cue.bHasWorldLocation = true;
		}

		Context.GameMode->EmitCombatCue(Cue);
	}

}

bool FCardResolver::ResolveCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	return ResolveCard(Card, Context, OutResult, nullptr);
}

bool FCardResolver::ResolveCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult,
	FJargonEffectTrace* OutTrace
)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	if (!Card->UsesCardScript())
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has no CardScript authored. Cards resolve through CardScript effect lines only."),
			*Card->DisplayName.ToString());
		return false;
	}

	return ResolveEffectSpecCard(Card, Context, OutResult, OutTrace);
}

bool FCardResolver::ResolveEffectSpecCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	return ResolveEffectSpecCard(Card, Context, OutResult, nullptr);
}

bool FCardResolver::ResolveEffectSpecCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult,
	FJargonEffectTrace* OutTrace)
{
	if (!Card || !Card->UsesCardScript())
	{
		return false;
	}

	TArray<FJargonEffectSpec> JargonEffectSpecs;
	Card->BuildBaseEffectSpecs(JargonEffectSpecs);
	const FJargonEffectContext JargonContext = FJargonEffectContextBuilder::BuildForCard(
		Context.GameMode.Get(),
		const_cast<UCardDefinition*>(Card),
		Context.SourceUnit.Get(),
		Context.UnitTarget.Get(),
		Context.TileTarget.Get());

	FJargonEffectExecutionRequest BaseExecutionRequest;
	BaseExecutionRequest.Effects = &JargonEffectSpecs;
	BaseExecutionRequest.Context = JargonContext;
	BaseExecutionRequest.SourceLabel = Card->DisplayName.ToString();
	BaseExecutionRequest.HookName = TEXT("Card Base Effects");
	BaseExecutionRequest.OutTrace = OutTrace;
	BaseExecutionRequest.AsyncWarningPolicy = EJargonEffectAsyncWarningPolicy::Silent;
	const FJargonEffectExecutionReport BaseExecutionReport = FJargonEffectExecutor::Execute(BaseExecutionRequest);
	const FJargonEffectResult& JargonResult = BaseExecutionReport.Result;

	MergeJargonResultIntoCardResult(JargonResult, OutResult);

	if (!BaseExecutionReport.bResolverSucceeded)
	{
		return false;
	}

	if (JargonResult.bContinuesAsynchronously)
	{
		if (Context.SelectedElementalBonusIndices.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Card '%s' started an async base effect. Elemental bonus groups are skipped for this resolve pass."),
				*Card->DisplayName.ToString());
		}
		return true;
	}

	if (!Card->CardScript)
	{
		return true;
	}

	const TSet<int32> SelectedBonusIndices = BuildSelectedElementalBonusIndexSet(Context);
	if (SelectedBonusIndices.Num() <= 0)
	{
		return true;
	}

	for (const int32 SelectedBonusIndex : SelectedBonusIndices)
	{
		if (!Card->CardScript->ElementalBonuses.IsValidIndex(SelectedBonusIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Card '%s' selected invalid elemental bonus group index %d. The index is ignored."),
				*Card->DisplayName.ToString(),
				SelectedBonusIndex);
		}
	}

	for (int32 BonusIndex = 0; BonusIndex < Card->CardScript->ElementalBonuses.Num(); ++BonusIndex)
	{
		if (!SelectedBonusIndices.Contains(BonusIndex))
		{
			continue;
		}

		const FJargonCardElementalBonusScript& BonusGroup = Card->CardScript->ElementalBonuses[BonusIndex];
		if (BonusGroup.ElementType == EJargonElementType::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("Card '%s' selected elemental bonus group %d skipped because ElementType is None."),
				*Card->DisplayName.ToString(),
				BonusIndex);
			continue;
		}

		const int32 RequiredCharges = FMath::Max(0, BonusGroup.RequiredCharges);
		if (RequiredCharges <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Card '%s' selected elemental bonus group %d skipped because RequiredCharges is not positive."),
				*Card->DisplayName.ToString(),
				BonusIndex);
			continue;
		}

		if (!BonusGroup.HasAnyActions())
		{
			UE_LOG(LogTemp, Warning, TEXT("Card '%s' selected elemental bonus group %d skipped because it has no actions."),
				*Card->DisplayName.ToString(),
				BonusIndex);
			continue;
		}

		if (!Context.GameMode->HasElementCharges(BonusGroup.ElementType, RequiredCharges))
		{
			UE_LOG(LogTemp, Log, TEXT("Card '%s' selected elemental bonus group %d skipped because %d %s charges are no longer available."),
				*Card->DisplayName.ToString(),
				BonusIndex,
				RequiredCharges,
				*GetElementDisplayText(BonusGroup.ElementType).ToString());
			continue;
		}

		bool bSpentCharges = false;
		if (BonusGroup.bSpendCharges)
		{
			bSpentCharges = Context.GameMode->TrySpendElementCharges(BonusGroup.ElementType, RequiredCharges);
			if (!bSpentCharges)
			{
				UE_LOG(LogTemp, Warning, TEXT("Card '%s' elemental bonus group %d met its charge check but failed to spend charges."),
					*Card->DisplayName.ToString(),
					BonusIndex);
				continue;
			}

			UE_LOG(LogTemp, Log, TEXT("Card '%s' spent %d %s charges for elemental bonus group %d."),
				*Card->DisplayName.ToString(),
				RequiredCharges,
				*GetElementDisplayText(BonusGroup.ElementType).ToString(),
				BonusIndex);
		}

		TArray<FJargonEffectSpec> BonusJargonEffects;
		Card->BuildElementalBonusEffectSpecs(BonusIndex, BonusJargonEffects);
		FJargonEffectExecutionRequest BonusExecutionRequest;
		BonusExecutionRequest.Effects = &BonusJargonEffects;
		BonusExecutionRequest.Context = JargonContext;
		BonusExecutionRequest.SourceLabel = Card->DisplayName.ToString();
		BonusExecutionRequest.HookName = FString::Printf(TEXT("Elemental Bonus %d"), BonusIndex);
		BonusExecutionRequest.AsyncWarningPolicy = EJargonEffectAsyncWarningPolicy::Silent;
		const FJargonEffectExecutionReport BonusExecutionReport = FJargonEffectExecutor::Execute(BonusExecutionRequest);
		const FJargonEffectResult& BonusJargonResult = BonusExecutionReport.Result;
		if (!BonusExecutionReport.bResolverSucceeded)
		{
			if (bSpentCharges)
			{
				Context.GameMode->GainElementCharges(BonusGroup.ElementType, RequiredCharges);
			}

			UE_LOG(LogTemp, Warning, TEXT("Card '%s' elemental bonus group %d did not resolve any effects."),
				*Card->DisplayName.ToString(),
				BonusIndex);
			continue;
		}

		EmitElementalBonusTriggeredCue(
			Card,
			Context,
			BonusGroup.ElementType,
			BonusIndex,
			RequiredCharges,
			bSpentCharges);

		MergeJargonResultIntoCardResult(BonusJargonResult, OutResult);

		if (BonusJargonResult.bContinuesAsynchronously)
		{
			if (BonusIndex < Card->CardScript->ElementalBonuses.Num() - 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Card '%s' elemental bonus group %d started an async effect. Later bonus groups are skipped for this resolve pass."),
					*Card->DisplayName.ToString(),
					BonusIndex);
			}
			break;
		}
	}

	return true;
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
