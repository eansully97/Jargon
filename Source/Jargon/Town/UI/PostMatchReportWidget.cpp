#include "Town/UI/PostMatchReportWidget.h"

void UPostMatchReportWidget::RefreshFromReportData(const FJargonPostCombatReportData& InReportData)
{
	ReportData = InReportData;
	BP_OnReportDataRefreshed();
}
