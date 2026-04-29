#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardCreationTemplateTool.generated.h"

class UCardDefinition;

UENUM(BlueprintType)
enum class ECardCreationTemplateType : uint8
{
	DamageSpell UMETA(DisplayName = "Damage Spell"),
	AoEDamageSpell UMETA(DisplayName = "AoE Damage Spell"),
	StunSpell UMETA(DisplayName = "Stun Spell"),
	ChainSpell UMETA(DisplayName = "Chain Spell"),
	MovementSpell UMETA(DisplayName = "Movement Spell"),
	SummonCard UMETA(DisplayName = "Summon Card"),
	TrapCard UMETA(DisplayName = "Trap Card"),
	AuraCard UMETA(DisplayName = "Aura Card"),
	UtilityCard UMETA(DisplayName = "Utility Card")
};

/**
 * Editor-facing helper for creating UCardDefinition Data Assets from common templates.
 *
 * This tool intentionally does not generate, import, or assign card art.
 * It creates the card, seeds template fields/effects, writes an image prompt,
 * optionally copies that prompt to clipboard, and leaves CardArt blank for manual setup.
 */
UCLASS(BlueprintType)
class JARGON_API UCardCreationTemplateTool : public UDataAsset
{
	GENERATED_BODY()

public:
	UCardCreationTemplateTool();

	/** Main editor workflow. Creates a card asset and optionally writes/copies an art prompt. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Card Creation", meta = (DisplayName = "Create Card"))
	void CreateCard();

	/** Compatibility wrapper for older calls. Prefer CreateCard. */
	UFUNCTION(BlueprintCallable, Category = "Debug|Manual")
	void CreateCardFromTemplate();

	/** Builds the prompt string for a card without writing it to disk. */
	UFUNCTION(BlueprintCallable, Category = "Card Art|Prompt")
	FString BuildCardArtPrompt(UCardDefinition* Card) const;

	/** Writes a prompt file for CardToGeneratePromptFor. Useful for existing cards. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Debug|Manual", meta = (DisplayName = "Generate Prompt For Configured Card"))
	void GeneratePromptForConfiguredCard();

protected:
#if WITH_EDITOR
	UCardDefinition* CreateCardAssetFromTemplate(FString& OutObjectPath);
	bool GenerateArtPromptForCard(UCardDefinition* Card);
	void LogCreatedCardValidation(UCardDefinition* Card, const FString& ObjectPath) const;
	void SyncEditorToCreatedAsset(UCardDefinition* Card) const;
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Creation")
	ECardCreationTemplateType TemplateType = ECardCreationTemplateType::DamageSpell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Creation", meta = (ToolTip = "Package path where the new card asset will be created. Example: /Game/Jargon/Data/Cards/Spells"))
	FString OutputFolder = TEXT("/Game/Jargon/Data/Cards");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Creation", meta = (ToolTip = "New asset name. DA_Card_ is added automatically if omitted. Existing assets are never overwritten."))
	FString NewCardAssetName = TEXT("DA_Card_NewCard");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Creation", meta = (ToolTip = "Optional display name. If empty, a display name is derived from the asset name."))
	FText DisplayNameOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Creation", meta = (MultiLine = "true", ToolTip = "Optional description. If empty, a template placeholder description is used."))
	FText DescriptionOverride;

	UPROPERTY(EditAnywhere, Category = "Card Creation|Overrides", AdvancedDisplay)
	bool bOverrideCost = false;

	UPROPERTY(EditAnywhere, Category = "Card Creation|Overrides", AdvancedDisplay, meta = (ClampMin = "0", EditCondition = "bOverrideCost", EditConditionHides))
	int32 CostOverride = 1;

	UPROPERTY(EditAnywhere, Category = "Card Creation|Overrides", AdvancedDisplay)
	bool bOverrideRange = false;

	UPROPERTY(EditAnywhere, Category = "Card Creation|Overrides", AdvancedDisplay, meta = (ClampMin = "0", EditCondition = "bOverrideRange", EditConditionHides))
	int32 RangeOverride = 3;

	UPROPERTY(EditAnywhere, Category = "Card Art|Prompt", meta = (ToolTip = "If enabled, Create Card writes a reusable prompt to Saved/CardArtPrompts. This does not create or assign art."))
	bool bCreatePrompt = true;

	UPROPERTY(EditAnywhere, Category = "Card Art|Prompt", meta = (ToolTip = "Copies the generated prompt to the clipboard after saving it."))
	bool bCopyPromptToClipboard = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt", meta = (MultiLine = "true"))
	FString CardArtPromptStyle;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt")
	FString SubjectOverride;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt")
	FString MoodOverride;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt", meta = (MultiLine = "true"))
	FString ExtraPromptNotes;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt", meta = (MultiLine = "true"))
	FString NegativePromptNotes;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt", meta = (ClampMin = "1"))
	int32 ArtWidth = 720;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt", meta = (ClampMin = "1"))
	int32 ArtHeight = 1280;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Card Art|Prompt", meta = (ToolTip = "Subfolder under Project/Saved, or an absolute path, where prompt files are written."))
	FString PromptOutputSubdirectory = TEXT("CardArtPrompts");

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Editor")
	bool bSyncContentBrowserToCreatedAsset = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Editor")
	bool bOpenCreatedAssetEditor = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Debug|Manual")
	TObjectPtr<UCardDefinition> CardToGeneratePromptFor = nullptr;

	UPROPERTY(VisibleInstanceOnly, Transient, AdvancedDisplay, BlueprintReadOnly, Category = "Debug|Manual")
	TObjectPtr<UCardDefinition> LastCreatedCard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Transient, AdvancedDisplay, BlueprintReadOnly, Category = "Debug|Manual")
	FString LastGeneratedPromptFilePath;
};