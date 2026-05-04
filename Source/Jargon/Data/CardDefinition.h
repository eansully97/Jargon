// CardDefinition.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Engine/DataAsset.h"
#include "CardDefinition.generated.h"

class ABattleTileEffect;
class ABattleUnit;
class UJargonCardScript;
class UJargonSummonedUnitDefinition;
class UJargonTileEffectDefinition;
class UTexture2D;
enum class EJargonEffectOperation : uint8;

UCLASS(BlueprintType)
class JARGON_API UCardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ToolTip = "Name shown on the card in UI and used by the art prompt generator."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (MultiLine = "true", ToolTip = "Short rules/flavor text shown on the card and included in generated art prompts."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0", ToolTip = "Normal Energy cost to play this card. Element charges are optional combo resources and do not replace this cost."))
	int32 Cost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ToolTip = "Broad card type used by UI, validation, prompt text, and tile-effect placement semantics."))
	ECardCategory Category = ECardCategory::Spell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (DisplayName = "Element", ToolTip = "Primary element this card interacts with, generates, spends, or represents. None is treated as Neutral for card authoring and deck construction."))
	EJargonElementType CardElement = EJargonElementType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ToolTip = "What kind of target the card asks the player to choose. Runtime effects come from CardScript effect lines."))
	ECardTargetType TargetType = ECardTargetType::Unit;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0", ToolTip = "Maximum targeting distance from the player unit for cards that target units or tiles."))
	int32 Range = 3;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Card|Effect Lines", meta = (ToolTip = "Primary card behavior using focused inline effect lines. Designers author Operation + Delivery + Payload here. Reusable card-game keywords live in payload definitions such as statuses, traits, and future modifiers."))
	TObjectPtr<UJargonCardScript> CardScript = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual", meta = (ToolTip = "Manually assigned card artwork. The prompt generator never creates, imports, or assigns this texture."))
	TObjectPtr<UTexture2D> CardArt = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "Explicit opt-in for the Generate Card Art Prompt editor button. This never creates, imports, or assigns art."))
	bool bGenerateArtPrompt = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Card|Generation", meta = (DisplayName = "Generate Card Art Prompt", ToolTip = "Writes a polished fantasy card art prompt to Saved/CardArtPrompts and logs the full prompt. CardArt remains manual. Requires bGenerateArtPrompt."))
	void GenerateCardArtPrompt() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "Explicit opt-in for the Generate Summon Unit Definition editor button. Keeps non-summon cards and accidental clicks from creating assets."))
	bool bGenerateSummonUnitDefinition = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "Content folder where generated summon definition Data Assets are created. Uses Unreal long package paths, for example /Game/Jargon/Data/SummonedUnits."))
	FString SummonDefinitionOutputFolder = TEXT("/Game/Jargon/Data/SummonedUnits");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "When true, the generated summon definition is assigned back to the selected summon keyword. Runtime Blueprint class is still authored separately on the card keyword."))
	bool bAssignGeneratedSummonDefinition = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (AdvancedDisplay, ToolTip = "When true, the button may reuse an existing matching summon definition asset or replace this card's existing definition reference. Existing definition asset fields are not overwritten."))
	bool bReplaceExistingSummonDefinition = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Card|Generation", meta = (DisplayName = "Generate Summon Unit Definition", ToolTip = "Creates a matching UJargonSummonedUnitDefinition asset for one summon keyword and optionally assigns it back to this card. Editor-only; never overwrites by default. Requires bGenerateSummonUnitDefinition."))
	void GenerateSummonUnitDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "Explicit opt-in for the Generate Tile Effect Definition editor button. Keeps non-trap/aura cards and accidental clicks from creating assets."))
	bool bGenerateTileEffectDefinition = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "Content folder where generated tile effect definition Data Assets are created. Uses Unreal long package paths, for example /Game/Jargon/Data/TileEffects."))
	FString TileEffectDefinitionOutputFolder = TEXT("/Game/Jargon/Data/TileEffects");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (ToolTip = "When true, the generated tile effect definition is assigned back to the selected trap/aura keyword. Runtime Blueprint class is still authored separately on the card keyword."))
	bool bAssignGeneratedTileEffectDefinition = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Generation", meta = (AdvancedDisplay, ToolTip = "When true, the button may reuse an existing matching tile effect definition asset or replace this card's existing definition reference. Existing definition asset fields are not overwritten."))
	bool bReplaceExistingTileEffectDefinition = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Card|Generation", meta = (DisplayName = "Generate Tile Effect Definition", ToolTip = "Creates a matching UJargonTileEffectDefinition asset for one trap/aura keyword and optionally assigns it back to this card. Editor-only; never overwrites by default. Requires bGenerateTileEffectDefinition."))
	void GenerateTileEffectDefinition();

	UFUNCTION(BlueprintPure, Category = "Card")
	bool UsesBoardTileTargeting() const
	{
		return TargetType != ECardTargetType::Self;
	}

	/** Targeting helper for UI/highlights; gameplay still resolves through CardScript effect specs. */
	UFUNCTION(BlueprintPure, Category = "Card")
	bool RequiresUnitOnTargetTile() const
	{
		return TargetType == ECardTargetType::Unit;
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool CreatesPersistentTileEffect() const
	{
		return HasEffectOperation(EJargonEffectOperation::PlaceTileEffect);
	}

	UFUNCTION(BlueprintPure, Category = "Card")
	bool RequiresEmptyTargetTile() const
	{
		return HasEffectOperation(EJargonEffectOperation::SummonUnit)
			|| HasEffectOperation(EJargonEffectOperation::PlaceTileEffect)
			|| HasEffectOperation(EJargonEffectOperation::MoveSource);
	}

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool UsesCardScript() const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool UsesEffectSpecs() const
	{
		return UsesCardScript();
	}

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	bool HasEffectOperation(EJargonEffectOperation Operation) const;

	/** Builds the base Operation + Delivery + Filter + Payload specs from the inline CardScript. */
	bool BuildBaseEffectSpecs(TArray<FJargonEffectSpec>& OutEffects) const;

	/** Builds one manually chosen elemental bonus group into shared effect specs. */
	bool BuildElementalBonusEffectSpecs(int32 BonusIndex, TArray<FJargonEffectSpec>& OutEffects) const;

	/** Number of optional elemental bonus groups offered to the manual bonus-choice widget. */
	int32 GetElementalBonusScriptCount() const;

	/** Lightweight runtime/editor validity check. Full editor validation reports richer asset authoring issues. */
	UFUNCTION(BlueprintPure, Category = "Card")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Card|Audit")
	FString GetEffectAuditSummary(const FJargonEffectSpec& EffectSpec) const;

	UFUNCTION(BlueprintPure, Category = "Card|Audit")
	FString GetAuditSummary() const;

	UFUNCTION(BlueprintPure, Category = "Card|Effects")
	EJargonEffectOperation GetPrimaryEffectOperation() const;

	UFUNCTION(BlueprintPure, Category = "Card")
	FText GetCardElementDisplayText() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
