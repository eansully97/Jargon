#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Core/JargonTypes.h"
#include "Engine/DataAsset.h"
#include "JargonTileEffectDefinition.generated.h"

class UTexture2D;
class UJargonAbilityDefinition;

/** Data Asset trigger that determines which runtime tile-effect hook executes TriggerAbility. */
UENUM(BlueprintType)
enum class EJargonTileEffectTrigger : uint8
{
	OnUnitEnter UMETA(DisplayName = "On Unit Enter"),
	OnPlayerTurnStart UMETA(DisplayName = "On Player Turn Start")
};

UCLASS(BlueprintType, meta = (DisplayName = "Jargon Tile Effect Definition"))
class JARGON_API UJargonTileEffectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (ToolTip = "Player-facing tile effect name for UI, logs, and future audit output."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect", meta = (MultiLine = "true", ToolTip = "Short authoring description for this trap or aura."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Visual", meta = (ToolTip = "Optional icon for future UI. Runtime mesh/VFX presentation belongs on the generic tile-effect Blueprint shell."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Runtime", meta = (ToolTip = "When this tile effect resolves its TriggerAbility. Trap definitions must use On Unit Enter; Aura definitions must use On Player Turn Start."))
	EJargonTileEffectTrigger Trigger = EJargonTileEffectTrigger::OnUnitEnter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Runtime", meta = (ToolTip = "Runtime category represented by this placed tile effect. Trap definitions use On Unit Enter; Aura definitions use On Player Turn Start."))
	ECardCategory TileEffectCategory = ECardCategory::Trap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Runtime", meta = (ClampMin = "0", ToolTip = "How many player turn starts this placed tile effect lasts. 0 means infinite."))
	int32 Duration = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Targeting", meta = (ClampMin = "0", ToolTip = "Default radius used by the runtime tile-effect actor for presentation and legacy behavior. This does not automatically populate Radius on entries in Effects; set per-effect Radius for shared effect AOE."))
	int32 EffectRadius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Runtime", meta = (ToolTip = "Whether this definition destroys its runtime tile-effect actor after successfully resolving on unit enter.", EditCondition = "Trigger == EJargonTileEffectTrigger::OnUnitEnter", EditConditionHides))
	bool bDestroyAfterUnitEnter = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Effect|Ability", meta = (ToolTip = "Reusable ability definition resolved when this tile effect triggers. Required for valid trap and aura definitions."))
	TObjectPtr<UJargonAbilityDefinition> TriggerAbility = nullptr;

	/** Checks static trap/aura authoring. Runtime duration and tile ownership live on ABattleTileEffect. */
	UFUNCTION(BlueprintPure, Category = "Tile Effect|Validation")
	bool IsValidDefinition() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
