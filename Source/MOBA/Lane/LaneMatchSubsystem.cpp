#include "Lane/LaneMatchSubsystem.h"

void ULaneMatchSubsystem::NotifyBaseDestroyed(int32 DestroyedBaseTeamIdIn)
{
	if (bEnded)
	{
		return;
	}

	bEnded = true;
	DestroyedBaseTeamId = DestroyedBaseTeamIdIn;
	WinnerTeamId = DestroyedBaseTeamIdIn == 1 ? 2 : 1;
	UE_LOG(LogTemp, Log, TEXT("基地被摧毁。被摧毁队伍=%d，获胜队伍=%d。不扣生命，不写局外服务。"),
		DestroyedBaseTeamId, WinnerTeamId);
}
