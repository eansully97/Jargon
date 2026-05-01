// JargonTypes.h

#pragma once

#include "CoreMinimal.h"
#include "JargonTypes.generated.h"

USTRUCT(BlueprintType)
struct JARGON_API FHexCoord
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex")
	int32 Q = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex")
	int32 R = 0;

	FHexCoord() = default;

	FHexCoord(int32 InQ, int32 InR)
		: Q(InQ)
		, R(InR)
	{
	}

	bool operator==(const FHexCoord& Other) const
	{
		return Q == Other.Q && R == Other.R;
	}

	bool operator!=(const FHexCoord& Other) const
	{
		return !(*this == Other);
	}

	FHexCoord operator+(const FHexCoord& Other) const
	{
		return FHexCoord(Q + Other.Q, R + Other.R);
	}

	FHexCoord operator-(const FHexCoord& Other) const
	{
		return FHexCoord(Q - Other.Q, R - Other.R);
	}

	FHexCoord operator*(int32 Scalar) const
	{
		return FHexCoord(Q * Scalar, R * Scalar);
	}

	int32 GetS() const
	{
		return -Q - R;
	}

	bool IsZero() const
	{
		return Q == 0 && R == 0;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(%d, %d)"), Q, R);
	}
};

FORCEINLINE uint32 GetTypeHash(const FHexCoord& Coord)
{
	return HashCombine(::GetTypeHash(Coord.Q), ::GetTypeHash(Coord.R));
}

UENUM(BlueprintType)
enum class ETeam : uint8
{
	Player UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy")
};

UENUM(BlueprintType)
enum class EJargonElementType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Fire = 1 UMETA(DisplayName = "Fire"),
	Storm = 2 UMETA(DisplayName = "Storm"),
	Nature = 3 UMETA(DisplayName = "Nature"),
	Radiance = 4 UMETA(DisplayName = "Radiance"),
	Quietus = 5 UMETA(DisplayName = "Quietus"),
	Frost = 6 UMETA(DisplayName = "Frost")
};

UENUM(BlueprintType)
enum class ETileHighlightState : uint8
{
	None      UMETA(DisplayName = "None"),
	Reachable UMETA(DisplayName = "Reachable"),
	Blocked   UMETA(DisplayName = "Blocked"),
	Occupied  UMETA(DisplayName = "Occupied"),
	Selected  UMETA(DisplayName = "Selected")
};

UENUM(BlueprintType)
enum class ECardCategory : uint8
{
	Spell  UMETA(DisplayName = "Spell"),
	Summon UMETA(DisplayName = "Summon"),
	Trap   UMETA(DisplayName = "Trap"),
	Aura   UMETA(DisplayName = "Aura")
};

UENUM(BlueprintType)
enum class ECardTargetType : uint8
{
	Unit UMETA(DisplayName = "Unit"),
	Tile UMETA(DisplayName = "Tile"),
	Self UMETA(DisplayName = "Self")
};

UENUM(BlueprintType)
enum class ECombatPhase : uint8
{
	BattleStart UMETA(DisplayName = "Battle Start"),
	PlayerTurn  UMETA(DisplayName = "Player Turn"),
	EnemyTurn   UMETA(DisplayName = "Enemy Turn"),
	Resolving   UMETA(DisplayName = "Resolving"),
	Victory     UMETA(DisplayName = "Victory"),
	Defeat      UMETA(DisplayName = "Defeat")
};
