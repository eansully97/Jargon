#pragma once

#include "CoreMinimal.h"
#include "Core/JargonHeroTypes.h"
#include "Engine/DataAsset.h"
#include "JargonHeroDefinition.generated.h"

class USkeletalMesh;
class UTexture2D;

/**
 * Base hero identity and class tuning.
 *
 * Elements are intentionally not authored here. Combat element charges produce
 * a transient runtime hero state so class identity and elemental influence stay separate.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Jargon Hero Definition"))
class JARGON_API UJargonHeroDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (DisplayPriority = "1", ToolTip = "Player-facing hero name shown in UI. This does not change gameplay by itself."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (DisplayPriority = "2", ToolTip = "Simple RPG class identity for this hero. Current supported classes are Mage, Rogue, and Paladin. Legacy hidden classes are mapped safely if old assets still use them."))
	EJargonHeroClass HeroClass = EJargonHeroClass::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero", meta = (DisplayName = "Skeletal Mesh Override", DisplayPriority = "3", ToolTip = "Optional single mesh override used by both the Town/Exploration character and combat player unit. Leave empty to keep Blueprint defaults."))
	TObjectPtr<USkeletalMesh> HeroSkeletalMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "1", DisplayName = "Max HP", DisplayPriority = "1", ToolTip = "Player hero maximum health in combat. This is the authoritative player stat and overrides the player battle unit Blueprint default."))
	int32 MaxHP = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "0", DisplayPriority = "2", ToolTip = "Player hero movement range in combat tiles. This is applied to the player battle unit at combat start."))
	int32 MoveRange = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "1", DisplayPriority = "3", ToolTip = "Player hero basic attack range in combat tiles. This is applied to the player battle unit at combat start."))
	int32 AttackRange = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Combat", meta = (ClampMin = "0", DisplayPriority = "4", ToolTip = "Player hero basic attack damage. This is applied to the player battle unit at combat start."))
	int32 AttackDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Optional UI", meta = (MultiLine = "true", AdvancedDisplay, ToolTip = "Optional flavor text for menus or future hero selection UI. It does not affect gameplay."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero|Optional UI", meta = (AdvancedDisplay, ToolTip = "Optional portrait for future UI. It is not used by combat yet."))
	TObjectPtr<UTexture2D> Portrait = nullptr;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hero|Preset", meta = (ToolTip = "Seeds recommended stats for the selected Hero Class. This is only an authoring shortcut; you can freely tune the values afterward."))
	void ApplyRecommendedClassPreset();

	UFUNCTION(BlueprintPure, Category = "Hero|Validation", meta = (ToolTip = "Returns true when required combat stat values are usable. This is a backend validation helper, not an editor workflow requirement."))
	bool IsValidDefinition() const;
};
