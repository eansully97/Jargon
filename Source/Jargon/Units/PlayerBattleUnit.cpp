// PlayerBattleUnit.cpp

#include "Units/PlayerBattleUnit.h"

APlayerBattleUnit::APlayerBattleUnit()
{
	Team = ETeam::Player;
	MaxHP = 5;
	CurrentHP = MaxHP;
	MoveRange = 3;
}