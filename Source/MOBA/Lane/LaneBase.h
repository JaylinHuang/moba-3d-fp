#pragma once

#include "CoreMinimal.h"
#include "Lane/LaneUnit.h"
#include "LaneBase.generated.h"

/** 基地。被摧毁入口只改对局状态，不把生命减到零。 */
UCLASS()
class MOBA_API ALaneBase : public ALaneUnit
{
	GENERATED_BODY()

public:
	ALaneBase();

	void Setup(int32 InTeamId);

	/** 服务器入口。调用后本基地不再可被攻击，对局进入结束状态。 */
	void NotifyDestroyed();
};
