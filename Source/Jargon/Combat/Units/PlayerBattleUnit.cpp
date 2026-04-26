// PlayerBattleUnit.cpp

#include "PlayerBattleUnit.h"

APlayerBattleUnit::APlayerBattleUnit()
{
	Team = ETeam::Player;
	MaxHP = 5;
	CurrentHP = MaxHP;
	MoveRange = 3;
}