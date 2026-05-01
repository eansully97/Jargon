#include "Data/ProjectSetupAuditTool.h"

#include "Data/CardDefinition.h"
#include "Data/CardPackDefinition.h"
#include "Data/JargonRelicDefinition.h"
#include "Exploration/Interactables/ExplorationRewardInteractable.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogProjectSetupAudit, Log, All);

namespace UE::Jargon::ProjectSetupAuditToolPrivate
{
const FName DefaultTownMapPath(TEXT("/Game/Jargon/Maps/L_TownMap"));
const FName DefaultExplorationMapPath(TEXT("/Game/Jargon/Maps/L_Exploration"));
const FName DefaultCombatMapPath(TEXT("/Game/Jargon/Maps/L_CombatMap"));
const FName DefaultTownGameModePath(TEXT("/Game/Jargon/Blueprints/Town/BP_TownGameMode"));
const FName DefaultTownPlayerControllerPath(TEXT("/Game/Jargon/Blueprints/Town/BP_TownPlayerController"));
const FName DefaultExplorationGameModePath(TEXT("/Game/Jargon/Blueprints/Exploration/BP_JargonExplorationGameMode"));
const FName DefaultExplorationPlayerControllerPath(TEXT("/Game/Jargon/Blueprints/Exploration/BP_JargonExplorationController"));
const FName DefaultCombatGameModePath(TEXT("/Game/Jargon/Blueprints/Combat/BP_CombatGameMode"));
const FName DefaultCardScanPath(TEXT("/Game/Jargon/Data/Cards"));
const FName LegacyCardScanPath(TEXT("/Game/Jargon/Cards"));
const FName DefaultPackScanPath(TEXT("/Game/Jargon/Data/CardPacks"));
const FName LegacyPackScanPath(TEXT("/Game/Jargon/CardPacks"));
const FName DefaultRelicScanPath(TEXT("/Game/Jargon/Data/Relics"));
const FName DefaultRewardBlueprintScanPath(TEXT("/Game/Jargon/Blueprints"));
const FName TopDownTemplatePath(TEXT("/Game/TopDown"));
const FName VariantStrategyTemplatePath(TEXT("/Game/Variant_Strategy"));
const FName VariantTwinStickTemplatePath(TEXT("/Game/Variant_TwinStick"));

#if WITH_EDITOR
struct FProjectAuditRow
{
	FString Severity;
	FString Area;
	FString Asset;
	FString Status;
	FString Message;
};

FString CsvEscape(const FString& Value)
{
	FString Escaped = Value;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);

	if (Escaped.Contains(TEXT(",")) || Escaped.Contains(TEXT("\"")) || Escaped.Contains(TEXT("\n")) || Escaped.Contains(TEXT("\r")))
	{
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	return Escaped;
}

void AddRow(
	TArray<FProjectAuditRow>& Rows,
	const FString& Severity,
	const FString& Area,
	const FString& Asset,
	const FString& Status,
	const FString& Message)
{
	FProjectAuditRow& Row = Rows.AddDefaulted_GetRef();
	Row.Severity = Severity;
	Row.Area = Area;
	Row.Asset = Asset;
	Row.Status = Status;
	Row.Message = Message;
}

FString BuildGeneratedClassPath(FName BlueprintPackagePath)
{
	const FString PackagePathString = BlueprintPackagePath.ToString();
	const FString AssetName = FPackageName::GetShortName(PackagePathString);
	return FString::Printf(TEXT("%s.%s_C"), *PackagePathString, *AssetName);
}

UClass* LoadBlueprintGeneratedClass(FName BlueprintPackagePath)
{
	if (BlueprintPackagePath.IsNone())
	{
		return nullptr;
	}

	return LoadClass<UObject>(nullptr, *BuildGeneratedClassPath(BlueprintPackagePath));
}

bool DoesPackageHaveAsset(IAssetRegistry& AssetRegistry, FName PackagePath)
{
	if (PackagePath.IsNone())
	{
		return false;
	}

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByPackageName(PackagePath, Assets);
	return Assets.Num() > 0;
}

TArray<FName> GetPackageDependencies(IAssetRegistry& AssetRegistry, FName PackagePath)
{
	TArray<FName> Dependencies;
	if (!PackagePath.IsNone())
	{
		AssetRegistry.GetDependencies(PackagePath, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
	}
	return Dependencies;
}

bool DependenciesContainPackage(const TArray<FName>& Dependencies, FName ExpectedPackage)
{
	for (const FName& Dependency : Dependencies)
	{
		if (Dependency == ExpectedPackage)
		{
			return true;
		}
	}
	return false;
}

void AuditPackageRootPresence(
	TArray<FProjectAuditRow>& Rows,
	IAssetRegistry& AssetRegistry,
	FName PackageRoot,
	const FString& Area,
	const FString& MessageIfPresent)
{
	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByPath(PackageRoot, Assets, true);
	AddRow(
		Rows,
		Assets.Num() > 0 ? TEXT("Warning") : TEXT("Info"),
		Area,
		PackageRoot.ToString(),
		Assets.Num() > 0 ? TEXT("Present") : TEXT("Not Found"),
		Assets.Num() > 0
			? FString::Printf(TEXT("%s Assets found recursively: %d."), *MessageIfPresent, Assets.Num())
			: TEXT("No assets found under this package root."));
}

bool NameArrayContains(const TArray<FName>& Values, FName ExpectedValue)
{
	for (const FName& Value : Values)
	{
		if (Value == ExpectedValue)
		{
			return true;
		}
	}

	return false;
}

void AuditConfiguredLegacyPath(
	TArray<FProjectAuditRow>& Rows,
	const TArray<FName>& ConfiguredPaths,
	FName LegacyPath,
	const FString& Area,
	const FString& Message)
{
	const bool bConfigured = NameArrayContains(ConfiguredPaths, LegacyPath);
	AddRow(
		Rows,
		bConfigured ? TEXT("Warning") : TEXT("Info"),
		Area,
		LegacyPath.ToString(),
		bConfigured ? TEXT("Configured") : TEXT("Not Configured"),
		bConfigured ? Message : TEXT("Legacy path is not configured on this audit tool."));
}

void AuditDependenciesWithPrefix(
	TArray<FProjectAuditRow>& Rows,
	IAssetRegistry& AssetRegistry,
	FName SourcePackage,
	FName DependencyPrefix,
	const FString& Area)
{
	const FString Prefix = DependencyPrefix.ToString();
	TArray<FString> MatchingDependencies;
	for (const FName& Dependency : GetPackageDependencies(AssetRegistry, SourcePackage))
	{
		const FString DependencyString = Dependency.ToString();
		if (DependencyString.StartsWith(Prefix))
		{
			MatchingDependencies.Add(DependencyString);
		}
	}

	if (MatchingDependencies.Num() > 0)
	{
		AddRow(
			Rows,
			TEXT("Warning"),
			Area,
			SourcePackage.ToString(),
			TEXT("References Legacy/Template Root"),
			FString::Printf(TEXT("References %s dependencies: %s"), *Prefix, *FString::Join(MatchingDependencies, TEXT("; "))));
	}
}

void AuditSourceFolderPresence(TArray<FProjectAuditRow>& Rows, const FString& RelativeFolder)
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), RelativeFolder);
	const bool bExists = IFileManager::Get().DirectoryExists(*FullPath);
	AddRow(
		Rows,
		bExists ? TEXT("Warning") : TEXT("Info"),
		TEXT("Template Source"),
		RelativeFolder,
		bExists ? TEXT("Present") : TEXT("Not Found"),
		bExists
			? TEXT("Template source folder is still present. Report only; do not remove until include/reference checks are complete.")
			: TEXT("Template source folder was not found."));
}

void AddFilesByPattern(TArray<FString>& OutFiles, const FString& Directory, const FString& Pattern)
{
	if (IFileManager::Get().DirectoryExists(*Directory))
	{
		IFileManager::Get().FindFilesRecursive(OutFiles, *Directory, *Pattern, true, false);
	}
}

bool ShouldSkipTextReferenceFile(const FString& FilePath)
{
	return FilePath.EndsWith(TEXT("ProjectSetupAuditTool.cpp"))
		|| FilePath.EndsWith(TEXT("ProjectSetupAuditTool.h"));
}

void AuditTextReferences(TArray<FProjectAuditRow>& Rows, const TArray<FString>& Tokens)
{
	TArray<FString> FilesToScan;
	AddFilesByPattern(FilesToScan, FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")), TEXT("*.h"));
	AddFilesByPattern(FilesToScan, FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")), TEXT("*.cpp"));
	AddFilesByPattern(FilesToScan, FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")), TEXT("*.cs"));
	AddFilesByPattern(FilesToScan, FPaths::Combine(FPaths::ProjectDir(), TEXT("Config")), TEXT("*.ini"));

	const FString ProjectFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Jargon.uproject"));
	if (FPaths::FileExists(ProjectFile))
	{
		FilesToScan.Add(ProjectFile);
	}

	for (const FString& Token : Tokens)
	{
		TArray<FString> MatchingFiles;
		for (const FString& FilePath : FilesToScan)
		{
			if (ShouldSkipTextReferenceFile(FilePath))
			{
				continue;
			}

			FString Contents;
			if (FFileHelper::LoadFileToString(Contents, *FilePath) && Contents.Contains(Token))
			{
				MatchingFiles.Add(FPaths::ConvertRelativePathToFull(FilePath));
			}
		}

		AddRow(
			Rows,
			MatchingFiles.Num() > 0 ? TEXT("Warning") : TEXT("Info"),
			TEXT("Text References"),
			Token,
			MatchingFiles.Num() > 0 ? TEXT("Found") : TEXT("Not Found"),
			MatchingFiles.Num() > 0
				? FString::Printf(TEXT("Token appears in %d source/config/project file(s): %s"), MatchingFiles.Num(), *FString::Join(MatchingFiles, TEXT("; ")))
				: TEXT("Token was not found in scanned source/config/project files."));
	}
}

void AuditTemplateAndLegacyReferences(
	TArray<FProjectAuditRow>& Rows,
	IAssetRegistry& AssetRegistry,
	const TArray<FName>& CardScanPaths,
	const TArray<FName>& PackScanPaths,
	const TArray<FName>& RootPackagesToCheck)
{
	AuditPackageRootPresence(Rows, AssetRegistry, TopDownTemplatePath, TEXT("Template Content"), TEXT("TopDown template content root is still present."));
	AuditPackageRootPresence(Rows, AssetRegistry, VariantStrategyTemplatePath, TEXT("Template Content"), TEXT("Variant_Strategy template content root is still present."));
	AuditPackageRootPresence(Rows, AssetRegistry, VariantTwinStickTemplatePath, TEXT("Template Content"), TEXT("Variant_TwinStick template content root is still present."));
	AuditPackageRootPresence(Rows, AssetRegistry, LegacyCardScanPath, TEXT("Legacy Content"), TEXT("Legacy card content root is still present."));
	AuditPackageRootPresence(Rows, AssetRegistry, LegacyPackScanPath, TEXT("Legacy Content"), TEXT("Legacy card pack content root is still present."));

	AuditConfiguredLegacyPath(
		Rows,
		CardScanPaths,
		LegacyCardScanPath,
		TEXT("Legacy Content"),
		TEXT("Legacy card scan path is configured. Keep until all cards are migrated/verified, then remove from the audit asset."));
	AuditConfiguredLegacyPath(
		Rows,
		PackScanPaths,
		LegacyPackScanPath,
		TEXT("Legacy Content"),
		TEXT("Legacy card pack scan path is configured. Keep until all packs are migrated/verified, then remove from the audit asset."));

	for (const FName& SourcePackage : RootPackagesToCheck)
	{
		AuditDependenciesWithPrefix(Rows, AssetRegistry, SourcePackage, TopDownTemplatePath, TEXT("Template Dependencies"));
		AuditDependenciesWithPrefix(Rows, AssetRegistry, SourcePackage, VariantStrategyTemplatePath, TEXT("Template Dependencies"));
		AuditDependenciesWithPrefix(Rows, AssetRegistry, SourcePackage, VariantTwinStickTemplatePath, TEXT("Template Dependencies"));
		AuditDependenciesWithPrefix(Rows, AssetRegistry, SourcePackage, LegacyCardScanPath, TEXT("Legacy Dependencies"));
		AuditDependenciesWithPrefix(Rows, AssetRegistry, SourcePackage, LegacyPackScanPath, TEXT("Legacy Dependencies"));
	}

	AuditSourceFolderPresence(Rows, TEXT("Source/Jargon/Variant_Strategy"));
	AuditSourceFolderPresence(Rows, TEXT("Source/Jargon/Variant_TwinStick"));
	AuditTextReferences(Rows, {
		TEXT("/Game/TopDown"),
		TEXT("/Game/Variant_Strategy"),
		TEXT("/Game/Variant_TwinStick"),
		TEXT("/Game/Jargon/Cards"),
		TEXT("/Game/Jargon/CardPacks"),
		TEXT("TP_TopDown")
	});
}

void AuditAssetExists(TArray<FProjectAuditRow>& Rows, IAssetRegistry& AssetRegistry, FName PackagePath, const FString& Area)
{
	const bool bExists = DoesPackageHaveAsset(AssetRegistry, PackagePath);
	AddRow(
		Rows,
		bExists ? TEXT("Info") : TEXT("Error"),
		Area,
		PackagePath.ToString(),
		bExists ? TEXT("Found") : TEXT("Missing"),
		bExists ? TEXT("Asset exists.") : TEXT("Expected asset was not found."));
}

void AuditMapDependency(
	TArray<FProjectAuditRow>& Rows,
	IAssetRegistry& AssetRegistry,
	FName MapPath,
	FName ExpectedDependency,
	const FString& Area,
	const FString& Description)
{
	const TArray<FName> Dependencies = GetPackageDependencies(AssetRegistry, MapPath);
	const bool bFoundDependency = DependenciesContainPackage(Dependencies, ExpectedDependency);
	AddRow(
		Rows,
		bFoundDependency ? TEXT("Info") : TEXT("Warning"),
		Area,
		MapPath.ToString(),
		bFoundDependency ? TEXT("Referenced") : TEXT("Needs Review"),
		bFoundDependency
			? FString::Printf(TEXT("%s references %s."), *Description, *ExpectedDependency.ToString())
			: FString::Printf(TEXT("%s did not list %s as a package dependency. Confirm World Settings/GameMode Override in-editor."), *Description, *ExpectedDependency.ToString()));
}

void AuditTownMapExplorationContent(TArray<FProjectAuditRow>& Rows, IAssetRegistry& AssetRegistry, FName TownMapPath)
{
	const TArray<FName> Dependencies = GetPackageDependencies(AssetRegistry, TownMapPath);
	for (const FName& Dependency : Dependencies)
	{
		const FString DependencyString = Dependency.ToString();
		if (DependencyString.Contains(TEXT("ExplorationEnemy")) ||
			DependencyString.Contains(TEXT("CurrencyCache")) ||
			DependencyString.Contains(TEXT("RelicReward")))
		{
			AddRow(
				Rows,
				TEXT("Warning"),
				TEXT("Map Content"),
				TownMapPath.ToString(),
				TEXT("Needs Review"),
				FString::Printf(TEXT("Town map references exploration-style content '%s'. This may be intentional test content, but confirm it belongs in Town."), *DependencyString));
		}
	}
}

UObject* GetClassDefaultObjectFromBlueprint(FName BlueprintPackagePath)
{
	UClass* LoadedClass = LoadBlueprintGeneratedClass(BlueprintPackagePath);
	return LoadedClass ? LoadedClass->GetDefaultObject() : nullptr;
}

FString GetReflectedObjectPath(UObject* Object, FName PropertyName)
{
	if (!Object)
	{
		return TEXT("None");
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
	if (!ObjectProperty)
	{
		return TEXT("Property Missing");
	}

	UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Object);
	return Value ? Value->GetPathName() : TEXT("None");
}

bool IsReflectedObjectAssigned(UObject* Object, FName PropertyName)
{
	if (!Object)
	{
		return false;
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
	return ObjectProperty && ObjectProperty->GetObjectPropertyValue_InContainer(Object) != nullptr;
}

void AuditObjectPropertyAssigned(
	TArray<FProjectAuditRow>& Rows,
	UObject* Object,
	FName PropertyName,
	const FString& Area,
	const FString& AssetLabel,
	bool bCritical)
{
	const bool bAssigned = IsReflectedObjectAssigned(Object, PropertyName);
	const FString PropertyPath = GetReflectedObjectPath(Object, PropertyName);
	AddRow(
		Rows,
		bAssigned ? TEXT("Info") : (bCritical ? TEXT("Error") : TEXT("Warning")),
		Area,
		AssetLabel,
		bAssigned ? TEXT("Assigned") : TEXT("Missing"),
		FString::Printf(TEXT("%s = %s"), *PropertyName.ToString(), *PropertyPath));
}

bool DoesClassExposeProperty(UClass* Class, FName PropertyName)
{
	return Class && Class->FindPropertyByName(PropertyName) != nullptr;
}

void AuditClassExposesWidgetBinding(
	TArray<FProjectAuditRow>& Rows,
	UClass* WidgetClass,
	FName PropertyName,
	const FString& Area,
	const FString& AssetLabel,
	bool bRequired)
{
	const bool bExposed = DoesClassExposeProperty(WidgetClass, PropertyName);
	AddRow(
		Rows,
		bExposed ? TEXT("Info") : (bRequired ? TEXT("Warning") : TEXT("Info")),
		Area,
		AssetLabel,
		bExposed ? TEXT("Supported") : TEXT("Not Exposed"),
		bExposed
			? FString::Printf(TEXT("Widget class exposes optional binding/property '%s'."), *PropertyName.ToString())
			: FString::Printf(TEXT("Widget class does not expose '%s'. This is only a problem if the Blueprint is expected to bind that widget."), *PropertyName.ToString()));
}

UClass* GetReflectedClass(UObject* Object, FName PropertyName)
{
	if (!Object)
	{
		return nullptr;
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
	UObject* Value = ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(Object) : nullptr;
	return Cast<UClass>(Value);
}

void AuditBattleUnitStatusWidget(
	TArray<FProjectAuditRow>& Rows,
	UClass* UnitClass,
	const FString& UnitAssetLabel)
{
	if (!UnitClass)
	{
		AddRow(
			Rows,
			TEXT("Warning"),
			TEXT("Battle Unit Status"),
			UnitAssetLabel,
			TEXT("Skipped"),
			TEXT("Could not inspect StatusWidgetClass because the unit class was not loaded."));
		return;
	}

	UObject* UnitCDO = UnitClass->GetDefaultObject();
	AuditObjectPropertyAssigned(Rows, UnitCDO, TEXT("StatusWidgetClass"), TEXT("Battle Unit Status"), UnitClass->GetPathName(), false);

	UClass* StatusWidgetClass = GetReflectedClass(UnitCDO, TEXT("StatusWidgetClass"));
	const FString StatusWidgetAssetLabel = StatusWidgetClass ? StatusWidgetClass->GetPathName() : UnitClass->GetPathName();
	AuditClassExposesWidgetBinding(Rows, StatusWidgetClass, TEXT("HPText"), TEXT("Battle Unit Status"), StatusWidgetAssetLabel, true);
	AuditClassExposesWidgetBinding(Rows, StatusWidgetClass, TEXT("AttackText"), TEXT("Battle Unit Status"), StatusWidgetAssetLabel, true);
	AuditClassExposesWidgetBinding(Rows, StatusWidgetClass, TEXT("ShieldText"), TEXT("Battle Unit Status"), StatusWidgetAssetLabel, true);
	AuditClassExposesWidgetBinding(Rows, StatusWidgetClass, TEXT("StunText"), TEXT("Battle Unit Status"), StatusWidgetAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, StatusWidgetClass, TEXT("FreezeText"), TEXT("Battle Unit Status"), StatusWidgetAssetLabel, false);
	AddRow(
		Rows,
		TEXT("Info"),
		TEXT("Battle Unit Status"),
		StatusWidgetAssetLabel,
		TEXT("Manual Layout Check"),
		TEXT("Status widgets should bind HPText, AttackText, and ShieldText. StunText and FreezeText are optional but recommended for readable combat status."));
}

template <typename AssetType>
void LoadAssetsFromPaths(const TArray<FName>& ScanPaths, TArray<AssetType*>& OutAssets)
{
	OutAssets.Reset();

	if (ScanPaths.Num() == 0)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> PathStrings;
	PathStrings.Reserve(ScanPaths.Num());
	for (const FName& Path : ScanPaths)
	{
		PathStrings.Add(Path.ToString());
	}

	AssetRegistry.ScanPathsSynchronous(PathStrings, true);

	TSet<FString> SeenAssets;
	for (const FName& Path : ScanPaths)
	{
		TArray<FAssetData> AssetDataList;
		AssetRegistry.GetAssetsByPath(Path, AssetDataList, true);

		for (const FAssetData& AssetData : AssetDataList)
		{
			AssetType* Asset = Cast<AssetType>(AssetData.GetAsset());
			if (!Asset || SeenAssets.Contains(Asset->GetPathName()))
			{
				continue;
			}

			SeenAssets.Add(Asset->GetPathName());
			OutAssets.Add(Asset);
		}
	}
}

void LoadBlueprintClassesFromPaths(const TArray<FName>& ScanPaths, TArray<UClass*>& OutClasses)
{
	OutClasses.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> PathStrings;
	PathStrings.Reserve(ScanPaths.Num());
	for (const FName& Path : ScanPaths)
	{
		PathStrings.Add(Path.ToString());
	}

	AssetRegistry.ScanPathsSynchronous(PathStrings, true);

	TSet<FString> SeenClassPaths;
	for (const FName& Path : ScanPaths)
	{
		TArray<FAssetData> AssetDataList;
		AssetRegistry.GetAssetsByPath(Path, AssetDataList, true);

		for (const FAssetData& AssetData : AssetDataList)
		{
			const FString PackagePath = AssetData.PackageName.ToString();
			const FString ClassPath = FString::Printf(TEXT("%s.%s_C"), *PackagePath, *FPackageName::GetShortName(PackagePath));
			if (SeenClassPaths.Contains(ClassPath))
			{
				continue;
			}

			UClass* LoadedClass = LoadClass<UObject>(nullptr, *ClassPath);
			if (!LoadedClass)
			{
				continue;
			}

			SeenClassPaths.Add(ClassPath);
			OutClasses.Add(LoadedClass);
		}
	}
}

FName GetReflectedFName(UObject* Object, FName PropertyName)
{
	if (!Object)
	{
		return NAME_None;
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	const FNameProperty* NameProperty = CastField<FNameProperty>(Property);
	return NameProperty ? NameProperty->GetPropertyValue_InContainer(Object) : NAME_None;
}

bool GetReflectedBool(UObject* Object, FName PropertyName, bool bFallback = false)
{
	if (!Object)
	{
		return bFallback;
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property);
	return BoolProperty ? BoolProperty->GetPropertyValue_InContainer(Object) : bFallback;
}

bool CardHasElementGenerator(const UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	for (const FCardEffectSpec& Effect : Card->Effects)
	{
		if (Effect.Operation == ECardEffectOperation::GainElementCharge)
		{
			return true;
		}
	}

	for (const FCardElementalBonusGroup& BonusGroup : Card->ElementalBonusGroups)
	{
		for (const FCardEffectSpec& Effect : BonusGroup.BonusEffects)
		{
			if (Effect.Operation == ECardEffectOperation::GainElementCharge)
			{
				return true;
			}
		}
	}

	return false;
}

void AuditElementContent(TArray<FProjectAuditRow>& Rows, const TArray<FName>& CardScanPaths)
{
	TArray<UCardDefinition*> Cards;
	LoadAssetsFromPaths(CardScanPaths, Cards);

	int32 ElementGeneratorCount = 0;
	int32 ElementalBonusCount = 0;
	for (const UCardDefinition* Card : Cards)
	{
		if (!Card)
		{
			continue;
		}

		if (CardHasElementGenerator(Card))
		{
			ElementGeneratorCount++;
		}

		if (Card->ElementalBonusGroups.Num() > 0)
		{
			ElementalBonusCount++;
		}
	}

	AddRow(
		Rows,
		TEXT("Info"),
		TEXT("Element Content"),
		TEXT("Cards"),
		TEXT("Scanned"),
		FString::Printf(TEXT("Found %d cards. Element generators: %d. Elemental bonus cards: %d."), Cards.Num(), ElementGeneratorCount, ElementalBonusCount));

	if (Cards.Num() > 0 && ElementGeneratorCount == 0)
	{
		AddRow(
			Rows,
			TEXT("Warning"),
			TEXT("Element Content"),
			TEXT("Cards"),
			TEXT("No Generators"),
			TEXT("Element charge backend exists, but no scanned card currently gains element charges."));
	}

	if (Cards.Num() > 0 && ElementalBonusCount == 0)
	{
		AddRow(
			Rows,
			TEXT("Warning"),
			TEXT("Element Content"),
			TEXT("Cards"),
			TEXT("No Bonuses"),
			TEXT("ElementalBonusGroups exist, but no scanned card currently uses them."));
	}
}

void AuditDataAssetCounts(
	TArray<FProjectAuditRow>& Rows,
	const TArray<FName>& CardScanPaths,
	const TArray<FName>& PackScanPaths,
	const TArray<FName>& RelicScanPaths)
{
	TArray<UCardDefinition*> Cards;
	TArray<UCardPackDefinition*> Packs;
	TArray<UJargonRelicDefinition*> Relics;
	LoadAssetsFromPaths(CardScanPaths, Cards);
	LoadAssetsFromPaths(PackScanPaths, Packs);
	LoadAssetsFromPaths(RelicScanPaths, Relics);

	AddRow(
		Rows,
		Cards.Num() > 0 ? TEXT("Info") : TEXT("Warning"),
		TEXT("Data Assets"),
		TEXT("Cards"),
		Cards.Num() > 0 ? TEXT("Found") : TEXT("Missing"),
		FString::Printf(TEXT("Found %d card definitions under configured scan paths."), Cards.Num()));

	AddRow(
		Rows,
		Packs.Num() > 0 ? TEXT("Info") : TEXT("Warning"),
		TEXT("Data Assets"),
		TEXT("Card Packs"),
		Packs.Num() > 0 ? TEXT("Found") : TEXT("Missing"),
		FString::Printf(TEXT("Found %d card pack definitions under configured scan paths."), Packs.Num()));

	AddRow(
		Rows,
		Relics.Num() > 0 ? TEXT("Info") : TEXT("Warning"),
		TEXT("Data Assets"),
		TEXT("Relics"),
		Relics.Num() > 0 ? TEXT("Found") : TEXT("Missing"),
		FString::Printf(TEXT("Found %d relic definitions under configured scan paths."), Relics.Num()));
}

void AuditRewardInteractables(TArray<FProjectAuditRow>& Rows, const TArray<FName>& RewardBlueprintScanPaths)
{
	TArray<UClass*> BlueprintClasses;
	LoadBlueprintClassesFromPaths(RewardBlueprintScanPaths, BlueprintClasses);

	int32 RewardClassCount = 0;
	for (UClass* BlueprintClass : BlueprintClasses)
	{
		if (!BlueprintClass || !BlueprintClass->IsChildOf(AExplorationRewardInteractable::StaticClass()))
		{
			continue;
		}

		RewardClassCount++;
		UObject* CDO = BlueprintClass->GetDefaultObject();
		const FName CompletionId = GetReflectedFName(CDO, TEXT("CompletionId"));
		const bool bClaimOnOverlap = GetReflectedBool(CDO, TEXT("bClaimOnOverlap"), true);

		AddRow(
			Rows,
			CompletionId.IsNone() ? TEXT("Warning") : TEXT("Info"),
			TEXT("Exploration Rewards"),
			BlueprintClass->GetPathName(),
			CompletionId.IsNone() ? TEXT("Needs Instance Check") : TEXT("Configured"),
			CompletionId.IsNone()
				? FString::Printf(TEXT("Class default CompletionId is empty. Placed instances should set a stable CompletionId or intentionally rely on actor name fallback. bClaimOnOverlap=%s."), bClaimOnOverlap ? TEXT("true") : TEXT("false"))
				: FString::Printf(TEXT("CompletionId=%s, bClaimOnOverlap=%s."), *CompletionId.ToString(), bClaimOnOverlap ? TEXT("true") : TEXT("false")));
	}

	AddRow(
		Rows,
		TEXT("Info"),
		TEXT("Exploration Rewards"),
		TEXT("Blueprint Scan"),
		TEXT("Scanned"),
		FString::Printf(TEXT("Found %d reward interactable Blueprint classes under configured scan paths."), RewardClassCount));
}

FString BuildCsv(const TArray<FProjectAuditRow>& Rows)
{
	FString Csv = TEXT("Severity,Area,Asset,Status,Message") LINE_TERMINATOR;
	for (const FProjectAuditRow& Row : Rows)
	{
		TArray<FString> Fields;
		Fields.Add(CsvEscape(Row.Severity));
		Fields.Add(CsvEscape(Row.Area));
		Fields.Add(CsvEscape(Row.Asset));
		Fields.Add(CsvEscape(Row.Status));
		Fields.Add(CsvEscape(Row.Message));
		Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
	}
	return Csv;
}
#endif
}

using namespace UE::Jargon::ProjectSetupAuditToolPrivate;

UProjectSetupAuditTool::UProjectSetupAuditTool()
{
	TownMapPath = DefaultTownMapPath;
	ExplorationMapPath = DefaultExplorationMapPath;
	CombatMapPath = DefaultCombatMapPath;
	TownGameModeBlueprintPath = DefaultTownGameModePath;
	TownPlayerControllerBlueprintPath = DefaultTownPlayerControllerPath;
	ExplorationGameModeBlueprintPath = DefaultExplorationGameModePath;
	ExplorationPlayerControllerBlueprintPath = DefaultExplorationPlayerControllerPath;
	CombatGameModeBlueprintPath = DefaultCombatGameModePath;
	CardScanPaths.Add(DefaultCardScanPath);
	CardScanPaths.Add(LegacyCardScanPath);
	PackScanPaths.Add(DefaultPackScanPath);
	PackScanPaths.Add(LegacyPackScanPath);
	RelicScanPaths.Add(DefaultRelicScanPath);
	RewardBlueprintScanPaths.Add(DefaultRewardBlueprintScanPath);
}

void UProjectSetupAuditTool::RunProjectSetupAudit()
{
#if WITH_EDITOR
	TArray<FProjectAuditRow> Rows;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TSet<FString> PathsToScan;
	const auto AddPackageDirectoryToScan = [&PathsToScan](FName PackagePath)
	{
		const FString PackagePathString = PackagePath.ToString();
		if (!PackagePathString.IsEmpty())
		{
			PathsToScan.Add(FPackageName::GetLongPackagePath(PackagePathString));
		}
	};
	AddPackageDirectoryToScan(TownMapPath);
	AddPackageDirectoryToScan(ExplorationMapPath);
	AddPackageDirectoryToScan(CombatMapPath);
	AddPackageDirectoryToScan(TownGameModeBlueprintPath);
	AddPackageDirectoryToScan(TownPlayerControllerBlueprintPath);
	AddPackageDirectoryToScan(ExplorationGameModeBlueprintPath);
	AddPackageDirectoryToScan(ExplorationPlayerControllerBlueprintPath);
	AddPackageDirectoryToScan(CombatGameModeBlueprintPath);
	for (const FName& Path : CardScanPaths)
	{
		PathsToScan.Add(Path.ToString());
	}
	for (const FName& Path : PackScanPaths)
	{
		PathsToScan.Add(Path.ToString());
	}
	for (const FName& Path : RelicScanPaths)
	{
		PathsToScan.Add(Path.ToString());
	}
	for (const FName& Path : RewardBlueprintScanPaths)
	{
		PathsToScan.Add(Path.ToString());
	}
	PathsToScan.Add(TopDownTemplatePath.ToString());
	PathsToScan.Add(VariantStrategyTemplatePath.ToString());
	PathsToScan.Add(VariantTwinStickTemplatePath.ToString());
	TArray<FString> PathsToScanArray = PathsToScan.Array();
	AssetRegistry.ScanPathsSynchronous(PathsToScanArray, true);

	AuditAssetExists(Rows, AssetRegistry, TownMapPath, TEXT("Maps"));
	AuditAssetExists(Rows, AssetRegistry, ExplorationMapPath, TEXT("Maps"));
	AuditAssetExists(Rows, AssetRegistry, CombatMapPath, TEXT("Maps"));
	AuditAssetExists(Rows, AssetRegistry, TownGameModeBlueprintPath, TEXT("Blueprints"));
	AuditAssetExists(Rows, AssetRegistry, TownPlayerControllerBlueprintPath, TEXT("Blueprints"));
	AuditAssetExists(Rows, AssetRegistry, ExplorationGameModeBlueprintPath, TEXT("Blueprints"));
	AuditAssetExists(Rows, AssetRegistry, ExplorationPlayerControllerBlueprintPath, TEXT("Blueprints"));
	AuditAssetExists(Rows, AssetRegistry, CombatGameModeBlueprintPath, TEXT("Blueprints"));

	AuditTemplateAndLegacyReferences(
		Rows,
		AssetRegistry,
		CardScanPaths,
		PackScanPaths,
		{
			TownMapPath,
			ExplorationMapPath,
			CombatMapPath,
			TownGameModeBlueprintPath,
			TownPlayerControllerBlueprintPath,
			ExplorationGameModeBlueprintPath,
			ExplorationPlayerControllerBlueprintPath,
			CombatGameModeBlueprintPath
		});

	AuditMapDependency(Rows, AssetRegistry, TownMapPath, TownGameModeBlueprintPath, TEXT("Map Hooks"), TEXT("Town map"));
	AuditMapDependency(Rows, AssetRegistry, ExplorationMapPath, ExplorationGameModeBlueprintPath, TEXT("Map Hooks"), TEXT("Exploration map"));
	AuditMapDependency(Rows, AssetRegistry, CombatMapPath, CombatGameModeBlueprintPath, TEXT("Map Hooks"), TEXT("Combat map"));
	AuditTownMapExplorationContent(Rows, AssetRegistry, TownMapPath);

	UObject* TownGameModeCDO = GetClassDefaultObjectFromBlueprint(TownGameModeBlueprintPath);
	AuditObjectPropertyAssigned(Rows, TownGameModeCDO, TEXT("DefaultHeroDefinition"), TEXT("Town GameMode"), TownGameModeBlueprintPath.ToString(), true);
	AuditObjectPropertyAssigned(Rows, TownGameModeCDO, TEXT("DefaultPawnClass"), TEXT("Town GameMode"), TownGameModeBlueprintPath.ToString(), true);
	AuditObjectPropertyAssigned(Rows, TownGameModeCDO, TEXT("PlayerControllerClass"), TEXT("Town GameMode"), TownGameModeBlueprintPath.ToString(), true);

	UObject* TownPlayerControllerCDO = GetClassDefaultObjectFromBlueprint(TownPlayerControllerBlueprintPath);
	AuditObjectPropertyAssigned(Rows, TownPlayerControllerCDO, TEXT("InteractionPromptWidgetClass"), TEXT("Town PlayerController"), TownPlayerControllerBlueprintPath.ToString(), false);
	AddRow(
		Rows,
		TEXT("Info"),
		TEXT("Interaction Prompt"),
		TownPlayerControllerBlueprintPath.ToString(),
		TEXT("Manual Layout Check"),
		TEXT("WBP_InteractionPrompt should use a compact desired-size root and bind PromptText/VerbText. Full-screen roots can display in unexpected places."));

	UObject* ExplorationGameModeCDO = GetClassDefaultObjectFromBlueprint(ExplorationGameModeBlueprintPath);
	AuditObjectPropertyAssigned(Rows, ExplorationGameModeCDO, TEXT("DefaultPawnClass"), TEXT("Exploration GameMode"), ExplorationGameModeBlueprintPath.ToString(), true);
	AuditObjectPropertyAssigned(Rows, ExplorationGameModeCDO, TEXT("PlayerControllerClass"), TEXT("Exploration GameMode"), ExplorationGameModeBlueprintPath.ToString(), true);

	UObject* ExplorationPlayerControllerCDO = GetClassDefaultObjectFromBlueprint(ExplorationPlayerControllerBlueprintPath);
	AuditObjectPropertyAssigned(Rows, ExplorationPlayerControllerCDO, TEXT("InteractionPromptWidgetClass"), TEXT("Exploration PlayerController"), ExplorationPlayerControllerBlueprintPath.ToString(), false);

	UObject* CombatGameModeCDO = GetClassDefaultObjectFromBlueprint(CombatGameModeBlueprintPath);
	AuditObjectPropertyAssigned(Rows, CombatGameModeCDO, TEXT("PlayerControllerClass"), TEXT("Combat GameMode"), CombatGameModeBlueprintPath.ToString(), true);
	AuditObjectPropertyAssigned(Rows, CombatGameModeCDO, TEXT("PlayerUnitClass"), TEXT("Combat GameMode"), CombatGameModeBlueprintPath.ToString(), true);
	AuditObjectPropertyAssigned(Rows, CombatGameModeCDO, TEXT("CombatHUDClass"), TEXT("Combat GameMode"), CombatGameModeBlueprintPath.ToString(), true);
	AuditObjectPropertyAssigned(Rows, CombatGameModeCDO, TEXT("PresentationManagerClass"), TEXT("Combat GameMode"), CombatGameModeBlueprintPath.ToString(), false);
	AuditObjectPropertyAssigned(Rows, CombatGameModeCDO, TEXT("PresentationSettings"), TEXT("Combat GameMode"), CombatGameModeBlueprintPath.ToString(), false);

	UClass* PlayerUnitClass = GetReflectedClass(CombatGameModeCDO, TEXT("PlayerUnitClass"));
	AuditBattleUnitStatusWidget(Rows, PlayerUnitClass, CombatGameModeBlueprintPath.ToString());

	UClass* CombatHUDClass = GetReflectedClass(CombatGameModeCDO, TEXT("CombatHUDClass"));
	const FString CombatHUDAssetLabel = CombatHUDClass ? CombatHUDClass->GetPathName() : CombatGameModeBlueprintPath.ToString();
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("FireChargeText"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("FrostChargeText"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("StormChargeText"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("NatureChargeText"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("RadianceChargeText"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("QuietusChargeText"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("FireChargeIcon"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("FrostChargeIcon"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("StormChargeIcon"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("NatureChargeIcon"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("RadianceChargeIcon"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AuditClassExposesWidgetBinding(Rows, CombatHUDClass, TEXT("QuietusChargeIcon"), TEXT("Combat HUD"), CombatHUDAssetLabel, false);
	AddRow(
		Rows,
		TEXT("Info"),
		TEXT("Combat HUD"),
		CombatGameModeBlueprintPath.ToString(),
		TEXT("Manual Layout Check"),
		TEXT("Combat HUD expects per-element count bindings: FireChargeText, FrostChargeText, StormChargeText, NatureChargeText, RadianceChargeText, QuietusChargeText. Optional icon bindings use matching Icon names."));

	AuditDataAssetCounts(Rows, CardScanPaths, PackScanPaths, RelicScanPaths);
	AuditElementContent(Rows, CardScanPaths);
	AuditRewardInteractables(Rows, RewardBlueprintScanPaths);

	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	for (const FProjectAuditRow& Row : Rows)
	{
		if (Row.Severity == TEXT("Error"))
		{
			ErrorCount++;
		}
		else if (Row.Severity == TEXT("Warning"))
		{
			WarningCount++;
		}
	}

	UE_LOG(LogProjectSetupAudit, Display, TEXT("=== Jargon Project Setup Audit ==="));
	UE_LOG(LogProjectSetupAudit, Display, TEXT("Rows: %d | Errors: %d | Warnings: %d"), Rows.Num(), ErrorCount, WarningCount);
	for (const FProjectAuditRow& Row : Rows)
	{
		if (Row.Severity == TEXT("Error"))
		{
			UE_LOG(LogProjectSetupAudit, Error, TEXT("[%s][%s] %s - %s"), *Row.Area, *Row.Status, *Row.Asset, *Row.Message);
		}
		else if (Row.Severity == TEXT("Warning"))
		{
			UE_LOG(LogProjectSetupAudit, Warning, TEXT("[%s][%s] %s - %s"), *Row.Area, *Row.Status, *Row.Asset, *Row.Message);
		}
		else
		{
			UE_LOG(LogProjectSetupAudit, Display, TEXT("[%s][%s] %s - %s"), *Row.Area, *Row.Status, *Row.Asset, *Row.Message);
		}
	}

	if (bExportCsvReport)
	{
		const FString SafeSubdirectory = OutputSubdirectory.IsEmpty() ? TEXT("ProjectAudit") : OutputSubdirectory;
		const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), SafeSubdirectory);
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);

		const FString CsvPath = FPaths::Combine(OutputDirectory, TEXT("ProjectSetupAudit.csv"));
		const bool bSavedCsv = FFileHelper::SaveStringToFile(BuildCsv(Rows), *CsvPath);
		if (bSavedCsv)
		{
			UE_LOG(LogProjectSetupAudit, Display, TEXT("Wrote project setup audit CSV: %s"), *CsvPath);
		}
		else
		{
			UE_LOG(LogProjectSetupAudit, Error, TEXT("Failed to write project setup audit CSV: %s"), *CsvPath);
		}
	}
#else
	UE_LOG(LogProjectSetupAudit, Warning, TEXT("Project Setup Audit is editor-only and cannot scan assets in this build."));
#endif
}
