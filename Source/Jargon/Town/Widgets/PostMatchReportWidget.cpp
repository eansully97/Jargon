#include "PostMatchReportWidget.h"

#include "Components/Button.h"

void UPostMatchReportWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &UPostMatchReportWidget::HandleContinueButtonClicked);
		ContinueButton->OnClicked.AddDynamic(this, &UPostMatchReportWidget::HandleContinueButtonClicked);
	}
}

void UPostMatchReportWidget::RefreshFromReportData(const FJargonPostCombatReportData& InReportData)
{
	ReportData = InReportData;
	BP_OnReportDataRefreshed();
}

void UPostMatchReportWidget::RequestContinue()
{
	BP_OnContinueRequested();
	OnPostMatchContinueRequested.Broadcast();
}

void UPostMatchReportWidget::HandleContinueButtonClicked()
{
	RequestContinue();
}
