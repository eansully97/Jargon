#pragma once

#include "CoreMinimal.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "Engine/DataAsset.h"
#include "JargonCombatPresentationSettings.generated.h"

class UNiagaraSystem;
class USoundBase;
class UJargonFloatingCombatTextWidget;

UCLASS(BlueprintType)
class JARGON_API UJargonCombatPresentationSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UJargonCombatPresentationSettings();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	bool bEnableVFX = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	bool bEnableSFX = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	bool bEnableFloatingText = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX")
	TMap<EJargonCombatCueType, TObjectPtr<UNiagaraSystem>> DefaultNiagaraByCue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|SFX")
	TMap<EJargonCombatCueType, TObjectPtr<USoundBase>> DefaultSoundByCue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text")
	TSubclassOf<UJargonFloatingCombatTextWidget> FloatingTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text")
	int32 FloatingTextZOrder = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text")
	FVector FloatingTextWorldOffset = FVector(0.f, 0.f, 90.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text")
	FVector2D FloatingTextScreenOffset = FVector2D(0.f, -32.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Debug", meta = (ToolTip = "Runs one-time setup checks when the presentation manager initializes."))
	bool bEnableDebugValidation = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Debug", meta = (ToolTip = "Draws temporary world-space cue markers for verifying cue locations. Disabled in shipping/test builds."))
	bool bEnableDebugCueDraw = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Debug", meta = (EditCondition = "bEnableDebugCueDraw", ClampMin = "0.01"))
	float DebugCueDuration = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Debug", meta = (EditCondition = "bEnableDebugCueDraw", ClampMin = "1.0", ToolTip = "Base world-space size for debug cue markers. AoE radius cues multiply this by the cue radius."))
	float DebugCueScale = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor DamageColor = FLinearColor(1.f, 0.12f, 0.08f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor HealColor = FLinearColor(0.1f, 0.95f, 0.28f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor ShieldColor = FLinearColor(0.2f, 0.65f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor StunColor = FLinearColor(0.95f, 0.82f, 0.1f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor RelicColor = FLinearColor(0.85f, 0.45f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor DefaultColor = FLinearColor::White;

	UFUNCTION(BlueprintPure, Category = "Presentation")
	FLinearColor GetColorForCue(EJargonCombatCueType CueType) const;
};
