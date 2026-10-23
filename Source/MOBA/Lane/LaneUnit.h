#pragma once

#include "CoreMinimal.h"
#include "Attackable.h"
#include "GameFramework/Actor.h"
#include "HeroCombatProfile.h"
#include "Lane/LaneTypes.h"
#include "LaneUnit.generated.h"

/** 小兵、野怪、塔和基地的共同外观与可攻击接口。 */
UCLASS()
class MOBA_API ALaneUnit : public AActor, public IAttackable
{
	GENERATED_BODY()

public:
	ALaneUnit();

	virtual int32 GetTeamId() const override { return TeamId; }
	virtual bool IsAlive() const override { return bAlive; }
	ELaneUnitKind GetLaneKind() const { return Kind; }

	void SetTeamAndKind(int32 InTeamId, ELaneUnitKind InKind);
	void SetAlive(bool bInAlive) { bAlive = bInAlive; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyColor();
	void MoveToward(const FVector& WorldPoint, float Speed, float DeltaSeconds);
	/** 在攻击距离内且间隔已到就提交一次伤害请求。返回真表示人已经在距离内。 */
	bool TryAttack(AActor* Target, float Range, float Interval, EAttackDelivery Delivery);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(Replicated)
	int32 TeamId = 1;

	UPROPERTY(Replicated)
	ELaneUnitKind Kind = ELaneUnitKind::MeleeMinion;

	UPROPERTY(Replicated)
	bool bAlive = true;

	float NextAttackTime = 0.f;
};
