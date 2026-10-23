#pragma once

#include "CoreMinimal.h"
#include "Lane/LaneTypes.h"
#include "Lane/LaneUnit.h"
#include "LaneMinion.generated.h"

/** 沿路点推进的近战或远程小兵。战斗写入生命之前一直存活。 */
UCLASS()
class MOBA_API ALaneMinion : public ALaneUnit
{
	GENERATED_BODY()

public:
	ALaneMinion();

	void Setup(const FLanePlaceholderStats& InStats, int32 InTeamId, ELaneUnitKind InKind, const TArray<FVector>& InPath, const TArray<TObjectPtr<ALaneUnit>>& InSiegeTargets);

	virtual void Tick(float DeltaSeconds) override;

private:
	bool IsUsable(const AActor* Target) const;
	bool IsMinionStillInAggro(const AActor* Target) const;
	AActor* ChooseByDistance() const;
	ALaneMinion* FindNearestEnemyMinion() const;
	ALaneUnit* NextEnemyStructure() const;
	void ApplyCallForHelp();
	void ChaseAndAttack(AActor* Target, float DeltaSeconds);
	void FollowPath(float DeltaSeconds);

	UPROPERTY()
	FLanePlaceholderStats Stats;

	UPROPERTY()
	TArray<FVector> Path;

	UPROPERTY()
	TArray<TObjectPtr<ALaneUnit>> SiegeTargets;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	bool bStructureLocked = false;
	int32 PathIndex = 0;
	float AttackRange = 180.f;
	float AggroRadius = 600.f;
	EAttackDelivery Delivery = EAttackDelivery::Melee;
};
