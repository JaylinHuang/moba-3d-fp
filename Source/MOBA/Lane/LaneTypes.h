#pragma once

#include "CoreMinimal.h"
#include "LaneTypes.generated.h"

/** 兵线单位种类。英雄不是其中任何一种。 */
UENUM(BlueprintType)
enum class ELaneUnitKind : uint8
{
	MeleeMinion UMETA(DisplayName = "近战小兵"),
	RangedMinion UMETA(DisplayName = "远程小兵"),
	Monster UMETA(DisplayName = "野怪"),
	OuterTower UMETA(DisplayName = "外塔"),
	InnerTower UMETA(DisplayName = "内塔"),
	Base UMETA(DisplayName = "基地")
};

/**
 * 兵线第一版的占位数值。
 * 正式经济、攻速和攻击距离归战斗窗口，这里的数字只为了能跑起来。
 */
USTRUCT(BlueprintType)
struct FLanePlaceholderStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "占位")
	bool bPlaceholder = true;

	/** 占位刷兵间隔，单位秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float WaveInterval = 30.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	int32 MeleePerWave = 3;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	int32 RangedPerWave = 3;

	/** 占位移动速度，单位厘米/秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float MoveSpeed = 325.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float MeleeAttackRange = 180.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float RangedAttackRange = 520.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float MeleeAggroRadius = 600.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float RangedAggroRadius = 650.f;

	/** 占位出手间隔，单位秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.05"))
	float AttackInterval = 1.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float CallRadius = 750.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float TowerAttackRange = 800.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0.05"))
	float TowerAttackInterval = 1.f;

	/** 判定「己方小兵在附近」的占位半径。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float AlliedMinionRadius = 900.f;

	/** 附近有己方小兵时，英雄对塔的伤害系数。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float HeroDamageScaleWithMinions = 1.f;

	/** 附近没有己方小兵时，英雄对塔的占位降低系数。本模块不按它扣血。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float HeroDamageScaleWithoutMinions = 0.25f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float CampRadius = 650.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float CampRespawnSeconds = 45.f;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	int32 MonsterCount = 2;

	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float MonsterMoveSpeed = 300.f;
};

/** 外塔、内塔、基地会被小兵锁死。 */
inline bool LaneIsStructureKind(ELaneUnitKind Kind)
{
	return Kind == ELaneUnitKind::OuterTower
		|| Kind == ELaneUnitKind::InnerTower
		|| Kind == ELaneUnitKind::Base;
}
