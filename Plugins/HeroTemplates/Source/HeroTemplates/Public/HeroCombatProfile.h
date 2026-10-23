#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroCombatProfile.generated.h"

/** 出手形式。距离不由这个枚举决定。 */
UENUM(BlueprintType)
enum class EAttackDelivery : uint8
{
	Melee UMETA(DisplayName = "近身挥击"),
	Projectile UMETA(DisplayName = "飞行物")
};

/** 技能出手形式。这一版只有这三种，没有地面画圈。 */
UENUM(BlueprintType)
enum class EHeroSkillForm : uint8
{
	HitScan UMETA(DisplayName = "命中扫描"),
	Projectile UMETA(DisplayName = "飞行物"),
	SelfBuff UMETA(DisplayName = "自身增益")
};

/** 一个技能槽的占位配置。不包含技能点，升级不靠这里发放。 */
USTRUCT(BlueprintType)
struct FHeroSkillSlot
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "技能")
	EHeroSkillForm Form = EHeroSkillForm::HitScan;

	/** 扫描长度或飞行物最远距离，厘米。自身增益不使用。 */
	UPROPERTY(EditDefaultsOnly, Category = "技能", meta = (ClampMin = "0"))
	float Range = 400.f;

	/** 释放后的冷却，秒。 */
	UPROPERTY(EditDefaultsOnly, Category = "技能", meta = (ClampMin = "0"))
	float Cooldown = 6.f;

	/** 飞行物速度，厘米/秒。另外两种形式不使用。 */
	UPROPERTY(EditDefaultsOnly, Category = "技能", meta = (ClampMin = "0"))
	float ProjectileSpeed = 1400.f;

	/** 自身增益持续时间，秒。 */
	UPROPERTY(EditDefaultsOnly, Category = "技能", meta = (ClampMin = "0"))
	float BuffDuration = 0.f;

	/** 自身增益的占位移速加成。不写进伤害公式。 */
	UPROPERTY(EditDefaultsOnly, Category = "技能", meta = (ClampMin = "0"))
	float BuffMoveSpeed = 0.f;
};

/** 一次出手使用的伤害类型。不按职业写死，由这份配置决定。 */
UENUM(BlueprintType)
enum class ECombatDamageType : uint8
{
	Physical UMETA(DisplayName = "物理"),
	Magic UMETA(DisplayName = "魔法"),
	TrueDamage UMETA(DisplayName = "真实")
};

/**
 * 一名英雄的战斗配置。
 * 战斗模块落地前，攻速和攻击距离是占位值。
 */
UCLASS(BlueprintType)
class HEROTEMPLATES_API UHeroCombatProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 为真时，下面的攻速和攻击距离只是占位，不是正式数值。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位")
	bool bPlaceholderStats = true;

	/** 占位攻击距离，单位是厘米。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0"))
	float PlaceholderAttackRange = 200.f;

	/** 占位出手间隔，单位是秒。攻速用间隔表示。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0.05"))
	float PlaceholderAttackInterval = 1.f;

	/** 占位前摇时长，单位是秒。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0"))
	float PlaceholderWindupDuration = 0.3f;

	/** 准心选择目标的最大夹角，不代替攻击距离。 */
	UPROPERTY(EditDefaultsOnly, Category = "普攻", meta = (ClampMin = "0"))
	float SelectionAngleDegrees = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "普攻")
	EAttackDelivery Delivery = EAttackDelivery::Melee;

	/** 飞行物占位速度，厘米/秒。近身挥击不使用。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0"))
	float PlaceholderProjectileSpeed = 1800.f;

	/** 这次出手用哪一种伤害。三名模板可以改，结算不看职业。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位")
	ECombatDamageType OutgoingDamageType = ECombatDamageType::Physical;

	/** 占位物理伤害。只有出手类型是物理时才参与结算。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0"))
	float PhysicalDamage = 40.f;

	/** 占位魔法伤害。只有出手类型是魔法时才参与结算。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0"))
	float MagicDamage = 0.f;

	/** 占位真实伤害。按原值计入，不减护甲和魔抗。 */
	UPROPERTY(EditDefaultsOnly, Category = "占位", meta = (ClampMin = "0"))
	float TrueDamage = 0.f;

	/** 四个技能槽。伤害数值仍走战斗结算，这里不另写公式。 */
	UPROPERTY(EditDefaultsOnly, Category = "技能")
	TArray<FHeroSkillSlot> SkillSlots;
};

UCLASS()
class HEROTEMPLATES_API UWarriorHeroProfile : public UHeroCombatProfile
{
	GENERATED_BODY()

public:
	UWarriorHeroProfile();
};

UCLASS()
class HEROTEMPLATES_API UMageHeroProfile : public UHeroCombatProfile
{
	GENERATED_BODY()

public:
	UMageHeroProfile();
};

UCLASS()
class HEROTEMPLATES_API UMarksmanHeroProfile : public UHeroCombatProfile
{
	GENERATED_BODY()

public:
	UMarksmanHeroProfile();
};
