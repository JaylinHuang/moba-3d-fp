#pragma once

#include "CoreMinimal.h"
#include "HeroCombatProfile.h"

class AActor;
class UWorld;

/** 忽略高度的距离。塔的进出范围和小兵选目标都用它。 */
float LaneHorizontalDistance(const FVector& A, const FVector& B);

/** 实现了可攻击接口、又不是兵线单位的，视为英雄。测试假人也会落入这一档。 */
bool LaneIsHero(const AActor* Actor);

bool LaneIsAlive(const AActor* Actor);

int32 LaneTeamId(const AActor* Actor);

bool LaneIsEnemyHero(const AActor* Viewer, const AActor* Actor);

bool LaneIsAlliedHero(const AActor* Viewer, const AActor* Actor);

/** 只问现有查询，不读草丛体积。 */
bool LaneIsVisibleTo(const AActor* Viewer, const AActor* Target);

/** 现有近战挥击和飞行物都先当作占位单体。 */
bool LaneIsPlaceholderSingleTarget(EAttackDelivery Delivery);

/** 提交伤害请求。不扣生命。广播尚未登记时，这里不会写入兵线伤害流水。 */
void LaneSubmitDamage(UWorld* World, AActor* Source, AActor* Target, EAttackDelivery Delivery);
