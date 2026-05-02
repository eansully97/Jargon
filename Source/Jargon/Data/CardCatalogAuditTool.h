#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardCatalogAuditTool.generated.h"

/**
 * Editor-facing read-only audit helper for card and pack Data Assets.
 *
 * Create one of these assets, adjust the scan paths if needed, then run the
 * Call In Editor audit function to log a report and export CSV snapshots.
 */
UCLASS(BlueprintType)
class JARGON_API UCardCatalogAuditTool : public UDataAsset
{
	GENERATED_BODY()

public:
	UCardCatalogAuditTool();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Card Catalog")
	void RunCardCatalogAudit();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Catalog|Scan", meta = (ToolTip = "Package paths scanned recursively for UCardDefinition assets. Example: /Game/Jargon/Data/Cards"))
	TArray<FName> CardScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Catalog|Scan", meta = (ToolTip = "Package paths scanned recursively for UCardPackDefinition assets. Example: /Game/Jargon/Data/CardPacks"))
	TArray<FName> PackScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Catalog|Scan", meta = (ToolTip = "Package paths scanned recursively for UJargonSummonedUnitDefinition assets. Example: /Game/Jargon/Data/Cards/SummonedDefinitions"))
	TArray<FName> SummonedUnitScanPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Catalog|Output", meta = (ToolTip = "Subfolder under Project/Saved where CSV reports are written."))
	FString OutputSubdirectory = TEXT("CardCatalog");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Catalog|Output")
	bool bExportCsvReports = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Catalog|Balance", meta = (ToolTip = "When CSV export is enabled, also writes card balance, variety matrix, and generated rules-text suggestion reports. These reports never modify card assets."))
	bool bExportBalanceReports = true;
};
