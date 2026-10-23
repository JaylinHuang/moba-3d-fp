#pragma once

#include "CoreMinimal.h"
#include "HeroCombatProfile.h"
#include "CombatPlaceholder.generated.h"

/** 固定商品。没有合成树。 */
UENUM()
enum class ECombatShopItem : uint8
{
	Boots,
	Attack,
	Health
};

/**
 * 战斗占位数值。
 * 具体生命、金币、攻速、距离、商品和复活秒数以后再调。
 */
USTRUCT(BlueprintType)
struct FCombatPlaceholderStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "占位")
	bool bPlaceholder = true;

	/** 占位生命。一次攻击打不死。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float Health = 400.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float Armor = 0.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float MagicResist = 0.f;

	/** 没有英雄配置的单位，出手用这一种。 */
	UPROPERTY(EditAnywhere, Category = "占位")
	ECombatDamageType OutgoingDamageType = ECombatDamageType::Physical;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float PhysicalDamage = 40.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float MagicDamage = 0.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float TrueDamage = 0.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float MinionGold = 40.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float HeroKillGold = 300.f;

	/** 占位自然金币，单位是每秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float PassiveGoldPerSecond = 2.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float MinionExperience = 30.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float HeroKillExperience = 80.f;

	/** 升一级所需的占位经验。等级从 1 起，经验累积到等级乘以该值就升级。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float ExperiencePerLevel = 100.f;

	/** 占位共享半径，单位厘米。用水平距离。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float ShareRadius = 1300.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float WarriorAttackRange = 200.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float MageAttackRange = 550.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float MarksmanAttackRange = 800.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.05"))
	float WarriorAttackInterval = 1.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.05"))
	float MageAttackInterval = 1.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.05"))
	float MarksmanAttackInterval = 1.f;

	/** 占位商品价格。没有合成。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float BootsCost = 300.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float BootsMoveSpeed = 60.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float AttackItemCost = 450.f;

	/** 攻击商品加到哪一种伤害上。不写死成物理。 */
	UPROPERTY(EditAnywhere, Category = "占位")
	ECombatDamageType AttackItemDamageType = ECombatDamageType::Physical;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float AttackItemBonus = 20.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float HealthItemCost = 400.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float HealthItemBonus = 150.f;

	/** 占位复活：基础秒数，再加每分钟增长。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.1"))
	float RespawnBaseSeconds = 6.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float RespawnGrowthPerMinute = 2.f;

	/** 占位泉水半径，厘米。从泉水位置量水平距离。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float FountainRadius = 800.f;

	/** 占位泉水回血，每秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float FountainHealPerSecond = 40.f;

	/** 占位回城读条，秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.1"))
	float RecallChannelSeconds = 8.f;

	/** 离开回城起点超过该水平距离则取消，厘米。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float RecallMoveTolerance = 80.f;
};
