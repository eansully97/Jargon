#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectSetupAuditTool.generated.h"

/**
 * Editor-only setup validator for the main prototype loop.
 *
 * This tool does not modify assets. It checks expected C++/Blueprint hook
 * assignments, reports likely setup gaps, and writes a small CSV to Saved.
 */
UCLASS(BlueprintType)
class JARGON_API UProjectSetupAuditTool : public UDataAsset
{
	GENERATED_BODY()

public:
	UProjectSetupAuditTool();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Setup Audit")
	void RunProjectSetupAudit();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Maps", meta = (ToolTip = "Expected Town map package path. Used for presence and dependency checks."))
	FName TownMapPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Maps", meta = (ToolTip = "Expected Exploration map package path. Used for presence and dependency checks."))
	FName ExplorationMapPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Maps", meta = (ToolTip = "Expected Combat map package path. Used for presence and dependency checks."))
	FName CombatMapPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Blueprints", meta = (ToolTip = "Expected Town GameMode Blueprint package path."))
	FName TownGameModeBlueprintPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Blueprints", meta = (ToolTip = "Expected Town PlayerController Blueprint package path."))
	FName TownPlayerControllerBlueprintPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Blueprints", meta = (ToolTip = "Expected Exploration GameMode Blueprint package path."))
	FName ExplorationGameModeBlueprintPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Blueprints", meta = (ToolTip = "Expected Exploration PlayerController Blueprint package path."))
	FName ExplorationPlayerControllerBlueprintPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Blueprints", meta = (ToolTip = "Expected Combat GameMode Blueprint package path."))
	FName CombatGameModeBlueprintPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Assets", meta = (ToolTip = "Card paths scanned recursively for element/content proof checks."))
	TArray<FName> CardScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Assets", meta = (ToolTip = "Card pack paths scanned recursively for basic pack content checks."))
	TArray<FName> PackScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Assets", meta = (ToolTip = "Artifact paths scanned recursively for basic artifact content checks."))
	TArray<FName> ArtifactScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Assets", meta = (ToolTip = "Blueprint paths scanned recursively for reward interactable setup checks."))
	TArray<FName> RewardBlueprintScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Output", meta = (ToolTip = "Subfolder under Project/Saved where the setup audit CSV is written."))
	FString OutputSubdirectory = TEXT("ProjectAudit");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Project Setup Audit|Output")
	bool bExportCsvReport = true;
};
