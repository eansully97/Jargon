// Fill out your copyright notice in the Description page of Project Settings.


#include "CardDefinition.h"

#include "Combat/Units/BattleUnit.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCardDefinitionPrompt, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogCardDefinitionSummonAuthoring, Log, All);

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

FString GetCardEffectOperationDisplayName(ECardEffectOperation Operation)
{
	return GetEnumDisplayName(StaticEnum<ECardEffectOperation>(), static_cast<int64>(Operation));
}

FString GetElementTypeDisplayName(EJargonElementType ElementType)
{
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

bool TrySelectSummonEffectForDefinitionGeneration(
	const UCardDefinition* Card,
	bool bReplaceExistingDefinition,
	int32& OutEffectIndex,
	FString& OutFailureReason)
{
	OutEffectIndex = INDEX_NONE;
	OutFailureReason.Reset();

	if (!Card)
	{
		OutFailureReason = TEXT("No card definition was provided.");
		return false;
	}

	TArray<int32> SummonEffectIndices;
	TArray<int32> UnassignedSummonEffectIndices;
	for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
	{
		const FCardEffectSpec& EffectSpec = Card->Effects[EffectIndex];
		if (EffectSpec.Operation != ECardEffectOperation::SummonUnit)
		{
			continue;
		}

		SummonEffectIndices.Add(EffectIndex);
		if (!EffectSpec.SummonedUnitDefinition)
		{
			UnassignedSummonEffectIndices.Add(EffectIndex);
		}
	}

	if (SummonEffectIndices.Num() <= 0)
	{
		OutFailureReason = TEXT("card has no SummonUnit effects.");
		return false;
	}

	if (SummonEffectIndices.Num() == 1)
	{
		OutEffectIndex = SummonEffectIndices[0];
		return true;
	}

	if (UnassignedSummonEffectIndices.Num() == 1)
	{
		OutEffectIndex = UnassignedSummonEffectIndices[0];
		return true;
	}

	if (UnassignedSummonEffectIndices.Num() > 1)
	{
		OutFailureReason = TEXT("card has multiple unassigned SummonUnit effects; assign one manually or generate definitions one at a time.");
		return false;
	}

	if (bReplaceExistingDefinition)
	{
		OutFailureReason = TEXT("card has multiple SummonUnit effects that already have definitions; Phase 3 does not guess which existing reference to replace.");
	}
	else
	{
		OutFailureReason = TEXT("card has multiple SummonUnit effects and all already have definitions.");
	}
	return false;
}

FString BuildObjectPathFromPackageAndAssetName(const FString& PackageName, const FString& AssetName)
{
	return FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);
}

bool CardOperationUsesValue(ECardEffectOperation Operation)
{
	switch (Operation)
	{
	case ECardEffectOperation::DealDamage:
	case ECardEffectOperation::Heal:
	case ECardEffectOperation::ApplyShield:
	case ECardEffectOperation::ApplyStun:
	case ECardEffectOperation::ApplyFreeze:
	case ECardEffectOperation::PlaceTileEffect:
	case ECardEffectOperation::DrawCards:
	case ECardEffectOperation::GainEnergy:
	case ECardEffectOperation::GainElementCharge:
	case ECardEffectOperation::ChainDamage:
	case ECardEffectOperation::ChainHeal:
	case ECardEffectOperation::ChainStun:
		return true;

	default:
		return false;
	}
}

bool IsChainCardOperation(ECardEffectOperation Operation)
{
	return Operation == ECardEffectOperation::ChainDamage
		|| Operation == ECardEffectOperation::ChainHeal
		|| Operation == ECardEffectOperation::ChainStun;
}

bool CardHasOperation(const UCardDefinition* Card, ECardEffectOperation Operation)
{
	if (!Card)
	{
		return false;
	}

	for (const FCardEffectSpec& EffectSpec : Card->Effects)
	{
		if (EffectSpec.Operation == Operation)
		{
			return true;
		}
	}

	for (const FCardElementalBonusGroup& BonusGroup : Card->ElementalBonusGroups)
	{
		for (const FCardEffectSpec& BonusEffectSpec : BonusGroup.BonusEffects)
		{
			if (BonusEffectSpec.Operation == Operation)
			{
				return true;
			}
		}
	}

	return false;
}

FString BuildEffectListSummary(const TArray<FCardEffectSpec>& Effects)
{
	if (Effects.Num() == 0)
	{
		return TEXT("None");
	}

	TArray<FString> EffectSummaries;
	for (const FCardEffectSpec& EffectSpec : Effects)
	{
		TArray<FString> Fields;

		if (CardOperationUsesValue(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("value %d"), EffectSpec.Value));
		}

		if (EffectSpec.Operation == ECardEffectOperation::GainElementCharge)
		{
			Fields.Add(FString::Printf(TEXT("element %s"), *GetElementTypeDisplayName(EffectSpec.ElementType)));
		}

		if (EffectSpec.EffectRadius > 0)
		{
			Fields.Add(FString::Printf(TEXT("radius %d"), EffectSpec.EffectRadius));
		}

		if (IsChainCardOperation(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("chain count %d"), EffectSpec.ChainCount));
		}

		if (EffectSpec.Operation == ECardEffectOperation::MoveSelf)
		{
			Fields.Add(FString::Printf(TEXT("move distance %d"), EffectSpec.MoveDistance));
		}

		if (EffectSpec.Operation == ECardEffectOperation::PushTarget)
		{
			Fields.Add(FString::Printf(TEXT("push distance %d"), EffectSpec.PushDistance));
		}

		if (EffectSpec.Operation == ECardEffectOperation::PullTarget)
		{
			Fields.Add(FString::Printf(TEXT("pull distance %d"), EffectSpec.PullDistance));
		}

		const FString FieldSummary = Fields.Num() > 0
			? FString::Printf(TEXT(" (%s)"), *FString::Join(Fields, TEXT(", ")))
			: FString();

		EffectSummaries.Add(FString::Printf(
			TEXT("%s%s"),
			*GetCardEffectOperationDisplayName(EffectSpec.Operation),
			*FieldSummary));
	}

	return FString::Join(EffectSummaries, TEXT("; "));
}

FString BuildCardEffectAuditSummary(const FCardEffectSpec& EffectSpec)
{
	TArray<FString> Fields;

	if (CardOperationUsesValue(EffectSpec.Operation))
	{
		Fields.Add(FString::Printf(TEXT("Value=%d"), EffectSpec.Value));
	}

	if (EffectSpec.Operation == ECardEffectOperation::GainElementCharge)
	{
		Fields.Add(FString::Printf(TEXT("Element=%s"), *GetElementTypeDisplayName(EffectSpec.ElementType)));
	}

	if (EffectSpec.EffectRadius > 0)
	{
		Fields.Add(FString::Printf(TEXT("Radius=%d"), EffectSpec.EffectRadius));
	}

	if (IsChainCardOperation(EffectSpec.Operation))
	{
		Fields.Add(FString::Printf(TEXT("Chain=%d"), EffectSpec.ChainCount));
	}

	if (EffectSpec.Operation == ECardEffectOperation::MoveSelf)
	{
		Fields.Add(FString::Printf(TEXT("Move=%d"), EffectSpec.MoveDistance));
	}

	if (EffectSpec.Operation == ECardEffectOperation::PushTarget)
	{
		Fields.Add(FString::Printf(TEXT("Push=%d"), EffectSpec.PushDistance));
		Fields.Add(FString::Printf(TEXT("Collision=%d"), EffectSpec.CollisionDamage));
	}

	if (EffectSpec.Operation == ECardEffectOperation::PullTarget)
	{
		Fields.Add(FString::Printf(TEXT("Pull=%d"), EffectSpec.PullDistance));
	}

	if (EffectSpec.Operation == ECardEffectOperation::SummonUnit)
	{
		Fields.Add(FString::Printf(TEXT("Definition=%s"), *GetPathNameSafe(EffectSpec.SummonedUnitDefinition.Get())));
		Fields.Add(FString::Printf(
			TEXT("UnitClass=%s%s"),
			*GetNameSafe(EffectSpec.UnitClass.Get()),
			EffectSpec.UnitClass
				? (EffectSpec.SummonedUnitDefinition ? TEXT(" fallback") : TEXT(" legacy"))
				: TEXT("")));
		Fields.Add(FString::Printf(TEXT("AttackExhausted=%s"), EffectSpec.bSummonEntersWithAttackExhausted ? TEXT("true") : TEXT("false")));
	}

	if (EffectSpec.Operation == ECardEffectOperation::PlaceTileEffect)
	{
		Fields.Add(FString::Printf(TEXT("TileEffect=%s"), *GetNameSafe(EffectSpec.TileEffectClass.Get())));
		Fields.Add(FString::Printf(TEXT("Duration=%d"), EffectSpec.TileEffectDuration));
	}

	const FString FieldSummary = Fields.Num() > 0
		? FString::Printf(TEXT(" %s"), *FString::Join(Fields, TEXT(" ")))
		: FString();

	return FString::Printf(TEXT("%s%s"), *GetCardEffectOperationDisplayName(EffectSpec.Operation), *FieldSummary);
}

FString BuildCardElementalBonusAuditSummary(const FCardElementalBonusGroup& BonusGroup)
{
	TArray<FString> BonusEffectSummaries;
	BonusEffectSummaries.Reserve(BonusGroup.BonusEffects.Num());
	for (const FCardEffectSpec& BonusEffect : BonusGroup.BonusEffects)
	{
		BonusEffectSummaries.Add(BuildCardEffectAuditSummary(BonusEffect));
	}

	return FString::Printf(
		TEXT("Bonus: %s Required=%d Spend=%s Effects=[%s]"),
		*GetElementTypeDisplayName(BonusGroup.ElementType),
		BonusGroup.RequiredCharges,
		BonusGroup.bSpendCharges ? TEXT("true") : TEXT("false"),
		BonusEffectSummaries.Num() > 0 ? *FString::Join(BonusEffectSummaries, TEXT("; ")) : TEXT("None"));
}

FString BuildCardEffectPromptSummary(const UCardDefinition* Card)
{
	if (!Card)
	{
		return TEXT("None");
	}

	const FString BaseEffectSummary = BuildEffectListSummary(Card->Effects);
	if (Card->ElementalBonusGroups.Num() <= 0)
	{
		return BaseEffectSummary;
	}

	TArray<FString> BonusSummaries;
	for (int32 BonusIndex = 0; BonusIndex < Card->ElementalBonusGroups.Num(); ++BonusIndex)
	{
		const FCardElementalBonusGroup& BonusGroup = Card->ElementalBonusGroups[BonusIndex];
		BonusSummaries.Add(FString::Printf(
			TEXT("%s %d %s charge(s): %s"),
			BonusGroup.bSpendCharges ? TEXT("Spend") : TEXT("Check"),
			BonusGroup.RequiredCharges,
			*GetElementTypeDisplayName(BonusGroup.ElementType),
			*BuildEffectListSummary(BonusGroup.BonusEffects)));
	}

	return FString::Printf(TEXT("%s. Elemental bonuses: %s"), *BaseEffectSummary, *FString::Join(BonusSummaries, TEXT("; ")));
}

void AddElementIfMeaningful(TArray<EJargonElementType>& Elements, EJargonElementType ElementType)
{
	if (ElementType != EJargonElementType::None)
	{
		Elements.AddUnique(ElementType);
	}
}

FString DeriveCardElementIdentity(const UCardDefinition* Card)
{
	TArray<EJargonElementType> Elements;
	if (Card)
	{
		for (const FCardEffectSpec& EffectSpec : Card->Effects)
		{
			if (EffectSpec.Operation == ECardEffectOperation::GainElementCharge)
			{
				AddElementIfMeaningful(Elements, EffectSpec.ElementType);
			}
		}

		for (const FCardElementalBonusGroup& BonusGroup : Card->ElementalBonusGroups)
		{
			AddElementIfMeaningful(Elements, BonusGroup.ElementType);

			for (const FCardEffectSpec& BonusEffectSpec : BonusGroup.BonusEffects)
			{
				if (BonusEffectSpec.Operation == ECardEffectOperation::GainElementCharge)
				{
					AddElementIfMeaningful(Elements, BonusEffectSpec.ElementType);
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

	if (CardHasOperation(Card, ECardEffectOperation::Heal))
	{
		return FString::Printf(TEXT("A restorative magical moment or healer's power representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, ECardEffectOperation::ApplyShield))
	{
		return FString::Printf(TEXT("A protective magical barrier or guardian force representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, ECardEffectOperation::ApplyStun))
	{
		return FString::Printf(TEXT("A stunning magical impact or disabling strike representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, ECardEffectOperation::ApplyFreeze))
	{
		return FString::Printf(TEXT("A freezing stasis effect, frostbound spell, or suspended icy impact representing '%s'."), *CardName);
	}

	if (CardHasOperation(Card, ECardEffectOperation::MoveSelf))
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
		TEXT("Effects: %s\n")
		TEXT("Main subject: %s\n")
		TEXT("Mood: %s\n\n")
		TEXT("Art direction:\n")
		TEXT("Stylized realistic fantasy card art, inspired by premium digital card games.\n")
		TEXT("Strong central focal point.\n")
		TEXT("Clean silhouette.\n")
		TEXT("Readable composition at small card size.\n")
		TEXT("Dramatic lighting.\n")
		TEXT("Sharp details on the main subject.\n")
		TEXT("Background supports the subject but does not distract.\n")
		TEXT("Use strong value contrast so the subject stands out clearly.\n")
		TEXT("Avoid clutter.\n")
		TEXT("Avoid tiny details that would be unreadable when scaled down.\n")
		TEXT("Make the action, theme, and card identity immediately understandable.\n\n")
		TEXT("Composition:\n")
		TEXT("Vertical portrait composition.\n")
		TEXT("Main subject centered or slightly offset.\n")
		TEXT("Clear foreground, midground, and background separation.\n")
		TEXT("Leave no text, no card frame, no UI, no logos, no borders.\n")
		TEXT("The image should be usable as raw card art.\n\n")
		TEXT("Quality:\n")
		TEXT("High resolution, polished digital painting, professional fantasy illustration, clean edges, readable shapes, detailed but not noisy, cinematic lighting, visually striking.\n\n")
		TEXT("Negative prompt:\n")
		TEXT("low quality, blurry, muddy, cluttered, over-detailed, unreadable, bad anatomy, distorted face, extra limbs, messy composition, text, letters, numbers, watermark, logo, card border, UI, frame, cropped subject, random objects, dull lighting\n"),
		*CardName,
		*ElementIdentity,
		*CardType,
		*TargetType,
		*PromptDescription,
		*EffectSummary,
		*MainSubject,
		*Mood);
}

bool ValidateCardEffectSpec(
	const UCardDefinition* Card,
	const FCardEffectSpec& EffectSpec,
	const FString& EffectLabel)
{
	if (EffectSpec.Operation == ECardEffectOperation::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s has operation None."),
			*GetNameSafe(Card),
			*EffectLabel);
		return false;
	}

	switch (EffectSpec.Operation)
	{
	case ECardEffectOperation::DealDamage:
	case ECardEffectOperation::Heal:
	case ECardEffectOperation::ApplyShield:
	case ECardEffectOperation::ApplyStun:
	case ECardEffectOperation::ApplyFreeze:
	case ECardEffectOperation::DrawCards:
	case ECardEffectOperation::GainEnergy:
	case ECardEffectOperation::ChainDamage:
	case ECardEffectOperation::ChainHeal:
	case ECardEffectOperation::ChainStun:
		if (EffectSpec.Value <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires Value > 0."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	case ECardEffectOperation::GainElementCharge:
		if (EffectSpec.Value <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires Value > 0."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		if (EffectSpec.ElementType == EJargonElementType::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires ElementType other than None."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	case ECardEffectOperation::MoveSelf:
		if (EffectSpec.MoveDistance <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires MoveDistance > 0."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	case ECardEffectOperation::PushTarget:
		if (EffectSpec.PushDistance <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires PushDistance > 0."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	case ECardEffectOperation::PullTarget:
		if (EffectSpec.PullDistance <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s requires PullDistance > 0."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	case ECardEffectOperation::SummonUnit:
		if (!EffectSpec.SummonedUnitDefinition && !EffectSpec.UnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s has neither SummonedUnitDefinition nor UnitClass."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	case ECardEffectOperation::PlaceTileEffect:
		if (!EffectSpec.TileEffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: %s has no TileEffectClass."),
				*GetNameSafe(Card),
				*EffectLabel);
			return false;
		}
		break;

	default:
		break;
	}

	return true;
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

	int32 SelectedEffectIndex = INDEX_NONE;
	FString FailureReason;
	if (!TrySelectSummonEffectForDefinitionGeneration(this, bReplaceExistingSummonDefinition, SelectedEffectIndex, FailureReason))
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Could not generate summon definition for card '%s': %s"),
			*GetNameSafe(this),
			*FailureReason);
		return;
	}

	if (!Effects.IsValidIndex(SelectedEffectIndex))
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Could not generate summon definition for card '%s': selected effect index %d is invalid."),
			*GetNameSafe(this),
			SelectedEffectIndex);
		return;
	}

	FCardEffectSpec& SelectedEffect = Effects[SelectedEffectIndex];
	if (SelectedEffect.SummonedUnitDefinition && !bReplaceExistingSummonDefinition)
	{
		UE_LOG(LogCardDefinitionSummonAuthoring, Warning, TEXT("Card '%s' effect %d already has SummonedUnitDefinition '%s'. Enable bReplaceExistingSummonDefinition to replace the card reference."),
			*GetNameSafe(this),
			SelectedEffectIndex,
			*GetPathNameSafe(SelectedEffect.SummonedUnitDefinition.Get()));
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
		Definition->OptionalUnitClassOverride = SelectedEffect.UnitClass;
		Definition->bSummonEntersWithAttackExhausted = SelectedEffect.bSummonEntersWithAttackExhausted;

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
		SelectedEffect.SummonedUnitDefinition = Definition;
		MarkPackageDirty();
		UE_LOG(LogCardDefinitionSummonAuthoring, Display, TEXT("Assigned summon definition '%s' to card '%s' effect %d. Legacy UnitClass remains '%s'."),
			*GetPathNameSafe(Definition),
			*GetNameSafe(this),
			SelectedEffectIndex,
			*GetNameSafe(SelectedEffect.UnitClass.Get()));
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

bool UCardDefinition::HasEffectOperation(ECardEffectOperation Operation) const
{
	for (const FCardEffectSpec& EffectSpec : Effects)
	{
		if (EffectSpec.Operation == Operation)
		{
			return true;
		}
	}

	return false;
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

	if (Effects.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: no Effects authored."), *GetNameSafe(this));
		return false;
	}

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		const FCardEffectSpec& EffectSpec = Effects[EffectIndex];
		const FString EffectLabel = FString::Printf(TEXT("effect %d"), EffectIndex);
		if (!ValidateCardEffectSpec(this, EffectSpec, EffectLabel))
		{
			return false;
		}
	}

	for (int32 BonusIndex = 0; BonusIndex < ElementalBonusGroups.Num(); ++BonusIndex)
	{
		const FCardElementalBonusGroup& BonusGroup = ElementalBonusGroups[BonusIndex];
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

		if (BonusGroup.BonusEffects.Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: elemental bonus group %d has no BonusEffects."),
				*GetNameSafe(this),
				BonusIndex);
			return false;
		}

		for (int32 BonusEffectIndex = 0; BonusEffectIndex < BonusGroup.BonusEffects.Num(); ++BonusEffectIndex)
		{
			const FString EffectLabel = FString::Printf(TEXT("elemental bonus group %d effect %d"), BonusIndex, BonusEffectIndex);
			if (!ValidateCardEffectSpec(this, BonusGroup.BonusEffects[BonusEffectIndex], EffectLabel))
			{
				return false;
			}
		}
	}

	return true;
}

FString UCardDefinition::GetEffectAuditSummary(const FCardEffectSpec& EffectSpec) const
{
	return BuildCardEffectAuditSummary(EffectSpec);
}

FString UCardDefinition::GetElementalBonusAuditSummary(const FCardElementalBonusGroup& BonusGroup) const
{
	return BuildCardElementalBonusAuditSummary(BonusGroup);
}

FString UCardDefinition::GetAuditSummary() const
{
	TArray<FString> EffectSummaries;
	EffectSummaries.Reserve(Effects.Num());
	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		EffectSummaries.Add(FString::Printf(TEXT("Effect %d: %s"), EffectIndex, *GetEffectAuditSummary(Effects[EffectIndex])));
	}

	TArray<FString> BonusSummaries;
	BonusSummaries.Reserve(ElementalBonusGroups.Num());
	for (int32 BonusIndex = 0; BonusIndex < ElementalBonusGroups.Num(); ++BonusIndex)
	{
		BonusSummaries.Add(FString::Printf(TEXT("Bonus %d: %s"), BonusIndex, *GetElementalBonusAuditSummary(ElementalBonusGroups[BonusIndex])));
	}

	const FString NameText = DisplayName.IsEmpty() ? GetNameSafe(this) : DisplayName.ToString();
	return FString::Printf(
		TEXT("%s | Cost=%d Range=%d Category=%s Target=%s Effects=[%s] ElementalBonuses=[%s]"),
		*NameText,
		Cost,
		Range,
		*GetCardCategoryDisplayName(Category),
		*GetCardTargetTypeDisplayName(TargetType),
		EffectSummaries.Num() > 0 ? *FString::Join(EffectSummaries, TEXT("; ")) : TEXT("None"),
		BonusSummaries.Num() > 0 ? *FString::Join(BonusSummaries, TEXT("; ")) : TEXT("None"));
}

ECardEffectOperation UCardDefinition::GetPrimaryEffectOperation() const
{
	return Effects.Num() > 0 ? Effects[0].Operation : ECardEffectOperation::None;
}

int32 UCardDefinition::GetConfiguredRangeForEffect(const FCardEffectSpec& EffectSpec) const
{
	if (EffectSpec.Operation == ECardEffectOperation::MoveSelf)
	{
		return FMath::Max(0, EffectSpec.MoveDistance);
	}

	return FMath::Max(0, Range);
}

int32 UCardDefinition::GetConfiguredRadiusForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.EffectRadius);
}

int32 UCardDefinition::GetConfiguredValueForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.Value);
}

int32 UCardDefinition::GetConfiguredPushDistanceForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.PushDistance);
}

int32 UCardDefinition::GetConfiguredCollisionDamageForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.CollisionDamage);
}
