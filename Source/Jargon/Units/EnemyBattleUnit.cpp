// EnemyBattleUnit.cpp

#include "Units/EnemyBattleUnit.h"

AEnemyBattleUnit::AEnemyBattleUnit()
{
	Team = ETeam::Enemy;
	MaxHP = 3;
	CurrentHP = MaxHP;
	MoveRange = 2;
	AttackRange = 1;
	AttackDamage = 1;
	KillCurrencyReward.Copper = 2;
}
