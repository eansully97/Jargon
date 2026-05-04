#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JargonAbilityAuditTool.generated.h"

/**
 * Editor-only audit helper for reusable ability definitions and remaining raw non-card effect hooks.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Ability Audit Tool"))
class JARGON_API UJargonAbilityAuditTool : public UDataAsset
{
	GENERATED_BODY()

public:
	UJargonAbilityAuditTool();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability Audit|Scan", meta = (ToolTip = "Content roots to scan for ability definitions and non-card definitions that still expose raw effect hooks."))
	TArray<FName> ScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability Audit|Output", meta = (ToolTip = "Subfolder under Project/Saved where ability audit CSV reports are written."))
	FString OutputSubfolder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability Audit|Output", meta = (ToolTip = "When true, writes AbilityAudit.csv under Project/Saved/OutputSubfolder."))
	bool bExportCsvReport = true;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Ability Audit", meta = (ToolTip = "Scans ability definitions and remaining raw non-card effect hooks, then writes a migration-focused CSV report. Does not modify assets."))
	void RunAbilityAudit();
};
