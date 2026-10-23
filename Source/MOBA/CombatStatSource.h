#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatStatSource.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCombatStatSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * 战斗模块落地后的读取口。
 * 返回真时，英雄使用这里的数值，并且不写回。
 */
class ICombatStatSource
{
	GENERATED_BODY()

public:
	virtual bool TryGetAttackRange(float& OutRange) const = 0;
	virtual bool TryGetAttackInterval(float& OutInterval) const = 0;
};
