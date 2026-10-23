#pragma once

#include "CoreMinimal.h"

class UWorld;

/** 在关卡里放上一条兵线、两条路的空位、营地和草丛。已经有布局时不重复放。 */
struct MOBA_API FLaneLayout
{
	static void Ensure(UWorld* World);
};
