#include "HeroCombatProfile.h"

namespace
{
	FHeroSkillSlot MakeHitScan(float Range, float Cooldown)
	{
		FHeroSkillSlot Slot;
		Slot.Form = EHeroSkillForm::HitScan;
		Slot.Range = Range;
		Slot.Cooldown = Cooldown;
		return Slot;
	}

	FHeroSkillSlot MakeProjectile(float Range, float Speed, float Cooldown)
	{
		FHeroSkillSlot Slot;
		Slot.Form = EHeroSkillForm::Projectile;
		Slot.Range = Range;
		Slot.ProjectileSpeed = Speed;
		Slot.Cooldown = Cooldown;
		return Slot;
	}

	FHeroSkillSlot MakeSelfBuff(float Duration, float MoveSpeed, float Cooldown)
	{
		FHeroSkillSlot Slot;
		Slot.Form = EHeroSkillForm::SelfBuff;
		Slot.BuffDuration = Duration;
		Slot.BuffMoveSpeed = MoveSpeed;
		Slot.Cooldown = Cooldown;
		return Slot;
	}
}

UWarriorHeroProfile::UWarriorHeroProfile()
{
	bPlaceholderStats = true;
	PlaceholderAttackRange = 200.f;
	PlaceholderAttackInterval = 1.f;
	PlaceholderWindupDuration = 0.3f;
	SelectionAngleDegrees = 10.f;
	Delivery = EAttackDelivery::Melee;
	PlaceholderProjectileSpeed = 0.f;
	SkillSlots = {
		MakeHitScan(350.f, 5.f),
		MakeProjectile(700.f, 1200.f, 8.f),
		MakeSelfBuff(4.f, 120.f, 14.f),
		MakeHitScan(450.f, 18.f)
	};
}

UMageHeroProfile::UMageHeroProfile()
{
	bPlaceholderStats = true;
	PlaceholderAttackRange = 550.f;
	PlaceholderAttackInterval = 1.f;
	PlaceholderWindupDuration = 0.3f;
	SelectionAngleDegrees = 10.f;
	Delivery = EAttackDelivery::Projectile;
	PlaceholderProjectileSpeed = 1600.f;
	SkillSlots = {
		MakeHitScan(650.f, 6.f),
		MakeProjectile(1000.f, 1600.f, 7.f),
		MakeSelfBuff(3.f, 80.f, 12.f),
		MakeProjectile(1100.f, 1800.f, 16.f)
	};
}

UMarksmanHeroProfile::UMarksmanHeroProfile()
{
	bPlaceholderStats = true;
	PlaceholderAttackRange = 800.f;
	PlaceholderAttackInterval = 1.f;
	PlaceholderWindupDuration = 0.25f;
	SelectionAngleDegrees = 10.f;
	Delivery = EAttackDelivery::Projectile;
	PlaceholderProjectileSpeed = 2200.f;
	SkillSlots = {
		MakeHitScan(900.f, 5.f),
		MakeProjectile(1200.f, 2200.f, 7.f),
		MakeSelfBuff(3.f, 100.f, 12.f),
		MakeHitScan(1000.f, 16.f)
	};
}
