// JargonTypes.h

#pragma once

#include "CoreMinimal.h"
#include "JargonTypes.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	Player UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy")
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
enum class ECardEffectType : uint8
{
	Damage UMETA(DisplayName = "Damage")
};

UENUM(BlueprintType)
enum class ECardTargetType : uint8
{
	Unit UMETA(DisplayName = "Unit")
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