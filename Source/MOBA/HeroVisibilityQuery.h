#pragma once

#include "CoreMinimal.h"
#include "HeroVisibilityQuery.generated.h"

/** 目标是否可见。具体判定在世界视野子系统，英雄和兵线共用这一份。 */
UCLASS()
class MOBA_API UHeroVisibilityQuery : public UObject
{
	GENERATED_BODY()

public:
	virtual bool IsVisibleTo(const AActor* Viewer, const AActor* Target) const;

	/** 不分配查询对象。同步子系统缺失时，双方都存在则视为可见。 */
	static bool CanSee(const AActor* Viewer, const AActor* Target);
};
