// EnemyDummyUnit.cpp

#include "EnemyDummyUnit.h"

AEnemyDummyUnit::AEnemyDummyUnit()
{
	Team = ETeam::Enemy;
	MaxHP = 2;
	CurrentHP = MaxHP;
	MoveRange = 2;
	AttackRange = 1;
	AttackDamage = 1;
}