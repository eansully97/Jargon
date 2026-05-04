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

	/** Master presentation toggles; disabling these never changes gameplay resolution. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	bool bEnableVFX = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	bool bEnableSFX = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	bool bEnableFloatingText = true;

	/** Default Niagara systems keyed by cue type; Blueprint presentation can still add bespoke effects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX")
	TMap<EJargonCombatCueType, TObjectPtr<UNiagaraSystem>> DefaultNiagaraByCue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform")
	bool bUseSimpleVFXTransformMode = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode"))
	bool bOrientVFXToCueDirection = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode && bOrientVFXToCueDirection"))
	bool bInvertVFXDirection = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode"))
	bool bScaleVFXComponentByCueValue = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode", ClampMin = "0.01"))
	float BaseVFXComponentScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode && bScaleVFXComponentByCueValue", ClampMin = "0.0"))
	float ValueVFXComponentScaleAmount = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode && bScaleVFXComponentByCueValue", ClampMin = "0.0"))
	float RadiusVFXComponentScaleAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode", ClampMin = "0.01"))
	float MinVFXComponentScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Simple Transform", meta = (EditCondition = "bUseSimpleVFXTransformMode", ClampMin = "0.01"))
	float MaxVFXComponentScale = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	bool bApplyNiagaraCueParameters = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.01"))
	float DefaultDurationScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName CueColorParameterName = TEXT("User.CueColor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName ValueParameterName = TEXT("User.Value");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName RadiusParameterName = TEXT("User.Radius");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName CueLocationParameterName = TEXT("User.CueLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName SourceLocationParameterName = TEXT("User.SourceLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName TargetLocationParameterName = TEXT("User.TargetLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName DurationScaleParameterName = TEXT("User.DurationScale");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName CueTypeIdParameterName = TEXT("User.CueTypeId");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName HasSourceLocationParameterName = TEXT("User.HasSourceLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName HasTargetLocationParameterName = TEXT("User.HasTargetLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName CueDirectionParameterName = TEXT("User.CueDirection");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName HasCueDirectionParameterName = TEXT("User.HasCueDirection");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName SourceToTargetDistanceParameterName = TEXT("User.SourceToTargetDistance");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName SpriteSizeParameterName = TEXT("User.SpriteSize");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters")
	FName SpriteScaleParameterName = TEXT("User.SpriteScale");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "1.0"))
	float BaseSpriteSize = 32.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.0"))
	float ValueSpriteSizeScale = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "1.0"))
	float MaxSpriteSize = 96.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.01"))
	float BaseSpriteScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.0"))
	float ValueSpriteScaleAmount = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.0"))
	float RadiusSpriteScaleAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.01"))
	float MinSpriteScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|VFX|Parameters", meta = (ClampMin = "0.01"))
	float MaxSpriteScale = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|SFX")
	TMap<EJargonCombatCueType, TObjectPtr<USoundBase>> DefaultSoundByCue;

	/** Widget class spawned into the viewport for native floating combat text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text")
	TSubclassOf<UJargonFloatingCombatTextWidget> FloatingTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text", meta = (ToolTip = "When enabled, floating combat text prefers numeric feedback such as damage, healing, and shield gain. Explicit presentation text from important cues, such as hero passives and elemental bonuses, is still allowed."))
	bool bShowOnlyNumericFloatingText = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text", meta = (ToolTip = "When enabled, rapid damage cues against the same unit are collapsed into one floating damage number. This is presentation-only and does not change damage timing or rules."))
	bool bAggregateDamageFloatingText = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text", meta = (EditCondition = "bAggregateDamageFloatingText", ClampMin = "0.0", ToolTip = "Seconds to wait for additional damage cues before showing one combined floating damage number."))
	float DamageFloatingTextAggregationWindow = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Floating Text")
	int32 FloatingTextZOrder = -10;

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
	FLinearColor FreezeColor = FLinearColor(0.35f, 0.85f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor ArtifactColor = FLinearColor(0.85f, 0.45f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor ElementalBonusColor = FLinearColor(1.f, 0.58f, 0.12f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Colors")
	FLinearColor DefaultColor = FLinearColor::White;

	/** Returns the authored presentation color for a cue type, falling back to DefaultColor. */
	UFUNCTION(BlueprintPure, Category = "Presentation")
	FLinearColor GetColorForCue(EJargonCombatCueType CueType) const;
};
