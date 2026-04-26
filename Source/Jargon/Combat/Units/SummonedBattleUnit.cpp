// SummonedBattleUnit.cpp

#include "SummonedBattleUnit.h"

ASummonedBattleUnit::ASummonedBattleUnit()
{
	Team = ETeam::Player;
	MaxHP = 2;
	CurrentHP = MaxHP;
	MoveRange = 2;
	AttackRange = 1;
	AttackDamage = 1;
}
