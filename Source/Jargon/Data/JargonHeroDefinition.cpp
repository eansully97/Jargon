#include "Data/JargonHeroDefinition.h"

void UJargonHeroDefinition::ApplyRecommendedClassPreset()
{
	switch (HeroClass)
	{
	case EJargonHeroClass::Paladin:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Paladin"));
		}
		MaxHP = 7;
		MoveRange = 3;
		AttackRange = 1;
		AttackDamage = 2;
		break;

	case EJargonHeroClass::Mage:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Mage"));
		}
		MaxHP = 4;
		MoveRange = 3;
		AttackRange = 3;
		AttackDamage = 1;
		break;

	case EJargonHeroClass::Rogue:
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(TEXT("Rogue"));
		}
		MaxHP = 5;
		MoveRange = 4;
		AttackRange = 1;
		AttackDamage = 2;
		break;

	default:
		MaxHP = 5;
		MoveRange = 3;
		AttackRange = 1;
		AttackDamage = 1;
		break;
	}
}

bool UJargonHeroDefinition::IsValidDefinition() const
{
	return MaxHP > 0
		&& MoveRange >= 0
		&& AttackRange > 0
		&& AttackDamage >= 0;
}
