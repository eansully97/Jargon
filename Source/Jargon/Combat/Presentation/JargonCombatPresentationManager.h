#pragma once

#include "CoreMinimal.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "GameFramework/Actor.h"
#include "JargonCombatPresentationManager.generated.h"

class AJargonCombatGameMode;
class UJargonCombatPresentationSettings;

UCLASS(Blueprintable)
class JARGON_API AJargonCombatPresentationManager : public AActor
{
	GENERATED_BODY()

public:
	AJargonCombatPresentationManager();

	void InitializePresentation(UJargonCombatPresentationSettings* InSettings, AJargonCombatGameMode* InCombatGameMode);

	UFUNCTION(BlueprintCallable, Category = "Combat Presentation")
	void HandleCombatCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintCallable, Category = "Combat Presentation|Debug")
	bool ValidatePresentationSetup(bool bLogWarnings = true) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool GetBestCueLocation(const FJargonCombatCueEvent& Cue, FVector& OutLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool GetCueSourceLocation(const FJargonCombatCueEvent& Cue, FVector& OutLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool GetCueTargetLocation(const FJargonCombatCueEvent& Cue, FVector& OutLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	FText GetCueDisplayText(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	FLinearColor GetCueDisplayColor(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool IsUnitCue(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool IsTileCue(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool IsRelicCue(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation")
	UJargonCombatPresentationSettings* GetPresentationSettings() const
	{
		return PresentationSettings;
	}

protected:
	void PlayConfiguredVFX(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void PlayConfiguredSFX(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void SpawnFloatingText(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void DrawDebugCue(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void DispatchBlueprintCueEvents(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnCombatCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnDamageCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnHealCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnShieldCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnStunCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnUnitCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnTileEffectCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnRelicTriggeredCue(const FJargonCombatCueEvent& Cue);

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Presentation")
	TObjectPtr<UJargonCombatPresentationSettings> PresentationSettings = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Presentation")
	TObjectPtr<AJargonCombatGameMode> CombatGameMode = nullptr;
};
