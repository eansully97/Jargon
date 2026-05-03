#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JargonStatusEffectDefinition.generated.h"

UENUM(BlueprintType)
enum class EJargonStatusEffectKind : uint8
{
	None UMETA(DisplayName = "None"),
	Stun UMETA(DisplayName = "Stun"),
	Freeze UMETA(DisplayName = "Freeze"),
	Burn UMETA(DisplayName = "Burn"),
	Root UMETA(DisplayName = "Root"),
	Vulnerable UMETA(DisplayName = "Vulnerable"),
	Regen UMETA(DisplayName = "Regen"),
	Weak UMETA(DisplayName = "Weak")
};

UENUM(BlueprintType)
enum class EJargonStatusEffectIntent : uint8
{
	Hostile UMETA(DisplayName = "Hostile"),
	Friendly UMETA(DisplayName = "Friendly"),
	Any UMETA(DisplayName = "Any")
};

/**
 * Data-driven authoring definition for built-in status effects.
 * Runtime behavior still dispatches to ABattleUnit's built-in status functions.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Status Effect Definition"))
class JARGON_API UJargonStatusEffectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UJargonStatusEffectDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (ToolTip = "Player-facing status name for UI, audit output, and future CardScript status actions."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (MultiLine = "true", ToolTip = "Short authoring description of what this status represents."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (MultiLine = "true", ToolTip = "Rules-first text for designers, such as 'At turn start, take damage equal to Burn, then reduce Burn by 1.'"))
	FText RulesText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (ToolTip = "Built-in runtime status this definition applies. None is invalid."))
	EJargonStatusEffectKind StatusKind = EJargonStatusEffectKind::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (ToolTip = "Plain-language label for what the authored numeric value means, such as turns, stacks, or next-hit bonus damage."))
	FText ValueLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (ToolTip = "Default targeting intent for validation/audit language. This does not drive runtime behavior yet."))
	EJargonStatusEffectIntent TargetIntent = EJargonStatusEffectIntent::Hostile;

	UFUNCTION(BlueprintPure, Category = "Status|Validation")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Status|Debug")
	FString GetAuditSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
