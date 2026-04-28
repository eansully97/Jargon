// Fill out your copyright notice in the Description page of Project Settings.


#include "CardDefinition.h"

#include "Combat/Units/BattleUnit.h"

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

bool UCardDefinition::IsValidDefinition() const
{
	if (DisplayName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: DisplayName is empty."), *GetNameSafe(this));
		return false;
	}

	if (Cost < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: Cost is negative."), *GetNameSafe(this));
		return false;
	}

	if (Range < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: Range is negative."), *GetNameSafe(this));
		return false;
	}

	if (Effects.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: no Effects authored."), *GetNameSafe(this));
		return false;
	}

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		const FCardEffectSpec& EffectSpec = Effects[EffectIndex];

		if (EffectSpec.Operation == ECardEffectOperation::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: effect %d has operation None."),
				*GetNameSafe(this),
				EffectIndex);
			return false;
		}

		switch (EffectSpec.Operation)
		{
		case ECardEffectOperation::DealDamage:
		case ECardEffectOperation::Heal:
		case ECardEffectOperation::ApplyShield:
		case ECardEffectOperation::ApplyStun:
		case ECardEffectOperation::DrawCards:
		case ECardEffectOperation::GainEnergy:
		case ECardEffectOperation::ChainDamage:
		case ECardEffectOperation::ChainHeal:
		case ECardEffectOperation::ChainStun:
			if (EffectSpec.Value <= 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: effect %d requires Value > 0."),
					*GetNameSafe(this),
					EffectIndex);
				return false;
			}
			break;

		case ECardEffectOperation::MoveSelf:
			if (EffectSpec.MoveDistance <= 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: MoveSelf effect %d requires MoveDistance > 0."),
					*GetNameSafe(this),
					EffectIndex);
				return false;
			}
			break;

		case ECardEffectOperation::PushTarget:
			if (EffectSpec.PushDistance <= 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: PushTarget effect %d requires PushDistance > 0."),
					*GetNameSafe(this),
					EffectIndex);
				return false;
			}
			break;

		case ECardEffectOperation::PullTarget:
			if (EffectSpec.PullDistance <= 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: PullTarget effect %d requires PullDistance > 0."),
					*GetNameSafe(this),
					EffectIndex);
				return false;
			}
			break;

		case ECardEffectOperation::SummonUnit:
			if (!EffectSpec.UnitClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: SummonUnit effect %d has no UnitClass."),
					*GetNameSafe(this),
					EffectIndex);
				return false;
			}
			break;

		case ECardEffectOperation::PlaceTileEffect:
			if (!EffectSpec.TileEffectClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("CardDefinition '%s' invalid: PlaceTileEffect effect %d has no TileEffectClass."),
					*GetNameSafe(this),
					EffectIndex);
				return false;
			}
			break;

		default:
			break;
		}
	}

	return true;
}

ECardEffectOperation UCardDefinition::GetPrimaryEffectOperation() const
{
	return Effects.Num() > 0 ? Effects[0].Operation : ECardEffectOperation::None;
}

int32 UCardDefinition::GetConfiguredRangeForEffect(const FCardEffectSpec& EffectSpec) const
{
	if (EffectSpec.Operation == ECardEffectOperation::MoveSelf)
	{
		return FMath::Max(0, EffectSpec.MoveDistance);
	}

	return FMath::Max(0, Range);
}

int32 UCardDefinition::GetConfiguredRadiusForEffect(const FCardEffectSpec& EffectSpec) const
{
	return FMath::Max(0, EffectSpec.EffectRadius);
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
