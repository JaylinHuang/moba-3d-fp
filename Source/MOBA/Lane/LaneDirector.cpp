#include "Lane/LaneDirector.h"

#include "Lane/LaneDamageFeed.h"
#include "Lane/LaneMatchSubsystem.h"
#include "Lane/LaneMinion.h"
#include "TimerManager.h"

ALaneDirector::ALaneDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	Tags.Add(TEXT("LaneTestLayout"));
}

void ALaneDirector::Configure(const FLanePlaceholderStats& InStats, const TArray<FVector>& InPathTeam1, const TArray<FVector>& InPathTeam2, const TArray<ALaneUnit*>& InSiegeTeam1, const TArray<ALaneUnit*>& InSiegeTeam2)
{
	Stats = InStats;
	PathTeam1 = InPathTeam1;
	PathTeam2 = InPathTeam2;
	SiegeTeam1.Reset();
	SiegeTeam2.Reset();
	for (ALaneUnit* Unit : InSiegeTeam1)
	{
		SiegeTeam1.Add(Unit);
	}
	for (ALaneUnit* Unit : InSiegeTeam2)
	{
		SiegeTeam2.Add(Unit);
	}
}

void ALaneDirector::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
	{
		return;
	}

	if (!bLoggedPlaceholder)
	{
		bLoggedPlaceholder = true;
		UE_LOG(LogTemp, Log, TEXT("兵线占位：波次 %.0f 秒，近战 %d，远程 %d，移速 %.0f，近战距离 %.0f，远程距离 %.0f，索敌 %.0f/%.0f，出手间隔 %.2f，呼救半径 %.0f，塔范围 %.0f，塔间隔 %.2f，己方小兵半径 %.0f，有小兵系数 %.2f，无小兵系数 %.2f，营地半径 %.0f，营地刷新 %.0f 秒，野怪 %d。"),
			Stats.WaveInterval, Stats.MeleePerWave, Stats.RangedPerWave, Stats.MoveSpeed,
			Stats.MeleeAttackRange, Stats.RangedAttackRange, Stats.MeleeAggroRadius, Stats.RangedAggroRadius,
			Stats.AttackInterval, Stats.CallRadius, Stats.TowerAttackRange, Stats.TowerAttackInterval,
			Stats.AlliedMinionRadius, Stats.HeroDamageScaleWithMinions, Stats.HeroDamageScaleWithoutMinions,
			Stats.CampRadius, Stats.CampRespawnSeconds, Stats.MonsterCount);
	}

	SpawnBothWaves();
	GetWorldTimerManager().SetTimer(WaveTimer, this, &ALaneDirector::SpawnBothWaves, Stats.WaveInterval, true);
}

void ALaneDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
	{
		return;
	}
	if (ULaneDamageFeedSubsystem* Feed = GetWorld()->GetSubsystem<ULaneDamageFeedSubsystem>())
	{
		Feed->BeginFrame();
	}
}

void ALaneDirector::SpawnBothWaves()
{
	const ULaneMatchSubsystem* Match = GetWorld() != nullptr ? GetWorld()->GetSubsystem<ULaneMatchSubsystem>() : nullptr;
	if (Match != nullptr && Match->IsMatchEnded())
	{
		GetWorldTimerManager().ClearTimer(WaveTimer);
		return;
	}
	SpawnWave(1, PathTeam1, SiegeTeam1);
	SpawnWave(2, PathTeam2, SiegeTeam2);
}

void ALaneDirector::SpawnWave(int32 TeamId, const TArray<FVector>& Path, const TArray<TObjectPtr<ALaneUnit>>& SiegeTargets)
{
	if (GetWorld() == nullptr || Path.Num() == 0)
	{
		return;
	}

	const FVector Origin = Path[0];
	const float Direction = TeamId == 1 ? -1.f : 1.f;
	auto SpawnOne = [&](ELaneUnitKind Kind, int32 Index, float Lateral)
	{
		const FVector Location(Origin.X + Direction * Index * 90.f, Origin.Y + Lateral, 80.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ALaneMinion* Minion = GetWorld()->SpawnActor<ALaneMinion>(ALaneMinion::StaticClass(), Location, FRotator::ZeroRotator, Params))
		{
			Minion->Setup(Stats, TeamId, Kind, Path, SiegeTargets);
		}
	};

	for (int32 Index = 0; Index < Stats.MeleePerWave; ++Index)
	{
		SpawnOne(ELaneUnitKind::MeleeMinion, Index, (Index - 1) * 70.f);
	}
	for (int32 Index = 0; Index < Stats.RangedPerWave; ++Index)
	{
		SpawnOne(ELaneUnitKind::RangedMinion, Index + Stats.MeleePerWave, (Index - 1) * 70.f);
	}
}
