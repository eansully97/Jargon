#include "Data/CardCreationTemplateTool.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCardCreationTemplateTool, Log, All);

namespace
{
FString GetTemplateDisplayName(ECardCreationTemplateType TemplateType)
{
	const UEnum* TemplateEnum = StaticEnum<ECardCreationTemplateType>();
	return TemplateEnum
		? TemplateEnum->GetDisplayNameTextByValue(static_cast<int64>(TemplateType)).ToString()
		: TEXT("Card");
}

FString GetCardCategoryDisplayName(ECardCategory Category)
{
	const UEnum* CategoryEnum = StaticEnum<ECardCategory>();
	return CategoryEnum
		? CategoryEnum->GetDisplayNameTextByValue(static_cast<int64>(Category)).ToString()
		: FString::FromInt(static_cast<int32>(Category));
}

FString GetCardTargetTypeDisplayName(ECardTargetType TargetType)
{
	const UEnum* TargetTypeEnum = StaticEnum<ECardTargetType>();
	return TargetTypeEnum
		? TargetTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(TargetType)).ToString()
		: FString::FromInt(static_cast<int32>(TargetType));
}

FString GetCardEffectOperationDisplayName(ECardEffectOperation Operation)
{
	const UEnum* OperationEnum = StaticEnum<ECardEffectOperation>();
	return OperationEnum
		? OperationEnum->GetDisplayNameTextByValue(static_cast<int64>(Operation)).ToString()
		: FString::FromInt(static_cast<int32>(Operation));
}

FString SanitizeAssetName(FString RawAssetName)
{
	RawAssetName = RawAssetName.TrimStartAndEnd();

	if (RawAssetName.IsEmpty())
	{
		RawAssetName = TEXT("NewCard");
	}

	FString Sanitized;
	Sanitized.Reserve(RawAssetName.Len());

	for (const TCHAR Character : RawAssetName)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('_'))
		{
			Sanitized.AppendChar(Character);
		}
		else
		{
			Sanitized.AppendChar(TEXT('_'));
		}
	}

	while (Sanitized.Contains(TEXT("__")))
	{
		Sanitized.ReplaceInline(TEXT("__"), TEXT("_"), ESearchCase::CaseSensitive);
	}

	Sanitized.TrimStartAndEndInline();
	Sanitized.RemoveFromStart(TEXT("_"));
	Sanitized.RemoveFromEnd(TEXT("_"));

	if (Sanitized.IsEmpty())
	{
		Sanitized = TEXT("NewCard");
	}

	if (!Sanitized.StartsWith(TEXT("DA_Card_")))
	{
		Sanitized = TEXT("DA_Card_") + Sanitized;
	}

	return Sanitized;
}

FString NormalizeOutputFolder(FString RawOutputFolder)
{
	RawOutputFolder = RawOutputFolder.TrimStartAndEnd();
	RawOutputFolder.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);

	while (RawOutputFolder.EndsWith(TEXT("/")))
	{
		RawOutputFolder.LeftChopInline(1);
	}

	return RawOutputFolder.IsEmpty() ? TEXT("/Game/Jargon/Data/Cards") : RawOutputFolder;
}

FString ResolveSavedOrAbsoluteDirectory(FString RawDirectory, const FString& DefaultSubdirectory)
{
	RawDirectory = RawDirectory.TrimStartAndEnd();
	RawDirectory.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);

	if (RawDirectory.IsEmpty())
	{
		RawDirectory = DefaultSubdirectory;
	}

	RawDirectory.RemoveFromStart(TEXT("Saved/"));
	RawDirectory.RemoveFromStart(TEXT("/Saved/"));

	FString ResolvedDirectory = FPaths::IsRelative(RawDirectory)
		? FPaths::Combine(FPaths::ProjectSavedDir(), RawDirectory)
		: RawDirectory;

	FPaths::NormalizeDirectoryName(ResolvedDirectory);
	return FPaths::ConvertRelativePathToFull(ResolvedDirectory);
}

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

FString GetCardDisplayName(const UCardDefinition* Card)
{
	if (!Card)
	{
		return TEXT("New Card");
	}

	const FString DisplayName = Card->DisplayName.ToString().TrimStartAndEnd();
	return DisplayName.IsEmpty() ? DeriveDisplayNameFromAssetName(Card->GetName()) : DisplayName;
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

FString DeriveCardArtAssetNameFromCard(const UCardDefinition* Card)
{
	return DeriveCardArtAssetNameFromCardAssetName(Card ? Card->GetName() : TEXT("DA_Card_NewCard"));
}

FText GetTemplateDescription(ECardCreationTemplateType TemplateType)
{
	switch (TemplateType)
	{
	case ECardCreationTemplateType::DamageSpell:
		return FText::FromString(TEXT("Deal 1 damage to an enemy."));

	case ECardCreationTemplateType::AoEDamageSpell:
		return FText::FromString(TEXT("Deal 1 damage to enemies in a small area."));

	case ECardCreationTemplateType::StunSpell:
		return FText::FromString(TEXT("Stun an enemy for 1 turn."));

	case ECardCreationTemplateType::ChainSpell:
		return FText::FromString(TEXT("Deal 1 damage that chains between enemies."));

	case ECardCreationTemplateType::MovementSpell:
		return FText::FromString(TEXT("Move to a target tile."));

	case ECardCreationTemplateType::SummonCard:
		return FText::FromString(TEXT("Summon a unit onto an empty tile."));

	case ECardCreationTemplateType::TrapCard:
		return FText::FromString(TEXT("Place a trap on a target tile."));

	case ECardCreationTemplateType::AuraCard:
		return FText::FromString(TEXT("Place an aura on a target tile."));

	case ECardCreationTemplateType::UtilityCard:
		return FText::FromString(TEXT("Draw 1 card. Gain 1 energy."));

	default:
		return FText::FromString(TEXT("New card."));
	}
}

FCardEffectSpec MakeEffect(ECardEffectOperation Operation)
{
	FCardEffectSpec EffectSpec;
	EffectSpec.Operation = Operation;
	return EffectSpec;
}

void ApplyTemplateToCard(UCardDefinition* Card, ECardCreationTemplateType TemplateType)
{
	if (!Card)
	{
		return;
	}

	Card->Effects.Reset();

	switch (TemplateType)
	{
	case ECardCreationTemplateType::DamageSpell:
		Card->Category = ECardCategory::Spell;
		Card->TargetType = ECardTargetType::Unit;
		Card->Cost = 1;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::DealDamage);
			EffectSpec.Value = 1;
			EffectSpec.EffectRadius = 0;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::AoEDamageSpell:
		Card->Category = ECardCategory::Spell;
		Card->TargetType = ECardTargetType::Tile;
		Card->Cost = 2;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::DealDamage);
			EffectSpec.Value = 1;
			EffectSpec.EffectRadius = 1;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::StunSpell:
		Card->Category = ECardCategory::Spell;
		Card->TargetType = ECardTargetType::Unit;
		Card->Cost = 2;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::ApplyStun);
			EffectSpec.Value = 1;
			EffectSpec.EffectRadius = 0;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::ChainSpell:
		Card->Category = ECardCategory::Spell;
		Card->TargetType = ECardTargetType::Unit;
		Card->Cost = 2;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::ChainDamage);
			EffectSpec.Value = 1;
			EffectSpec.EffectRadius = 2;
			EffectSpec.ChainCount = 3;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::MovementSpell:
		Card->Category = ECardCategory::Spell;
		Card->TargetType = ECardTargetType::Tile;
		Card->Cost = 1;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::MoveSelf);
			EffectSpec.MoveDistance = 3;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::SummonCard:
		Card->Category = ECardCategory::Summon;
		Card->TargetType = ECardTargetType::Tile;
		Card->Cost = 2;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::SummonUnit);
			EffectSpec.bSummonEntersWithAttackExhausted = true;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::TrapCard:
		Card->Category = ECardCategory::Trap;
		Card->TargetType = ECardTargetType::Tile;
		Card->Cost = 1;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::PlaceTileEffect);
			EffectSpec.Value = 1;
			EffectSpec.EffectRadius = 0;
			EffectSpec.TileEffectDuration = 0;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::AuraCard:
		Card->Category = ECardCategory::Aura;
		Card->TargetType = ECardTargetType::Tile;
		Card->Cost = 2;
		Card->Range = 3;
		{
			FCardEffectSpec EffectSpec = MakeEffect(ECardEffectOperation::PlaceTileEffect);
			EffectSpec.Value = 1;
			EffectSpec.EffectRadius = 1;
			EffectSpec.TileEffectDuration = 0;
			Card->Effects.Add(EffectSpec);
		}
		break;

	case ECardCreationTemplateType::UtilityCard:
		Card->Category = ECardCategory::Spell;
		Card->TargetType = ECardTargetType::Self;
		Card->Cost = 0;
		Card->Range = 0;
		{
			FCardEffectSpec DrawCardsEffect = MakeEffect(ECardEffectOperation::DrawCards);
			DrawCardsEffect.Value = 1;
			Card->Effects.Add(DrawCardsEffect);

			FCardEffectSpec GainEnergyEffect = MakeEffect(ECardEffectOperation::GainEnergy);
			GainEnergyEffect.Value = 1;
			Card->Effects.Add(GainEnergyEffect);
		}
		break;

	default:
		break;
	}
}

bool OperationUsesValue(ECardEffectOperation Operation)
{
	switch (Operation)
	{
	case ECardEffectOperation::DealDamage:
	case ECardEffectOperation::Heal:
	case ECardEffectOperation::ApplyShield:
	case ECardEffectOperation::ApplyStun:
	case ECardEffectOperation::PlaceTileEffect:
	case ECardEffectOperation::DrawCards:
	case ECardEffectOperation::GainEnergy:
	case ECardEffectOperation::ChainDamage:
	case ECardEffectOperation::ChainHeal:
	case ECardEffectOperation::ChainStun:
		return true;

	default:
		return false;
	}
}

bool IsChainOperation(ECardEffectOperation Operation)
{
	return Operation == ECardEffectOperation::ChainDamage
		|| Operation == ECardEffectOperation::ChainHeal
		|| Operation == ECardEffectOperation::ChainStun;
}

FString BuildCardEffectSummary(const UCardDefinition* Card)
{
	if (!Card || Card->Effects.Num() == 0)
	{
		return TEXT("None");
	}

	TArray<FString> EffectSummaries;
	for (const FCardEffectSpec& EffectSpec : Card->Effects)
	{
		TArray<FString> Fields;

		if (OperationUsesValue(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("Value %d"), EffectSpec.Value));
		}

		if (EffectSpec.EffectRadius > 0)
		{
			Fields.Add(FString::Printf(TEXT("Radius %d"), EffectSpec.EffectRadius));
		}

		if (IsChainOperation(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("Chain count %d"), EffectSpec.ChainCount));
		}

		if (EffectSpec.Operation == ECardEffectOperation::MoveSelf)
		{
			Fields.Add(FString::Printf(TEXT("Move distance %d"), EffectSpec.MoveDistance));
		}

		if (EffectSpec.Operation == ECardEffectOperation::PushTarget)
		{
			Fields.Add(FString::Printf(TEXT("Push distance %d"), EffectSpec.PushDistance));
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

bool HasIntentionalMissingReference(const UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	for (const FCardEffectSpec& EffectSpec : Card->Effects)
	{
		if (EffectSpec.Operation == ECardEffectOperation::SummonUnit && !EffectSpec.UnitClass)
		{
			return true;
		}

		if (EffectSpec.Operation == ECardEffectOperation::PlaceTileEffect && !EffectSpec.TileEffectClass)
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
bool DoesAssetAlreadyExist(const FString& PackageName, const FString& AssetName)
{
	const FString ObjectPath = PackageName + TEXT(".") + AssetName;

	if (FindObject<UObject>(nullptr, *ObjectPath))
	{
		return true;
	}

	FString ExistingPackageFilename;
	if (FPackageName::DoesPackageExist(PackageName, &ExistingPackageFilename))
	{
		return true;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	const FAssetData ExistingAsset =
		AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPath));

	return ExistingAsset.IsValid();
}
#endif
}

UCardCreationTemplateTool::UCardCreationTemplateTool()
{
	bCreatePrompt = true;
	bCopyPromptToClipboard = true;
	bSyncContentBrowserToCreatedAsset = true;
	bOpenCreatedAssetEditor = true;

	CardArtPromptStyle = TEXT("Stylized fantasy digital trading card artwork, strong central focal point, readable at small size, dramatic lighting, clean silhouette, strong contrast, polished illustration, premium card game style.");
	MoodOverride = TEXT("Dramatic, magical, energetic.");
	NegativePromptNotes = TEXT("No text, no card frame, no UI, no logo, no border, no watermark.");
	PromptOutputSubdirectory = TEXT("CardArtPrompts");
	ArtWidth = 720;
	ArtHeight = 1280;
}

void UCardCreationTemplateTool::CreateCard()
{
#if WITH_EDITOR
	LastCreatedCard = nullptr;
	LastGeneratedPromptFilePath.Reset();

	UE_LOG(LogCardCreationTemplateTool, Display, TEXT("=== Create Card Started ==="));

	FString CreatedCardObjectPath;
	UCardDefinition* NewCard = CreateCardAssetFromTemplate(CreatedCardObjectPath);
	if (!NewCard)
	{
		UE_LOG(LogCardCreationTemplateTool, Error, TEXT("Create Card failed before an asset was created."));
		return;
	}

	LastCreatedCard = NewCard;

	if (bCreatePrompt)
	{
		GenerateArtPromptForCard(NewCard);
	}
	else
	{
		UE_LOG(LogCardCreationTemplateTool, Display, TEXT("Prompt generation skipped."));
	}

	LogCreatedCardValidation(NewCard, CreatedCardObjectPath);
	SyncEditorToCreatedAsset(NewCard);

	if (bOpenCreatedAssetEditor && GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(NewCard);
		}
	}

	UE_LOG(LogCardCreationTemplateTool, Display, TEXT("=== Create Card Finished ==="));
#else
	UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Card Creation Template Tool is editor-only and cannot create assets in this build."));
#endif
}

void UCardCreationTemplateTool::CreateCardFromTemplate()
{
	CreateCard();
}

FString UCardCreationTemplateTool::BuildCardArtPrompt(UCardDefinition* Card) const
{
	if (!Card)
	{
		return FString();
	}

	const FString CardName = GetCardDisplayName(Card);
	const FString Category = GetCardCategoryDisplayName(Card->Category);
	const FString TargetType = GetCardTargetTypeDisplayName(Card->TargetType);
	const FString Description = Card->Description.ToString().TrimStartAndEnd();
	const FString EffectSummary = BuildCardEffectSummary(Card);

	const FString Subject = SubjectOverride.TrimStartAndEnd().IsEmpty()
		? CardName
		: SubjectOverride.TrimStartAndEnd();

	const FString Mood = MoodOverride.TrimStartAndEnd().IsEmpty()
		? TEXT("Dramatic, readable, game-ready.")
		: MoodOverride.TrimStartAndEnd();

	const FString Style = CardArtPromptStyle.TrimStartAndEnd().IsEmpty()
		? TEXT("Stylized fantasy digital trading card artwork, strong central focal point, readable at small size, dramatic lighting, clean silhouette, strong contrast, polished illustration, premium card game style.")
		: CardArtPromptStyle.TrimStartAndEnd();

	const FString Negative = NegativePromptNotes.TrimStartAndEnd().IsEmpty()
		? TEXT("No text, no card frame, no UI, no logo, no border, no watermark.")
		: NegativePromptNotes.TrimStartAndEnd();

	return FString::Printf(
		TEXT("Create high-quality fantasy trading card artwork.\n\n")
		TEXT("Canvas:\n")
		TEXT("Vertical image, %dx%d, 9:16 aspect ratio.\n\n")
		TEXT("Card:\n")
		TEXT("Name: %s\n")
		TEXT("Category: %s\n")
		TEXT("Target type: %s\n")
		TEXT("Description: %s\n")
		TEXT("Effects: %s\n")
		TEXT("Subject: %s\n")
		TEXT("Mood: %s\n\n")
		TEXT("Art direction:\n")
		TEXT("%s\n")
		TEXT("Strong central focal point. Clean silhouette. Readable composition at small card size. Dramatic lighting. Strong value contrast. Avoid clutter.\n\n")
		TEXT("Do not include:\n")
		TEXT("%s\n\n")
		TEXT("Extra notes:\n")
		TEXT("%s\n"),
		FMath::Max(1, ArtWidth),
		FMath::Max(1, ArtHeight),
		*CardName,
		*Category,
		*TargetType,
		Description.IsEmpty() ? TEXT("None") : *Description,
		*EffectSummary,
		*Subject,
		*Mood,
		*Style,
		*Negative,
		*ExtraPromptNotes.TrimStartAndEnd());
}

void UCardCreationTemplateTool::GeneratePromptForConfiguredCard()
{
#if WITH_EDITOR
	UCardDefinition* TargetCard = CardToGeneratePromptFor.Get();
	if (!TargetCard)
	{
		UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Cannot generate prompt. CardToGeneratePromptFor is not configured."));
		return;
	}

	GenerateArtPromptForCard(TargetCard);
#else
	UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Card art prompt generation is editor-only."));
#endif
}

#if WITH_EDITOR
UCardDefinition* UCardCreationTemplateTool::CreateCardAssetFromTemplate(FString& OutObjectPath)
{
	OutObjectPath.Reset();

	const FString AssetName = SanitizeAssetName(NewCardAssetName);
	const FString PackageFolder = NormalizeOutputFolder(OutputFolder);
	const FString PackageName = PackageFolder / AssetName;
	OutObjectPath = PackageName + TEXT(".") + AssetName;

	FText PackageValidationError;
	if (!FPackageName::IsValidLongPackageName(PackageName, false, &PackageValidationError))
	{
		UE_LOG(LogCardCreationTemplateTool, Error, TEXT("Cannot create card. Invalid package path '%s': %s"),
			*PackageName,
			*PackageValidationError.ToString());
		return nullptr;
	}

	if (DoesAssetAlreadyExist(PackageName, AssetName))
	{
		UE_LOG(LogCardCreationTemplateTool, Error, TEXT("Cannot create card. Asset already exists: %s"), *OutObjectPath);
		return nullptr;
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogCardCreationTemplateTool, Error, TEXT("Cannot create card. Failed to create package: %s"), *PackageName);
		return nullptr;
	}

	UCardDefinition* NewCard = NewObject<UCardDefinition>(
		Package,
		UCardDefinition::StaticClass(),
		*AssetName,
		RF_Public | RF_Standalone | RF_Transactional);

	if (!NewCard)
	{
		UE_LOG(LogCardCreationTemplateTool, Error, TEXT("Cannot create card. Failed to allocate UCardDefinition: %s"), *OutObjectPath);
		return nullptr;
	}

	ApplyTemplateToCard(NewCard, TemplateType);

	if (bOverrideCost)
	{
		NewCard->Cost = FMath::Max(0, CostOverride);
	}

	if (bOverrideRange)
	{
		NewCard->Range = FMath::Max(0, RangeOverride);
	}

	const FString DisplayName = DisplayNameOverride.IsEmpty()
		? DeriveDisplayNameFromAssetName(AssetName)
		: DisplayNameOverride.ToString();

	NewCard->DisplayName = FText::FromString(DisplayName);
	NewCard->Description = DescriptionOverride.IsEmpty()
		? GetTemplateDescription(TemplateType)
		: DescriptionOverride;

	// Card art is intentionally left blank. The prompt is generated so art can be made and assigned manually.
	NewCard->CardArt = nullptr;

	NewCard->PostEditChange();
	NewCard->MarkPackageDirty();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewCard);

	return NewCard;
}

bool UCardCreationTemplateTool::GenerateArtPromptForCard(UCardDefinition* Card)
{
	LastGeneratedPromptFilePath.Reset();

	if (!Card)
	{
		UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Cannot generate card art prompt. No card is configured."));
		return false;
	}

	const FString Prompt = BuildCardArtPrompt(Card);
	if (Prompt.IsEmpty())
	{
		UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Cannot generate card art prompt for '%s'. Prompt was empty."), *GetNameSafe(Card));
		return false;
	}

	const FString OutputDirectory = ResolveSavedOrAbsoluteDirectory(PromptOutputSubdirectory, TEXT("CardArtPrompts"));
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);

	const FString PromptFileName = DeriveCardArtAssetNameFromCard(Card) + TEXT("_prompt.txt");
	const FString PromptFilePath = FPaths::Combine(OutputDirectory, PromptFileName);

	if (!FFileHelper::SaveStringToFile(Prompt, *PromptFilePath))
	{
		UE_LOG(LogCardCreationTemplateTool, Error, TEXT("Failed to write card art prompt: %s"), *PromptFilePath);
		return false;
	}

	LastGeneratedPromptFilePath = PromptFilePath;

	if (bCopyPromptToClipboard)
	{
		FPlatformApplicationMisc::ClipboardCopy(*Prompt);
	}

	UE_LOG(LogCardCreationTemplateTool, Display, TEXT("Card art prompt saved: %s"), *PromptFilePath);

	if (bCopyPromptToClipboard)
	{
		UE_LOG(LogCardCreationTemplateTool, Display, TEXT("Card art prompt copied to clipboard."));
	}

	return true;
}

void UCardCreationTemplateTool::LogCreatedCardValidation(UCardDefinition* Card, const FString& ObjectPath) const
{
	if (!Card)
	{
		return;
	}

	const bool bValidDefinition = Card->IsValidDefinition();

	if (bValidDefinition)
	{
		UE_LOG(LogCardCreationTemplateTool, Display, TEXT("Created %s card '%s': %s"),
			*GetTemplateDisplayName(TemplateType),
			*Card->DisplayName.ToString(),
			*ObjectPath);
	}
	else if (HasIntentionalMissingReference(Card))
	{
		UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Created %s card '%s' with manual setup remaining: assign UnitClass or TileEffectClass before using it in packs/combat. Asset: %s"),
			*GetTemplateDisplayName(TemplateType),
			*Card->DisplayName.ToString(),
			*ObjectPath);
	}
	else
	{
		UE_LOG(LogCardCreationTemplateTool, Warning, TEXT("Created %s card '%s', but CardDefinition validation reported problems. Asset: %s"),
			*GetTemplateDisplayName(TemplateType),
			*Card->DisplayName.ToString(),
			*ObjectPath);
	}
}

void UCardCreationTemplateTool::SyncEditorToCreatedAsset(UCardDefinition* Card) const
{
	if (!GEditor || !bSyncContentBrowserToCreatedAsset || !Card)
	{
		return;
	}

	TArray<UObject*> AssetsToSync;
	AssetsToSync.Add(Card);

	GEditor->SyncBrowserToObjects(AssetsToSync);
}
#endif