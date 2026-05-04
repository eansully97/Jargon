// PlayerBattleUnit.cpp

#include "PlayerBattleUnit.h"

#include "Components/SkeletalMeshComponent.h"
#include "Data/JargonHeroDefinition.h"

APlayerBattleUnit::APlayerBattleUnit()
{
	Team = ETeam::Player;
	MaxHP = 5;
	CurrentHP = MaxHP;
	MoveRange = 3;
}

void APlayerBattleUnit::InitializeFromHeroDefinition(UJargonHeroDefinition* HeroDefinition)
{
	if (!HeroDefinition)
	{
		return;
	}

	AppliedHeroDefinition = HeroDefinition;
	SetBaseCombatStats(
		HeroDefinition->MaxHP,
		HeroDefinition->MoveRange,
		HeroDefinition->AttackRange,
		HeroDefinition->AttackDamage,
		true);

	if (HeroDefinition->HeroSkeletalMesh && UnitMesh)
	{
		UnitMesh->SetSkeletalMesh(HeroDefinition->HeroSkeletalMesh);
	}

	BP_OnHeroDefinitionApplied(HeroDefinition);
	InitializeIdlePresentation();
}
