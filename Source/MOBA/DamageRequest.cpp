#include "DamageRequest.h"

#include "Combat/CombatResolutionSubsystem.h"
#include "Lane/LaneDamageFeed.h"

void UDamageRequestSubsystem::Submit(const FHeroDamageRequest& Request)
{
	// 先留进本拍列表，再覆盖最后一条。塔要比较这一拍里的全部英雄攻击。
	// 追加本身不改生命、金钱和经验。
	if (UWorld* World = GetWorld())
	{
		if (ULaneDamageFeedSubsystem* Feed = World->GetSubsystem<ULaneDamageFeedSubsystem>())
		{
			Feed->Push(Request);
		}
	}

	LastRequest = Request;
	++SubmitCount;

	if (UWorld* World = GetWorld())
	{
		if (UCombatResolutionSubsystem* Combat = World->GetSubsystem<UCombatResolutionSubsystem>())
		{
			Combat->Resolve(Request);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("伤害请求已提交，来源=%s 目标=%s。权威端在追加之后结算。"),
		*GetNameSafe(Request.Source),
		*GetNameSafe(Request.Target));
}
