#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Attackable.h"
#include "AttackDummy.generated.h"

/** 测试图上的可攻击对照单位。没有生命结算，默认一直存活。 */
UCLASS()
class MOBA_API AAttackDummy : public AActor, public IAttackable
{
	GENERATED_BODY()

public:
	AAttackDummy();

	virtual int32 GetTeamId() const override { return TeamId; }
	virtual bool IsAlive() const override { return bAlive; }

	void SetAlive(bool bInAlive) { bAlive = bInAlive; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> Body;

	UPROPERTY(EditAnywhere, Category = "队伍")
	int32 TeamId = 2;

	UPROPERTY(Replicated, EditAnywhere, Category = "队伍")
	bool bAlive = true;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
