#pragma once

#include "CoreMinimal.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "GameFramework/Actor.h"
#include "JargonCombatPresentationManager.generated.h"

class AJargonCombatGameMode;
class ABattleUnit;
class UNiagaraComponent;
class UJargonCombatPresentationSettings;

UCLASS(Blueprintable)
class JARGON_API AJargonCombatPresentationManager : public AActor
{
	GENERATED_BODY()

public:
	AJargonCombatPresentationManager();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Wires this runtime manager to combat-owned settings and GameMode state. */
	void InitializePresentation(UJargonCombatPresentationSettings* InSettings, AJargonCombatGameMode* InCombatGameMode);

	/** Presentation entry point for gameplay cues; safe for Blueprint forwarding and C++ GameMode calls. */
	UFUNCTION(BlueprintCallable, Category = "Combat Presentation")
	void HandleCombatCue(const FJargonCombatCueEvent& Cue);

	/** Debug-only style setup check for missing presentation settings/classes; it cannot prove visual correctness. */
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
	bool IsArtifactCue(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	int32 GetCueTypeId(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	float GetCueDurationScale(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	float GetCueRadius(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	float GetCueValueAsFloat(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool HasValidSourceLocation(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation|Cue")
	bool HasValidTargetLocation(const FJargonCombatCueEvent& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Combat Presentation")
	UJargonCombatPresentationSettings* GetPresentationSettings() const
	{
		return PresentationSettings;
	}

protected:
	void PlayConfiguredVFX(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void ApplyCueParametersToNiagaraComponent(UNiagaraComponent* NiagaraComponent, const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	bool ResolveCueDirection(const FJargonCombatCueEvent& Cue, FVector& OutDirection, float& OutSourceToTargetDistance) const;
	float ResolveSimpleVFXComponentScale(const FJargonCombatCueEvent& Cue) const;
	void PlayConfiguredSFX(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void SpawnFloatingText(const FJargonCombatCueEvent& Cue, const FVector& CueLocation);
	void SpawnFloatingTextImmediate(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void QueueAggregatedDamageFloatingText(const FJargonCombatCueEvent& Cue, const FVector& CueLocation);
	void FlushAggregatedDamageFloatingText();
	bool ShouldSpawnFloatingTextForCue(const FJargonCombatCueEvent& Cue) const;
	void DrawDebugCue(const FJargonCombatCueEvent& Cue, const FVector& CueLocation) const;
	void DispatchBlueprintCueEvents(const FJargonCombatCueEvent& Cue);

	/** Blueprint catch-all hook for additional cue presentation. Gameplay should already be resolved. */
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
	void BP_OnFreezeCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnStatusCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnUnitCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnTileEffectCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnArtifactTriggeredCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnHeroClassPassiveTriggeredCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnHeroAspectTriggeredCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Presentation")
	void BP_OnElementalBonusTriggeredCue(const FJargonCombatCueEvent& Cue);

protected:
	/** Data Asset assigned by the combat GameMode; presentation only, no gameplay authority. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Presentation")
	TObjectPtr<UJargonCombatPresentationSettings> PresentationSettings = nullptr;

	/** Owning combat GameMode used for coordinate and viewport lookups. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat Presentation")
	TObjectPtr<AJargonCombatGameMode> CombatGameMode = nullptr;

	struct FPendingFloatingDamageCue
	{
		FJargonCombatCueEvent Cue;
		FVector CueLocation = FVector::ZeroVector;
		int32 TotalValue = 0;
	};

	/** Short-lived aggregation buffer so rapid damage ticks can present as one floating number. */
	TMap<TWeakObjectPtr<ABattleUnit>, FPendingFloatingDamageCue> PendingDamageFloatingTextByTarget;
	FTimerHandle DamageFloatingTextAggregationTimerHandle;
};
