#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "JargonHeroTypes.generated.h"

class UJargonHeroDefinition;

UENUM(BlueprintType)
enum class EJargonHeroClass : uint8
{
	None UMETA(DisplayName = "None"),
	Mage UMETA(DisplayName = "Mage"),
	Rogue UMETA(DisplayName = "Rogue"),
	Paladin UMETA(DisplayName = "Paladin")
};

UENUM(BlueprintType)
enum class EJargonHeroAspect : uint8
{
	None UMETA(DisplayName = "None"),

	// Legacy/compatibility aspect. Runtime aspect selection no longer assigns this directly.
	RadiantChampion UMETA(DisplayName = "Radiant Champion"),

	// Mage aspects: Mage + element threshold determines the active specialization.
	Pyromancer UMETA(DisplayName = "Mage / Fire - Pyromancer"),
	Cryomancer UMETA(DisplayName = "Mage / Frost - Cryomancer"),
	Stormcaller UMETA(DisplayName = "Mage / Storm - Stormcaller"),
	Wildheart UMETA(DisplayName = "Mage / Nature - Wildheart"),
	Lightweaver UMETA(DisplayName = "Mage / Radiance - Lightweaver"),
	Necromancer UMETA(DisplayName = "Mage / Quietus - Necromancer"),

	// Rogue aspects: Rogue + element threshold determines the active specialization.
	Ashblade UMETA(DisplayName = "Rogue / Fire - Ashblade"),
	Frostknife UMETA(DisplayName = "Rogue / Frost - Frostknife"),
	Tempest UMETA(DisplayName = "Rogue / Storm - Tempest"),
	Venomshade UMETA(DisplayName = "Rogue / Nature - Venomshade"),
	Inquisitor UMETA(DisplayName = "Rogue / Radiance - Inquisitor"),
	Reaper UMETA(DisplayName = "Rogue / Quietus - Reaper"),

	// Paladin aspects: Paladin + element threshold determines the active specialization.
	Sunbreaker UMETA(DisplayName = "Paladin / Fire - Sunbreaker"),
	Frostwarden UMETA(DisplayName = "Paladin / Frost - Frostwarden"),
	Stormguard UMETA(DisplayName = "Paladin / Storm - Stormguard"),
	Oathwarden UMETA(DisplayName = "Paladin / Nature - Oathwarden"),
	Templar UMETA(DisplayName = "Paladin / Radiance - Templar"),
	Graveknight UMETA(DisplayName = "Paladin / Quietus - Graveknight")
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonHeroClassInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	EJargonHeroClass HeroClass = EJargonHeroClass::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	bool bHasCombatStartPassive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	FText CombatStartPassiveName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	bool bHasPlayerTurnStartPassive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	FText PlayerTurnStartPassiveName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Class")
	bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonHeroAspectInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	EJargonHeroClass HeroClass = EJargonHeroClass::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	EJargonElementType ElementType = EJargonElementType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	EJargonElementType RequiredElement = EJargonElementType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	int32 RequiredElementCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	int32 RequiredCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	int32 CurrentElementCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	int32 CurrentCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	EJargonHeroAspect Aspect = EJargonHeroAspect::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	FText PassiveName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	FText PassiveDescription;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	bool bHasRequiredCharges = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	bool bRequirementMet = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct JARGON_API FJargonHeroRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero")
	TObjectPtr<UJargonHeroDefinition> BaseHeroDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	EJargonElementType DominantElement = EJargonElementType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	bool bHasElementInfluence = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	EJargonHeroAspect ActiveAspect = EJargonHeroAspect::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	bool bHasActiveAspect = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Aspect")
	int32 AspectThreshold = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	int32 FireCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	int32 FrostCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	int32 StormCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	int32 NatureCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	int32 RadianceCharges = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero|Elements")
	int32 QuietusCharges = 0;

	int32 GetChargesForElement(EJargonElementType Element) const
	{
		switch (Element)
		{
		case EJargonElementType::Fire:
			return FireCharges;
		case EJargonElementType::Frost:
			return FrostCharges;
		case EJargonElementType::Storm:
			return StormCharges;
		case EJargonElementType::Nature:
			return NatureCharges;
		case EJargonElementType::Radiance:
			return RadianceCharges;
		case EJargonElementType::Quietus:
			return QuietusCharges;
		default:
			return 0;
		}
	}
};
