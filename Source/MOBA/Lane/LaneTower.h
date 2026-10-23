#pragma once

#include "CoreMinimal.h"
#include "Lane/LaneTypes.h"
#include "Lane/LaneUnit.h"
#include "LaneTower.generated.h"

class ALaneMinion;

/** 外塔或内塔。英雄锁定按这一拍收齐后再选离塔最近的。 */
UCLASS()
class MOBA_API ALaneTower : public ALaneUnit
{
	GENERATED_BODY()

public:
	ALaneTower();

	void Setup(const FLanePlaceholderStats& InStats, int32 InTeamId, ELaneUnitKind InKind);

	/** 附近没有己方小兵时返回占位降低系数。不扣生命。 */
	float GetIncomingHeroDamageScale() const;
	bool HasAlliedMinionNearby() const;

	virtual void Tick(float DeltaSeconds) override;

private:
	bool IsLockStillValid() const;
	AActor* SelectHeroLockFromFrame() const;
	ALaneMinion* FindNearestEnemyMinion() const;

	UPROPERTY()
	FLanePlaceholderStats Stats;

	UPROPERTY()
	TObjectPtr<AActor> LockedHero;
};
