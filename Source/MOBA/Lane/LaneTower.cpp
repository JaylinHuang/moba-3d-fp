#include "Lane/LaneTower.h"

#include "EngineUtils.h"
#include "Lane/LaneDamageFeed.h"
#include "Lane/LaneMinion.h"
#include "Lane/LaneQueries.h"

ALaneTower::ALaneTower()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetActorScale3D(FVector(1.1f, 1.1f, 2.4f));
	Kind = ELaneUnitKind::OuterTower;
}

void ALaneTower::Setup(const FLanePlaceholderStats& InStats, int32 InTeamId, ELaneUnitKind InKind)
{
	Stats = InStats;
	SetTeamAndKind(InTeamId, InKind);
}

bool ALaneTower::HasAlliedMinionNearby() const
{
	if (GetWorld() == nullptr)
	{
		return false;
	}
	for (TActorIterator<ALaneMinion> It(GetWorld()); It; ++It)
	{
		const ALaneMinion* Minion = *It;
		if (Minion->GetTeamId() != TeamId || !Minion->IsAlive())
		{
			continue;
		}
		if (LaneHorizontalDistance(GetActorLocation(), Minion->GetActorLocation()) <= Stats.AlliedMinionRadius)
		{
			return true;
		}
	}
	return false;
}

float ALaneTower::GetIncomingHeroDamageScale() const
{
	return HasAlliedMinionNearby() ? Stats.HeroDamageScaleWithMinions : Stats.HeroDamageScaleWithoutMinions;
}

bool ALaneTower::IsLockStillValid() const
{
	if (!LaneIsAlive(LockedHero) || !LaneIsVisibleTo(this, LockedHero))
	{
		return false;
	}
	return LaneHorizontalDistance(GetActorLocation(), LockedHero->GetActorLocation()) <= Stats.TowerAttackRange;
}

AActor* ALaneTower::SelectHeroLockFromFrame() const
{
	const ULaneDamageFeedSubsystem* Feed = GetWorld() != nullptr ? GetWorld()->GetSubsystem<ULaneDamageFeedSubsystem>() : nullptr;
	if (Feed == nullptr)
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	int32 BestOrder = MAX_int32;
	for (const FLaneDamageNotice& Notice : Feed->GetFrame())
	{
		if (!LaneIsEnemyHero(this, Notice.Source) || !LaneIsAlliedHero(this, Notice.Target))
		{
			continue;
		}
		if (!LaneIsVisibleTo(this, Notice.Source))
		{
			continue;
		}
		const float Distance = LaneHorizontalDistance(GetActorLocation(), Notice.Source->GetActorLocation());
		if (Distance > Stats.TowerAttackRange)
		{
			continue;
		}
		const bool bCloser = Distance < BestDistance - KINDA_SMALL_NUMBER;
		const bool bSameDistanceEarlier = FMath::IsNearlyEqual(Distance, BestDistance) && Notice.Order < BestOrder;
		if (Best == nullptr || bCloser || bSameDistanceEarlier)
		{
			Best = Notice.Source;
			BestDistance = Distance;
			BestOrder = Notice.Order;
		}
	}
	return Best;
}

ALaneMinion* ALaneTower::FindNearestEnemyMinion() const
{
	ALaneMinion* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}
	for (TActorIterator<ALaneMinion> It(GetWorld()); It; ++It)
	{
		ALaneMinion* Candidate = *It;
		if (Candidate->GetTeamId() == TeamId || !Candidate->IsAlive() || !LaneIsVisibleTo(this, Candidate))
		{
			continue;
		}
		const float Distance = LaneHorizontalDistance(GetActorLocation(), Candidate->GetActorLocation());
		if (Distance > Stats.TowerAttackRange || Distance >= BestDistance)
		{
			continue;
		}
		Best = Candidate;
		BestDistance = Distance;
	}
	return Best;
}

void ALaneTower::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
	{
		return;
	}

	if (LockedHero != nullptr)
	{
		if (IsLockStillValid())
		{
			TryAttack(LockedHero, Stats.TowerAttackRange, Stats.TowerAttackInterval, EAttackDelivery::Melee);
			return;
		}
		// 这一拍的英雄请求发生在锁定仍存在时，放开后不拿来重锁。小兵仍按没有锁定的规则选择。
		LockedHero = nullptr;
		if (ALaneMinion* Minion = FindNearestEnemyMinion())
		{
			TryAttack(Minion, Stats.TowerAttackRange, Stats.TowerAttackInterval, EAttackDelivery::Melee);
		}
		return;
	}

	if (AActor* Hero = SelectHeroLockFromFrame())
	{
		LockedHero = Hero;
		TryAttack(LockedHero, Stats.TowerAttackRange, Stats.TowerAttackInterval, EAttackDelivery::Melee);
		return;
	}

	if (ALaneMinion* Minion = FindNearestEnemyMinion())
	{
		TryAttack(Minion, Stats.TowerAttackRange, Stats.TowerAttackInterval, EAttackDelivery::Melee);
	}
}
