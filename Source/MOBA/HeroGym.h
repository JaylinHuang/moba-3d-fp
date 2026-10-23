#pragma once

#include "CoreMinimal.h"

class UWorld;

/** 在关卡里放上地板、玩家出生点和可攻击对照单位。 */
struct MOBA_API FHeroGymSpawner
{
	static void Ensure(UWorld* World);
};
