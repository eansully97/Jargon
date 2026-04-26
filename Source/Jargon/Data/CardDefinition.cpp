// Fill out your copyright notice in the Description page of Project Settings.


#include "CardDefinition.h"

bool UCardDefinition::HasEffectOperation(ECardEffectOperation Operation) const
{
	for (const FCardEffectSpec& EffectSpec : Effects)
	{
		if (EffectSpec.Operation == Operation)
		{
			return true;
		}
	}

	return false;
}

ECardEffectOperation UCardDefinition::GetPrimaryEffectOperation() const
{
	return Effects.Num() > 0 ? Effects[0].Operation : ECardEffectOperation::None;
}

int32 UCardDefinition::GetConfiguredRangeForEffect(const FCardEffectSpec& EffectSpec) const
{
	return EffectSpec.RangeOverride >= 0 ? EffectSpec.RangeOverride : FMath::Max(0, Range);
}

int32 UCardDefinition::GetConfiguredRadiusForEffect(const FCardEffectSpec& EffectSpec) const
{
	return EffectSpec.RadiusOverride >= 0 ? EffectSpec.RadiusOverride : GetConfiguredAreaRadius();
}

int32 UCardDefinition::GetConfiguredValueForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.Value);
}

int32 UCardDefinition::GetConfiguredPushDistanceForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.PushDistance);
}

int32 UCardDefinition::GetConfiguredCollisionDamageForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.CollisionDamage);
}
