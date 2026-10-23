#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Attackable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UAttackable : public UInterface
{
	GENERATED_BODY()
};

/** 可被普通攻击选中的单位。友军由队伍编号区分。 */
class IAttackable
{
	GENERATED_BODY()

public:
	virtual int32 GetTeamId() const = 0;
	virtual bool IsAlive() const = 0;
};
