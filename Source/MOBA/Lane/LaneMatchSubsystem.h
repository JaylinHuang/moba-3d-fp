#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LaneMatchSubsystem.generated.h"

/** 服务器上的对局胜负。不写局外服务，也不扣生命。 */
UCLASS()
class MOBA_API ULaneMatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 某座基地被摧毁。先到的结果保留。获胜方是被摧毁基地的对方队伍。 */
	void NotifyBaseDestroyed(int32 DestroyedBaseTeamId);

	bool IsMatchEnded() const { return bEnded; }
	int32 GetWinnerTeamId() const { return bEnded ? WinnerTeamId : INDEX_NONE; }
	int32 GetDestroyedBaseTeamId() const { return bEnded ? DestroyedBaseTeamId : INDEX_NONE; }

private:
	bool bEnded = false;
	int32 WinnerTeamId = INDEX_NONE;
	int32 DestroyedBaseTeamId = INDEX_NONE;
};
