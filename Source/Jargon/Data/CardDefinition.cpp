// Fill out your copyright notice in the Description page of Project Settings.


#include "CardDefinition.h"

#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardScriptDefinition.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/JargonDataAssetValidationHelpers.h"
#include "Misc/DataValidation.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCardDefinitionPrompt, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogCardDefinitionSummonAuthoring, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogCardDefinitionTileEffectAuthoring, Log, All);

namespace
{
FString InsertSpacesIntoDisplayToken(const FString& Token)
{
	FString Result;
	Result.Reserve(Token.Len() + 4);

	for (int32 Index = 0; Index < Token.Len(); ++Index)
	{
		const TCHAR Character = Token[Index];
		const bool bShouldAddSpace =
			Index > 0
			&& FChar::IsUpper(Character)
			&& (FChar::IsLower(Token[Index - 1]) || (Index + 1 < Token.Len() && FChar::IsLower(Token[Index + 1])));

		if (bShouldAddSpace)
		{
			Result.AppendChar(TEXT(' '));
		}

		Result.AppendChar(Character);
	}

	return Result;
}

FString DeriveDisplayNameFromAssetName(FString AssetName)
{
	AssetName.RemoveFromStart(TEXT("DA_Card_"));
	AssetName.RemoveFromStart(TEXT("Card_"));
	AssetName.ReplaceInline(TEXT("_"), TEXT(" "), ESearchCase::CaseSensitive);

	TArray<FString> Tokens;
	AssetName.ParseIntoArray(Tokens, TEXT(" "), true);

	for (FString& Token : Tokens)
	{
		Token = InsertSpacesIntoDisplayToken(Token);
	}

	return Tokens.Num() > 0 ? FString::Join(Tokens, TEXT(" ")) : TEXT("New Card");
}

FString GetCardPromptName(const UCardDefinition* Card)
{
	if (!Card)
	{
		return TEXT("New Card");
	}

	const FString DisplayName = Card->DisplayName.ToString().TrimStartAndEnd();
	return DisplayName.IsEmpty() ? DeriveDisplayNameFromAssetName(Card->GetName()) : DisplayName;
}

FString GetEnumDisplayName(const UEnum* Enum, int64 Value)
{
	return Enum
		? Enum->GetDisplayNameTextByValue(Value).ToString()
		: FString::FromInt(static_cast<int32>(Value));
}

FString GetCardCategoryDisplayName(ECardCategory Category)
{
	return GetEnumDisplayName(StaticEnum<ECardCategory>(), static_cast<int64>(Category));
}

FString GetCardTargetTypeDisplayName(ECardTargetType TargetType)
{
	return GetEnumDisplayName(StaticEnum<ECardTargetType>(), static_cast<int64>(TargetType));
}

FString GetElementTypeDisplayName(EJargonElementType ElementType)
{
	if (ElementType == EJargonElementType::None)
	{
		return TEXT("Neutral");
	}

	return GetEnumDisplayName(StaticEnum<EJargonElementType>(), static_cast<int64>(ElementType));
}

FString DeriveCardArtAssetNameFromCardAssetName(FString CardAssetName)
{
	CardAssetName.RemoveFromStart(TEXT("DA_"));

	if (!CardAssetName.StartsWith(TEXT("Card_")))
	{
		CardAssetName = TEXT("Card_") + CardAssetName;
	}

	return TEXT("T_") + CardAssetName;
}

FString DerivePromptFileNameFromCard(const UCardDefinition* Card)
{
	return DeriveCardArtAssetNameFromCardAssetName(Card ? Card->GetName() : TEXT("DA_Card_NewCard")) + TEXT("_prompt.txt");
}

FString DeriveSummonDefinitionAssetNameFromCardAssetName(FString CardAssetName)
{
	CardAssetName.RemoveFromStart(TEXT("DA_"));
	CardAssetName.RemoveFromStart(TEXT("Card_"));
	CardAssetName.RemoveFromStart(TEXT("Summon_"));
	CardAssetName.ReplaceInline(TEXT(" "), TEXT("_"));

	return FString::Printf(TEXT("DA_SummonUnit_%s"), *CardAssetName);
}

FString DeriveTileEffectDefinitionAssetNameFromCardAssetName(FString CardAssetName)
{
	CardAssetName.RemoveFromStart(TEXT("DA_"));
	CardAssetName.RemoveFromStart(TEXT("Card_"));
	CardAssetName.RemoveFromStart(TEXT("Trap_"));
	CardAssetName.RemoveFromStart(TEXT("Aura_"));
	CardAssetName.ReplaceInline(TEXT(" "), TEXT("_"));

	return FString::Printf(TEXT("DA_TileEffect_%s"), *CardAssetName);
}

FString NormalizeLongPackageFolderPath(FString FolderPath)
{
	FolderPath.TrimStartAndEndInline();
	FolderPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	while (FolderPath.Len() > 1 && FolderPath.EndsWith(TEXT("/")))
	{
		FolderPath.LeftChopInline(1);
	}

	return FolderPath;
}

bool TrySelectSummonActionForDefinitionGeneration(
	UCardDefinition* Card,
	bool bReplaceExistingDefinition,
	UJargonCardSummonAction*& OutAction,
	FString& OutFailureReason)
{
	OutAction = nullptr;
	OutFailureReason.Reset();

	if (!Card)
	{
		OutFailureReason = TEXT("No card definition was provided.");
		return false;
	}

	if (!Card->CardScript)
	{
		OutFailureReason = TEXT("card has no CardScript.");
		return false;
	}

	TArray<UJargonCardSummonAction*> SummonActions;
	TArray<UJargonCardSummonAction*> UnassignedSummonActions;
	for (TObjectPtr<UJargonCardAction>& ActionPtr : Card->CardScript->Actions)
	{
		UJargonCardSummonAction* SummonAction = Cast<UJargonCardSummonAction>(ActionPtr.Get());
		if (!SummonAction)
		{
			continue;
		}

		SummonActions.Add(SummonAction);
		if (!SummonAction->SummonedUnitDefinition)
		{
			UnassignedSummonActions.Add(SummonAction);
		}
	}

	if (SummonActions.Num() <= 0)
	{
		OutFailureReason = TEXT("card has no summon keyword actions.");
		return false;
	}

	if (SummonActions.Num() == 1)
	{
		OutAction = SummonActions[0];
		return true;
	}

	if (UnassignedSummonActions.Num() == 1)
	{
		OutAction = UnassignedSummonActions[0];
		return true;
	}

	if (UnassignedSummonActions.Num() > 1)
	{
		OutFailureReason = TEXT("card has multiple unassigned summon keywords; assign one manually or generate definitions one at a time.");
		return false;
	}

	if (bReplaceExistingDefinition)
	{
		OutFailureReason = TEXT("card has multiple summon keywords that already have definitions; this button does not guess which existing reference to replace.");
	}
	else
	{
		OutFailureReason = TEXT("card has multiple summon keywords and all already have definitions.");
	}
	return false;
}

bool TrySelectTileEffectActionForDefinitionGeneration(
	UCardDefinition* Card,
	bool bReplaceExistingDefinition,
	UJargonCardPlaceTileEffectAction*& OutAction,
	FString& OutFailureReason)
{
	OutAction = nullptr;
	OutFailureReason.Reset();

	if (!Card)
	{
		OutFailureReason = TEXT("No card definition was provided.");
		return false;
	}

	if (!Card->CardScript)
	{
		OutFailureReason = TEXT("card has no CardScript.");
		return false;
	}

	TArray<UJargonCardPlaceTileEffectAction*> TileEffectActions;
	TArray<UJargonCardPlaceTileEffectAction*> UnassignedTileEffectActions;
	for (TObjectPtr<UJargonCardAction>& ActionPtr : Card->CardScript->Actions)
	{
		UJargonCardPlaceTileEffectAction* TileEffectAction = Cast<UJargonCardPlaceTileEffectAction>(ActionPtr.Get());
		if (!TileEffectAction)
		{
			continue;
		}

		TileEffectActions.Add(TileEffectAction);
		if (!TileEffectAction->TileEffectDefinition)
		{
			UnassignedTileEffectActions.Add(TileEffectAction);
		}
	}

	if (TileEffectActions.Num() <= 0)
	{
		OutFailureReason = TEXT("card has no place-tile-effect keyword actions.");
		return false;
	}

	if (TileEffectActions.Num() == 1)
	{
		OutAction = TileEffectActions[0];
		return true;
	}

	if (UnassignedTileEffectActions.Num() == 1)
	{
		OutAction = UnassignedTileEffectActions[0];
		return true;
	}

	if (UnassignedTileEffectActions.Num() > 1)
	{
		OutFailureReason = TEXT("card has multiple unassigned place-tile-effect keywords; assign one manually or generate definitions one at a time.");
		return false;
	}

	if (bReplaceExistingDefinition)
	{
		OutFailureReason = TEXT("card has multiple place-tile-effect keywords that already have definitions; this button does not guess which existing reference to replace.");
	}
	else
	{
		OutFailureReason = TEXT("card has multiple place-tile-effect keywords and all already have definitions.");
	}
	return false;
}

FString BuildObjectPathFromPackageAndAssetName(const FString& PackageName, const FString& AssetName)
{
	return FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);
}


FString GetEffectOperationDisplayName(EJargonEffectOperation Operation)
{
	return GetEnumDisplayName(StaticEnum<EJargonEffectOperation>(), static_cast<int64>(Operation));
}

bool CardHasOperation(const UCardDefinition* Card, EJargonEffectOperation Operation)
{
	return Card && Card->HasEffectOperation(Operation);
}

FString BuildCardEffectAuditSummary(const FJargonEffectSpec& EffectSpec)
{
	const FString PayloadSummary = JargonEffectContracts::BuildPayloadSummary(EffectSpec);
	return PayloadSummary.IsEmpty() || PayloadSummary == TEXT("None")
		? GetEffectOperationDisplayName(EffectSpec.Operation)
		: FString::Printf(TEXT("%s %s"), *GetEffectOperationDisplayName(EffectSpec.Operation), *PayloadSummary);
}

FString BuildEffectListSummary(const TArray<FJargonEffectSpec>& Effects)
{
	if (Effects.Num() == 0)
	{
		return TEXT("None");
	}

	TArray<FString> EffectSummaries;
	EffectSummaries.Reserve(Effects.Num());
	for (const FJargonEffectSpec& EffectSpec : Effects)
	{
		EffectSummaries.Add(BuildCardEffectAuditSummary(EffectSpec));
	}

	return FString::Join(EffectSummaries, TEXT("; "));
}

FString BuildCardEffectPromptSummary(const UCardDefinition* Card)
{
	return Card && Card->CardScript ? Card->CardScript->GetScriptSummary() : TEXT("None");
}

void AddElementIfMeaningful(TArray<EJargonElementType>& Elements, EJargonElementType ElementType)
{
	if (ElementType != EJargonElementType::None)
	{
		Elements.AddUnique(ElementType);
	}
}

bool IsDebugCardAsset(const UCardDefinition* Card)
{
	return Card && Card->GetPathName().Contains(TEXT("/Debug/"), ESearchCase::IgnoreCase);
}

void GatherExplicitCardScriptElements(const UCardDefinition* Card, TArray<EJargonElementType>& OutElements)
{
	OutElements.Reset();
	if (!Card)
	{
		return;
	}

	TArray<FJargonEffectSpec> BaseEffects;
	Card->BuildBaseEffectSpecs(BaseEffects);
	for (const FJargonEffectSpec& EffectSpec : BaseEffects)
	{
		if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
		{
			AddElementIfMeaningful(OutElements, EffectSpec.ElementType);
		}
	}

	if (!Card->CardScript)
	{
		return;
	}

	for (int32 BonusIndex = 0; BonusIndex < Card->CardScript->ElementalBonuses.Num(); ++BonusIndex)
	{
		const FJargonCardElementalBonusScript& BonusGroup = Card->CardScript->ElementalBonuses[BonusIndex];
		AddElementIfMeaningful(OutElements, BonusGroup.ElementType);

		TArray<FJargonEffectSpec> BonusEffects;
		Card->BuildElementalBonusEffectSpecs(BonusIndex, BonusEffects);
		for (const FJargonEffectSpec& BonusEffectSpec : BonusEffects)
		{
			if (BonusEffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
			{
				AddElementIfMeaningful(OutElements, BonusEffectSpec.ElementType);
			}
		}
	}
}

FString JoinElementNames(const TArray<EJargonElementType>& Elements)
{
	TArray<FString> ElementNames;
	ElementNames.Reserve(Elements.Num());
	for (const EJargonElementType ElementType : Elements)
	{
		ElementNames.Add(GetElementTypeDisplayName(ElementType));
	}

	return FString::Join(ElementNames, TEXT(", "));
}

bool CardElementMatchesExplicitElements(const UCardDefinition* Card)
{
	if (!Card)
	{
		return true;
	}

	TArray<EJargonElementType> ExplicitElements;
	GatherExplicitCardScriptElements(Card, ExplicitElements);
	for (const EJargonElementType ExplicitElement : ExplicitElements)
	{
		if (ExplicitElement != Card->CardElement)
		{
			return false;
		}
	}

	return true;
}

bool EffectSuggestsElementalIdentity(const FJargonEffectSpec& EffectSpec)
{
	if (EffectSpec.Operation == EJargonEffectOperation::ApplyStatus && EffectSpec.StatusEffectDefinition)
	{
		switch (EffectSpec.StatusEffectDefinition->StatusKind)
		{
		case EJargonStatusEffectKind::Stun:
		case EJargonStatusEffectKind::Freeze:
		case EJargonStatusEffectKind::Burn:
		case EJargonStatusEffectKind::Root:
		case EJargonStatusEffectKind::Vulnerable:
		case EJargonStatusEffectKind::Regen:
		case EJargonStatusEffectKind::Weak:
			return true;
		default:
			break;
		}
	}

	const FString ReferencedDefinitionNames = FString::Printf(
		TEXT("%s %s"),
		*GetNameSafe(EffectSpec.SummonedUnitDefinition.Get()),
		*GetNameSafe(EffectSpec.TileEffectDefinition.Get()));
	return ReferencedDefinitionNames.Contains(TEXT("Fire"), ESearchCase::IgnoreCase)
		|| ReferencedDefinitionNames.Contains(TEXT("Frost"), ESearchCase::IgnoreCase)
		|| ReferencedDefinitionNames.Contains(TEXT("Storm"), ESearchCase::IgnoreCase)
		|| ReferencedDefinitionNames.Contains(TEXT("Nature"), ESearchCase::IgnoreCase)
		|| ReferencedDefinitionNames.Contains(TEXT("Radiance"), ESearchCase::IgnoreCase)
		|| ReferencedDefinitionNames.Contains(TEXT("Quietus"), ESearchCase::IgnoreCase);
}

bool NeutralCardHasElementalIdentitySignals(const UCardDefinition* Card)
{
	if (!Card || Card->CardElement != EJargonElementType::None)
	{
		return false;
	}

	TArray<FJargonEffectSpec> BaseEffects;
	Card->BuildBaseEffectSpecs(BaseEffects);
	for (const FJargonEffectSpec& EffectSpec : BaseEffects)
	{
		if (EffectSuggestsElementalIdentity(EffectSpec))
		{
			return true;
		}
	}

	if (!Card->CardScript)
	{
		return false;
	}

	for (int32 BonusIndex = 0; BonusIndex < Card->CardScript->ElementalBonuses.Num(); ++BonusIndex)
	{
		TArray<FJargonEffectSpec> BonusEffects;
		Card->BuildElementalBonusEffectSpecs(BonusIndex, BonusEffects);
		for (const FJargonEffectSpec& BonusEffectSpec : BonusEffects)
		{
			if (EffectSuggestsElementalIdentity(BonusEffectSpec))
			{
				return true;
			}
		}
	}

	return false;
}

FString DeriveCardElementIdentity(const UCardDefinition* Card)
{
	TArray<EJargonElementType> Elements;
	if (Card)
	{
		TArray<FJargonEffectSpec> BaseEffects;
		Card->BuildBaseEffectSpecs(BaseEffects);
		for (const FJargonEffectSpec& EffectSpec : BaseEffects)
		{
			if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
			{
				AddElementIfMeaningful(Elements, EffectSpec.ElementType);
			}
		}

		if (Card->CardScript)
		{
			for (int32 BonusIndex = 0; BonusIndex < Card->CardScript->ElementalBonuses.Num(); ++BonusIndex)
			{
				const FJargonCardElementalBonusScript& BonusGroup = Card->CardScript->ElementalBonuses[BonusIndex];
				AddElementIfMeaningful(Elements, BonusGroup.ElementType);

				TArray<FJargonEffectSpec> BonusEffects;
				Card->BuildElementalBonusEffectSpecs(BonusIndex, BonusEffects);
				for (const FJargonEffectSpec& BonusEffectSpec : BonusEffects)
				{
					if (BonusEffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
					{
						AddElementIfMeaningful(Elements, BonusEffectSpec.ElementType);
					}
				}
			}
		}
	}

	if (Elements.Num() == 0)
	{
		return TEXT("Neutral fantasy magic / no explicit element");
	}

	TArray<FString> ElementNames;
	for (const EJargonElementType ElementType : Elements)
	{
		ElementNames.Add(GetElementTypeDisplayName(ElementType));
	}

	return FString::Join(ElementNames, TEXT(" / "));
}

FString DeriveCardMainSubject(const UCardDefinition* Card)
{
	const FString CardName = GetCardPromptName(Card);
	if (!Card)
	{
		return CardName;
	}

	switch (Card->Category)
	{
	case ECardCategory::Summon:
		return FString::Printf(TEXT("The summoned creature, hero, or ally represented by '%s'."), *CardName);

	case ECardCategory::Trap:
		return FString::Printf(TEXT("A dangerous magical trap or hazard representing '%s'."), *CardName);

	case ECardCategory::Aura:
		return FString::Printf(TEXT("A visible magical aura, field, or blessing representing '%s'."), *CardName);

	default:
		break;
	}

	if (CardHasOperation(Card, EJargonEffectOperation::Heal))
	{
		return FString::Printf(TEXT("A restorative magical moment or healer's power representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, EJargonEffectOperation::ApplyShield))
	{
		return FString::Printf(TEXT("A protective magical barrier or guardian force representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, EJargonEffectOperation::ApplyStatus))
	{
		return FString::Printf(TEXT("A visible magical condition, mark, or disabling strike representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, EJargonEffectOperation::MoveSource))
	{
		return FString::Printf(TEXT("A swift movement, dash, leap, or repositioning moment representing '%s'."), *CardName);
	}

	return FString::Printf(TEXT("The main character, creature, object, or magical event implied by '%s'."), *CardName);
}

FString DeriveCardMood(const UCardDefinition* Card)
{
	const FString ElementIdentity = DeriveCardElementIdentity(Card);
	if (ElementIdentity.Contains(TEXT("Quietus"), ESearchCase::IgnoreCase))
	{
		return TEXT("Dark, dangerous, eerie, mystical.");
	}

	if (ElementIdentity.Contains(TEXT("Radiance"), ESearchCase::IgnoreCase))
	{
		return TEXT("Heroic, luminous, sacred, hopeful.");
	}

	if (ElementIdentity.Contains(TEXT("Frost"), ESearchCase::IgnoreCase))
	{
		return TEXT("Cold, crystalline, still, dangerous.");
	}

	if (ElementIdentity.Contains(TEXT("Nature"), ESearchCase::IgnoreCase))
	{
		return TEXT("Mystical, organic, restorative, wild.");
	}

	if (ElementIdentity.Contains(TEXT("Storm"), ESearchCase::IgnoreCase))
	{
		return TEXT("Energetic, volatile, electric, dramatic.");
	}

	if (ElementIdentity.Contains(TEXT("Fire"), ESearchCase::IgnoreCase))
	{
		return TEXT("Dangerous, intense, fiery, dramatic.");
	}

	if (Card && Card->Category == ECardCategory::Trap)
	{
		return TEXT("Tense, dangerous, mysterious.");
	}

	if (Card && Card->Category == ECardCategory::Summon)
	{
		return TEXT("Epic, character-focused, heroic.");
	}

	return TEXT("Epic, mystical, readable, game-ready.");
}

bool ValidateJargonEffectSpecRuntime(
	const UCardDefinition* Card,
	const FJargonEffectSpec& EffectSpec,
	const FString& EffectLabel)
{
	if (EffectSpec.Operation == EJargonEffectOperation::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s has operation None."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (JargonEffectContracts::RequiresValue(EffectSpec.Operation) && EffectSpec.Value <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires Value > 0."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (JargonEffectContracts::RequiresStatusDefinition(EffectSpec.Operation) && !EffectSpec.StatusEffectDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires StatusEffectDefinition."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (JargonEffectContracts::RequiresElementType(EffectSpec.Operation) && EffectSpec.ElementType == EJargonElementType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires ElementType other than None."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::MoveSource && EffectSpec.MoveDistance <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires MoveDistance > 0."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PushTarget && EffectSpec.PushDistance <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires PushDistance > 0."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::PullTarget && EffectSpec.PullDistance <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires PullDistance > 0."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	if (JargonEffectContracts::RequiresSummonPayload(EffectSpec.Operation))
	{
		if (!EffectSpec.SummonedUnitDefinition || !EffectSpec.RuntimeSummonedUnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires summon definition and runtime class."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
	}

	if (JargonEffectContracts::RequiresTileEffectPayload(EffectSpec.Operation))
	{
		if (!EffectSpec.TileEffectDefinition || !EffectSpec.RuntimeTileEffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires tile effect definition and runtime class."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
	}

	return true;
}

FString BuildCardArtPromptText(const UCardDefinition* Card)
{
	if (!Card)
	{
		return FString();
	}

	const FString CardName = GetCardPromptName(Card);
	const FString ElementIdentity = DeriveCardElementIdentity(Card);
	const FString CardType = GetCardCategoryDisplayName(Card->Category);
	const FString TargetType = GetCardTargetTypeDisplayName(Card->TargetType);
	const FString Description = Card->Description.ToString().TrimStartAndEnd();
	const FString PromptDescription = Description.IsEmpty()
		? TEXT("No written description authored yet; use the card name, type, and effect summary to infer the visual identity.")
		: Description;
	const FString EffectSummary = BuildCardEffectPromptSummary(Card);
	const bool bHasEffectSummary = !EffectSummary.IsEmpty() && !EffectSummary.Equals(TEXT("None"), ESearchCase::IgnoreCase);
	const FString PromptEffectDescription = bHasEffectSummary
		? (Description.IsEmpty() ? EffectSummary : FString::Printf(TEXT("%s (%s)"), *Description, *EffectSummary))
		: PromptDescription;
	const FString MainSubject = DeriveCardMainSubject(Card);
	const FString Mood = DeriveCardMood(Card);

	return FString::Printf(
		TEXT("Create high-quality fantasy trading card artwork.\n\n")
		TEXT("Canvas / dimensions:\n")
		TEXT("Strict vertical image, 720x1280 pixels, 9:16 aspect ratio.\n")
		TEXT("The artwork must be clear, readable, and composed for a card game.\n\n")
		TEXT("Card details:\n")
		TEXT("Card name: %s\n")
		TEXT("Element / class: %s\n")
		TEXT("Card type: %s\n")
		TEXT("Target type: %s\n")
		TEXT("Card description / effect: %s\n")
		TEXT("\n")
		TEXT("Main subject:\n")
		TEXT("A single, clear visual moment representing the effect:\n")
		TEXT("%s\n\n")
		TEXT("Mood:\n")
		TEXT("%s\n\n")
		TEXT("---\n\n")
		TEXT("Art direction:\n")
		TEXT("Stylized realistic fantasy card art, inspired by premium digital card games.\n\n")
		TEXT("Composition:\n")
		TEXT("Vertical portrait composition.\n")
		TEXT("Strong central focal point.\n")
		TEXT("Clear foreground, midground, and background separation.\n")
		TEXT("Use 2-3 large, simple forms to define the effect.\n")
		TEXT("No clutter, no repeated overlapping elements.\n\n")
		TEXT("---\n\n")
		TEXT("Style constraints:\n")
		TEXT("Clean, bold shapes with strong readability.\n")
		TEXT("Smooth, painted surfaces - not hyper-detailed or noisy.\n")
		TEXT("Controlled detail density - high detail only on focal subject.\n\n")
		TEXT("Prefer thick, simple forms over many thin elements.\n")
		TEXT("Limit to a few dominant shapes (no complexity stacking).\n\n")
		TEXT("No thin filament details.\n")
		TEXT("No mesh-like patterns.\n")
		TEXT("No web-like distortions.\n")
		TEXT("No stringy noise or interconnected micro-lines.\n\n")
		TEXT("Shapes must remain clearly separated and readable.\n\n")
		TEXT("---\n\n")
		TEXT("Lighting:\n")
		TEXT("Cinematic but controlled.\n")
		TEXT("One primary light source.\n")
		TEXT("Soft secondary lighting.\n")
		TEXT("Glow is minimal and concentrated (not everywhere).\n\n")
		TEXT("---\n\n")
		TEXT("Background:\n")
		TEXT("Soft, blurred, low detail, slightly desaturated.\n")
		TEXT("Supports the subject without distraction.\n\n")
		TEXT("---\n\n")
		TEXT("Quality:\n")
		TEXT("Polished digital painting.\n")
		TEXT("Clean edges.\n")
		TEXT("Readable at small card size.\n")
		TEXT("Strong silhouette.\n")
		TEXT("Visually striking but not noisy.\n\n")
		TEXT("---\n\n")
		TEXT("Hard constraints (IMPORTANT):\n")
		TEXT("Limit visual elements to 2-3 primary shapes.\n")
		TEXT("Avoid repeated patterns.\n")
		TEXT("Avoid micro-detail noise.\n")
		TEXT("Avoid excessive texture.\n")
		TEXT("Avoid complex overlapping structures.\n\n")
		TEXT("---\n\n")
		TEXT("Negative prompt:\n")
		TEXT("low quality, blurry, muddy, cluttered, over-detailed, noisy, unreadable,\n")
		TEXT("thin lines, webbing, mesh patterns, stringy artifacts,\n")
		TEXT("bad anatomy, distortion, extra limbs,\n")
		TEXT("text, letters, numbers, watermark, logo, UI, border, frame\n"),
		*CardName,
		*ElementIdentity,
		*CardType,
		*TargetType,
		*PromptEffectDescription,
		*MainSubject,
		*Mood);
}

}

void UCardDefinition::GenerateCardArtPrompt() const
{
#if WITH_EDITOR
	if (!bGenerateArtPrompt)
	{
		UE_LOG(LogCardDefinitionPrompt, Display, TEXT("Prompt generation is disabled for card '%s'."), *GetNameSafe(this));
		return;
	}

	const FString Prompt = BuildCardArtPromptText(this);
	if (Prompt.IsEmpty())
	{
		UE_LOG(LogCardDefinitionPrompt, Warning, TEXT("Could not generate an art prompt for card '%s'."), *GetNameSafe(this));
		return;
	}

	const FString OutputDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CardArtPrompts")));
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);

	const FString PromptFilePath = FPaths::Combine(OutputDirectory, DerivePromptFileNameFromCard(this));
	if (!FFileHelper::SaveStringToFile(Prompt, *PromptFilePath))
	{
		UE_LOG(LogCardDefinitionPrompt, Error, TEXT("Failed to write card art prompt for '%s': %s"),
			*GetNameSafe(this),
			*PromptFilePath);
		return;
	}

	UE_LOG(LogCardDefinitionPrompt, Display, TEXT("Card art prompt saved for '%s': %s"),
		*GetCardPromptName(this),
		*PromptFilePath);
	UE_LOG(LogCardDefinitionPrompt, Display, TEXT("Prompt:\n%s"), *Prompt);
#else
	UE_LOG(LogCardDefinitionPrompt, Warning, TEXT("Card art prompt generation is editor-only."));
#endif
}

void UCardDefinition::GenerateSummonUnitDefinition()
{
#if WITH_EDITOR
	if (!bGenerateSummonUnitDefinition)
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Summon definition generation is disabled for card '%s'. Enable bGenerateSummonUnitDefinition first."),
			*GetNameSafe(this));
		return;
	}

	UJargonCardSummonAction* SelectedAction = nullptr;
	FString FailureReason;
	if (!TrySelectSummonActionForDefinitionGeneration(this, bReplaceExistingSummonDefinition, SelectedAction, FailureReason))
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Could not generate summon definition for card '%s': %s"),
			*GetNameSafe(this),
			*FailureReason);
		return;
	}

	if (SelectedAction->SummonedUnitDefinition && !bReplaceExistingSummonDefinition)
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Card '%s' selected summon keyword already has SummonedUnitDefinition '%s'. Enable bReplaceExistingSummonDefinition to replace the card reference."),
			*GetNameSafe(this),
			*GetPathNameSafe(SelectedAction->SummonedUnitDefinition.Get()));
		return;
	}

	const FString NormalizedOutputFolder = NormalizeLongPackageFolderPath(SummonDefinitionOutputFolder);
	FText PathError;
	if (!FPackageName::IsValidLongPackageName(NormalizedOutputFolder, true, &PathError))
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Card '%s' has invalid SummonDefinitionOutputFolder '%s': %s"),
			*GetNameSafe(this),
			*SummonDefinitionOutputFolder,
			*PathError.ToString());
		return;
	}

	const FString SummonDefinitionAssetName = DeriveSummonDefinitionAssetNameFromCardAssetName(GetName());
	const FString SummonDefinitionPackageName = FString::Printf(TEXT("%s/%s"), *NormalizedOutputFolder, *SummonDefinitionAssetName);
	const FString SummonDefinitionObjectPath = BuildObjectPathFromPackageAndAssetName(SummonDefinitionPackageName, SummonDefinitionAssetName);

	UJargonSummonedUnitDefinition* Definition = Cast<UJargonSummonedUnitDefinition>(
		StaticLoadObject(UJargonSummonedUnitDefinition::StaticClass(), nullptr, *SummonDefinitionObjectPath));
	const bool bExistingDefinitionAsset = Definition != nullptr;
	if (bExistingDefinitionAsset && !bReplaceExistingSummonDefinition)
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Summon definition asset already exists for card '%s': %s. Enable bReplaceExistingSummonDefinition to reuse it."),
			*GetNameSafe(this),
			*SummonDefinitionObjectPath);
		return;
	}

	if (!Definition)
	{
		if (UObject* ExistingObject = StaticLoadObject(UObject::StaticClass(), nullptr, *SummonDefinitionObjectPath))
		{
			UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Cannot create summon definition for card '%s': target object exists but is not a UJargonSummonedUnitDefinition: %s"),
				*GetNameSafe(this),
				*GetPathNameSafe(ExistingObject));
			return;
		}

		UPackage* Package = CreatePackage(*SummonDefinitionPackageName);
		if (!Package)
		{
			UE_LOG(LogCardDefinitionSummonAuthoring, Error, TEXT("Failed to create package for summon definition '%s'."), *SummonDefinitionPackageName);
			return;
		}

		Definition = NewObject<UJargonSummonedUnitDefinition>(
			Package,
			UJargonSummonedUnitDefinition::StaticClass(),
			*SummonDefinitionAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Definition)
		{
			UE_LOG(LogCardDefinitionSummonAuthoring, Error, TEXT("Failed to create summon definition asset '%s'."), *SummonDefinitionObjectPath);
			return;
		}

		Definition->Modify();
		Definition->DisplayName = DisplayName.IsEmpty()
			? FText::FromString(DeriveDisplayNameFromAssetName(GetName()))
			: DisplayName;
		if (!Description.IsEmpty())
		{
			Definition->Description = Description;
		}
		Definition->bSummonEntersWithAttackExhausted = SelectedAction->bSummonEntersWithAttackExhausted;

		FAssetRegistryModule::AssetCreated(Definition);
		Definition->MarkPackageDirty();
		Package->MarkPackageDirty();

		UE_LOG(LogCardDefinitionSummonAuthoring, Display, TEXT("Created summon definition '%s' for card '%s'."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Display, TEXT("Reusing existing summon definition '%s' for card '%s'. Existing definition fields were not overwritten."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this));
	}

	if (bAssignGeneratedSummonDefinition)
	{
		Modify();
		if (CardScript)
		{
			CardScript->Modify();
		}
		SelectedAction->Modify();
		SelectedAction->SummonedUnitDefinition = Definition;
		SelectedAction->RefreshEditorTitle();
		MarkPackageDirty();
		UE_LOG(LogCardDefinitionSummonAuthoring, Display, TEXT("Assigned summon definition '%s' to card '%s' selected summon keyword. RuntimeSummonedUnitClass remains authored separately on the keyword."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Display, TEXT("Created/reused summon definition '%s' but did not assign it because bAssignGeneratedSummonDefinition is false."),
			*GetPathNameSafe(Definition));
	}
#else
	UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Summon definition generation is editor-only."));
#endif
}

void UCardDefinition::GenerateTileEffectDefinition()
{
#if WITH_EDITOR
	if (!bGenerateTileEffectDefinition)
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Tile effect definition generation is disabled for card '%s'. Enable bGenerateTileEffectDefinition first."),
			*GetNameSafe(this));
		return;
	}

	if (Category != ECardCategory::Trap && Category != ECardCategory::Aura)
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Could not generate tile effect definition for card '%s': card Category must be Trap or Aura."),
			*GetNameSafe(this));
		return;
	}

	UJargonCardPlaceTileEffectAction* SelectedAction = nullptr;
	FString FailureReason;
	if (!TrySelectTileEffectActionForDefinitionGeneration(this, bReplaceExistingTileEffectDefinition, SelectedAction, FailureReason))
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Could not generate tile effect definition for card '%s': %s"),
			*GetNameSafe(this),
			*FailureReason);
		return;
	}

	if (SelectedAction->TileEffectDefinition && !bReplaceExistingTileEffectDefinition)
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Card '%s' selected tile-effect keyword already has TileEffectDefinition '%s'. Enable bReplaceExistingTileEffectDefinition to replace the card reference."),
			*GetNameSafe(this),
			*GetPathNameSafe(SelectedAction->TileEffectDefinition.Get()));
		return;
	}

	const FString NormalizedOutputFolder = NormalizeLongPackageFolderPath(TileEffectDefinitionOutputFolder);
	FText PathError;
	if (!FPackageName::IsValidLongPackageName(NormalizedOutputFolder, true, &PathError))
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Card '%s' has invalid TileEffectDefinitionOutputFolder '%s': %s"),
			*GetNameSafe(this),
			*TileEffectDefinitionOutputFolder,
			*PathError.ToString());
		return;
	}

	const FString TileEffectDefinitionAssetName = DeriveTileEffectDefinitionAssetNameFromCardAssetName(GetName());
	const FString TileEffectDefinitionPackageName = FString::Printf(TEXT("%s/%s"), *NormalizedOutputFolder, *TileEffectDefinitionAssetName);
	const FString TileEffectDefinitionObjectPath = BuildObjectPathFromPackageAndAssetName(TileEffectDefinitionPackageName, TileEffectDefinitionAssetName);

	UJargonTileEffectDefinition* Definition = Cast<UJargonTileEffectDefinition>(
		StaticLoadObject(UJargonTileEffectDefinition::StaticClass(), nullptr, *TileEffectDefinitionObjectPath));
	const bool bExistingDefinitionAsset = Definition != nullptr;
	if (bExistingDefinitionAsset && !bReplaceExistingTileEffectDefinition)
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Tile effect definition asset already exists for card '%s': %s. Enable bReplaceExistingTileEffectDefinition to reuse it."),
			*GetNameSafe(this),
			*TileEffectDefinitionObjectPath);
		return;
	}

	if (!Definition)
	{
		if (UObject* ExistingObject = StaticLoadObject(UObject::StaticClass(), nullptr, *TileEffectDefinitionObjectPath))
		{
			UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Cannot create tile effect definition for card '%s': target object exists but is not a UJargonTileEffectDefinition: %s"),
				*GetNameSafe(this),
				*GetPathNameSafe(ExistingObject));
			return;
		}

		UPackage* Package = CreatePackage(*TileEffectDefinitionPackageName);
		if (!Package)
		{
			UE_LOG(LogCardDefinitionTileEffectAuthoring, Error, TEXT("Failed to create package for tile effect definition '%s'."), *TileEffectDefinitionPackageName);
			return;
		}

		Definition = NewObject<UJargonTileEffectDefinition>(
			Package,
			UJargonTileEffectDefinition::StaticClass(),
			*TileEffectDefinitionAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Definition)
		{
			UE_LOG(LogCardDefinitionTileEffectAuthoring, Error, TEXT("Failed to create tile effect definition asset '%s'."), *TileEffectDefinitionObjectPath);
			return;
		}

		Definition->Modify();
		Definition->DisplayName = DisplayName.IsEmpty()
			? FText::FromString(DeriveDisplayNameFromAssetName(GetName()))
			: DisplayName;
		if (!Description.IsEmpty())
		{
			Definition->Description = Description;
		}
		Definition->TileEffectCategory = Category;
		Definition->Trigger = Category == ECardCategory::Aura
			? EJargonTileEffectTrigger::OnPlayerTurnStart
			: EJargonTileEffectTrigger::OnUnitEnter;
		Definition->bDestroyAfterUnitEnter = Category == ECardCategory::Trap;

		FAssetRegistryModule::AssetCreated(Definition);
		Definition->MarkPackageDirty();
		Package->MarkPackageDirty();

		UE_LOG(LogCardDefinitionTileEffectAuthoring, Display, TEXT("Created tile effect definition '%s' for card '%s'."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Display, TEXT("Reusing existing tile effect definition '%s' for card '%s'. Existing definition fields were not overwritten."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this));
	}

	if (bAssignGeneratedTileEffectDefinition)
	{
		Modify();
		if (CardScript)
		{
			CardScript->Modify();
		}
		SelectedAction->Modify();
		SelectedAction->TileEffectDefinition = Definition;
		SelectedAction->RefreshEditorTitle();
		MarkPackageDirty();
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Display, TEXT("Assigned tile effect definition '%s' to card '%s' selected place-tile-effect keyword. RuntimeTileEffectClass remains authored separately on the keyword."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogCardDefinitionTileEffectAuthoring, Display, TEXT("Created/reused tile effect definition '%s' but did not assign it because bAssignGeneratedTileEffectDefinition is false."),
			*GetPathNameSafe(Definition));
	}
#else
	UE_LOG(LogCardDefinitionTileEffectAuthoring, Warning, TEXT("Tile effect definition generation is editor-only."));
#endif
}


bool UCardDefinition::HasEffectOperation(EJargonEffectOperation Operation) const
{
	return CardScript && CardScript->HasRuntimeOperation(Operation);
}

bool UCardDefinition::UsesCardScript() const
{
	return CardScript && CardScript->HasAnyActions();
}

bool UCardDefinition::BuildBaseEffectSpecs(TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Reset();
	if (!CardScript)
	{
		return false;
	}

	CardScript->BuildBaseEffectSpecs(this, OutEffects);
	return OutEffects.Num() > 0;
}

bool UCardDefinition::BuildElementalBonusEffectSpecs(int32 BonusIndex, TArray<FJargonEffectSpec>& OutEffects) const
{
	OutEffects.Reset();
	return CardScript ? CardScript->BuildElementalBonusEffectSpecs(this, BonusIndex, OutEffects) : false;
}

int32 UCardDefinition::GetElementalBonusScriptCount() const
{
	return CardScript ? CardScript->ElementalBonuses.Num() : 0;
}

bool UCardDefinition::IsValidDefinition() const
{
	if (DisplayName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: DisplayName is empty."), *GetNameSafe(this));
		return false;
	}

	if (Cost < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: Cost is negative."), *GetNameSafe(this));
		return false;
	}

	if (Range < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: Range is negative."), *GetNameSafe(this));
		return false;
	}

	if (!UsesCardScript())
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: no CardScript actions authored."), *GetNameSafe(this));
		return false;
	}

	if (!CardElementMatchesExplicitElements(this))
	{
		TArray<EJargonElementType> ExplicitElements;
		GatherExplicitCardScriptElements(this, ExplicitElements);
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: CardElement is %s but CardScript explicitly uses element(s): %s."),
			*GetNameSafe(this),
			*GetElementTypeDisplayName(CardElement),
			*JoinElementNames(ExplicitElements));
		return false;
	}

	TArray<FJargonEffectSpec> BaseEffects;
	BuildBaseEffectSpecs(BaseEffects);
	for (int32 EffectIndex = 0; EffectIndex < BaseEffects.Num(); ++EffectIndex)
	{
		const FString EffectLabel = FString::Printf(TEXT("effect line %d"), EffectIndex);
		if (!ValidateJargonEffectSpecRuntime(this, BaseEffects[EffectIndex], EffectLabel))
		{
			return false;
		}
	}

	for (int32 BonusIndex = 0; BonusIndex < CardScript->ElementalBonuses.Num(); ++BonusIndex)
	{
		const FJargonCardElementalBonusScript& BonusGroup = CardScript->ElementalBonuses[BonusIndex];
		if (BonusGroup.ElementType == EJargonElementType::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: elemental bonus group %d requires ElementType other than None."),
				*GetNameSafe(this),
				BonusIndex);
			return false;
		}

		if (BonusGroup.RequiredCharges <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: elemental bonus group %d requires RequiredCharges > 0."),
				*GetNameSafe(this),
				BonusIndex);
			return false;
		}

		if (!BonusGroup.HasAnyActions())
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: elemental bonus group %d has no effect lines."),
				*GetNameSafe(this),
				BonusIndex);
			return false;
		}

		TArray<FJargonEffectSpec> BonusEffects;
		BuildElementalBonusEffectSpecs(BonusIndex, BonusEffects);
		for (int32 BonusEffectIndex = 0; BonusEffectIndex < BonusEffects.Num(); ++BonusEffectIndex)
		{
			const FString EffectLabel = FString::Printf(TEXT("elemental bonus group %d effect line %d"), BonusIndex, BonusEffectIndex);
			if (!ValidateJargonEffectSpecRuntime(this, BonusEffects[BonusEffectIndex], EffectLabel))
			{
				return false;
			}
		}
	}

	return true;
}

#if WITH_EDITOR
EDataValidationResult UCardDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (DisplayName.IsEmpty())
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("DisplayName is empty."));
	}

	if (Cost < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("Cost is negative: %d."), Cost));
	}

	if (Range < 0)
	{
		JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("Range is negative: %d."), Range));
	}

	if (!CardScript)
	{
		JargonDataAssetValidation::AddError(Context, this, TEXT("CardScript is required. Cards resolve through CardScript effect lines only."));
	}
	else
	{
		CardScript->ValidateScript(this, Context);
	}

	if (!CardElementMatchesExplicitElements(this))
	{
		TArray<EJargonElementType> ExplicitElements;
		GatherExplicitCardScriptElements(this, ExplicitElements);
		JargonDataAssetValidation::AddError(
			Context,
			this,
			FString::Printf(
				TEXT("CardElement is %s but CardScript explicitly uses element(s): %s."),
				*GetElementTypeDisplayName(CardElement),
				*JoinElementNames(ExplicitElements)));
	}

	if (!IsDebugCardAsset(this) && NeutralCardHasElementalIdentitySignals(this))
	{
		JargonDataAssetValidation::AddWarning(
			Context,
			this,
			TEXT("CardElement is Neutral, but status/summon/tile payloads suggest an elemental identity. Verify Neutral is intentional."));
	}

	TArray<FJargonEffectSpec> BaseEffects;
	BuildBaseEffectSpecs(BaseEffects);
	for (int32 EffectIndex = 0; EffectIndex < BaseEffects.Num(); ++EffectIndex)
	{
		JargonDataAssetValidation::ValidateJargonEffectSpec(
			this,
			BaseEffects[EffectIndex],
			FString::Printf(TEXT("CardScript effect line %d"), EffectIndex),
			Context);
	}

	if (CardScript)
	{
		for (int32 BonusIndex = 0; BonusIndex < CardScript->ElementalBonuses.Num(); ++BonusIndex)
		{
			TArray<FJargonEffectSpec> BonusEffects;
			BuildElementalBonusEffectSpecs(BonusIndex, BonusEffects);
			if (CardScript->ElementalBonuses[BonusIndex].HasAnyActions() && BonusEffects.Num() <= 0)
			{
				JargonDataAssetValidation::AddError(Context, this, FString::Printf(TEXT("CardScript elemental bonus %d has no generated effects."), BonusIndex));
			}

			for (int32 BonusEffectIndex = 0; BonusEffectIndex < BonusEffects.Num(); ++BonusEffectIndex)
			{
				JargonDataAssetValidation::ValidateJargonEffectSpec(
					this,
					BonusEffects[BonusEffectIndex],
					FString::Printf(TEXT("CardScript elemental bonus %d effect line %d"), BonusIndex, BonusEffectIndex),
					Context);
			}
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

FString UCardDefinition::GetEffectAuditSummary(const FJargonEffectSpec& EffectSpec) const
{
	return BuildCardEffectAuditSummary(EffectSpec);
}

FString UCardDefinition::GetAuditSummary() const
{
	const FString NameText = DisplayName.IsEmpty() ? GetNameSafe(this) : DisplayName.ToString();
	return FString::Printf(
		TEXT("%s | Element=%s Cost=%d Range=%d Category=%s Target=%s CardScript=%s"),
		*NameText,
		*GetElementTypeDisplayName(CardElement),
		Cost,
		Range,
		*GetCardCategoryDisplayName(Category),
		*GetCardTargetTypeDisplayName(TargetType),
		CardScript ? *CardScript->GetScriptSummary() : TEXT("None"));
}

EJargonEffectOperation UCardDefinition::GetPrimaryEffectOperation() const
{
	TArray<FJargonEffectSpec> Effects;
	BuildBaseEffectSpecs(Effects);
	return Effects.Num() > 0 ? Effects[0].Operation : EJargonEffectOperation::None;
}

FText UCardDefinition::GetCardElementDisplayText() const
{
	return FText::FromString(GetElementTypeDisplayName(CardElement));
}
