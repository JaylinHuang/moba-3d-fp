#include "Lane/LaneMinion.h"

#include "EngineUtils.h"
#include "Lane/LaneDamageFeed.h"
#include "Lane/LaneMatchSubsystem.h"
#include "Lane/LaneQueries.h"

ALaneMinion::ALaneMinion()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetActorScale3D(FVector(0.5f, 0.5f, 0.85f));
}

void ALaneMinion::Setup(const FLanePlaceholderStats& InStats, int32 InTeamId, ELaneUnitKind InKind, const TArray<FVector>& InPath, const TArray<TObjectPtr<ALaneUnit>>& InSiegeTargets)
{
	Stats = InStats;
	Path = InPath;
	SiegeTargets = InSiegeTargets;
	SetTeamAndKind(InTeamId, InKind);
	if (InKind == ELaneUnitKind::RangedMinion)
	{
		AttackRange = Stats.RangedAttackRange;
		AggroRadius = Stats.RangedAggroRadius;
		Delivery = EAttackDelivery::Projectile;
		SetActorScale3D(FVector(0.35f, 0.35f, 1.05f));
	}
	else
	{
		AttackRange = Stats.MeleeAttackRange;
		AggroRadius = Stats.MeleeAggroRadius;
		Delivery = EAttackDelivery::Melee;
		SetActorScale3D(FVector(0.5f, 0.5f, 0.85f));
	}
}

bool ALaneMinion::IsUsable(const AActor* Target) const
{
	if (!LaneIsAlive(Target) || !LaneIsVisibleTo(this, Target) || LaneTeamId(Target) == TeamId)
	{
		return false;
	}
	if (const ALaneMinion* Minion = Cast<ALaneMinion>(Target))
	{
		return Minion->GetLaneKind() == ELaneUnitKind::MeleeMinion || Minion->GetLaneKind() == ELaneUnitKind::RangedMinion;
	}
	if (LaneIsHero(Target))
	{
		return true;
	}
	const ALaneUnit* Unit = Cast<ALaneUnit>(Target);
	return Unit != nullptr && LaneIsStructureKind(Unit->GetLaneKind());
}

bool ALaneMinion::IsMinionStillInAggro(const AActor* Target) const
{
	return Target != nullptr && LaneHorizontalDistance(GetActorLocation(), Target->GetActorLocation()) <= AggroRadius;
}

AActor* ALaneMinion::ChooseByDistance() const
{
	AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		const bool bMinion = Cast<ALaneMinion>(Candidate) != nullptr;
		const bool bHero = LaneIsEnemyHero(this, Candidate);
		if (!bMinion && !bHero)
		{
			continue;
		}
		if (!LaneIsAlive(Candidate) || !LaneIsVisibleTo(this, Candidate) || LaneTeamId(Candidate) == TeamId)
		{
			continue;
		}
		const float Distance = LaneHorizontalDistance(GetActorLocation(), Candidate->GetActorLocation());
		if (Distance > AggroRadius || Distance >= BestDistance)
		{
			continue;
		}
		Best = Candidate;
		BestDistance = Distance;
	}
	return Best;
}

ALaneMinion* ALaneMinion::FindNearestEnemyMinion() const
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
		if (Candidate == this || Candidate->GetTeamId() == TeamId || !Candidate->IsAlive() || !LaneIsVisibleTo(this, Candidate))
		{
			continue;
		}
		const float Distance = LaneHorizontalDistance(GetActorLocation(), Candidate->GetActorLocation());
		if (Distance > AggroRadius || Distance >= BestDistance)
		{
			continue;
		}
		Best = Candidate;
		BestDistance = Distance;
	}
	return Best;
}

ALaneUnit* ALaneMinion::NextEnemyStructure() const
{
	for (const TObjectPtr<ALaneUnit>& Structure : SiegeTargets)
	{
		if (Structure != nullptr && Structure->IsAlive() && LaneIsVisibleTo(this, Structure))
		{
			return Structure;
		}
	}
	return nullptr;
}

void ALaneMinion::ApplyCallForHelp()
{
	if (bStructureLocked)
	{
		return;
	}
	if (LaneIsEnemyHero(this, CurrentTarget) && IsUsable(CurrentTarget))
	{
		return;
	}

	const ULaneDamageFeedSubsystem* Feed = GetWorld() != nullptr ? GetWorld()->GetSubsystem<ULaneDamageFeedSubsystem>() : nullptr;
	if (Feed == nullptr)
	{
		return;
	}

	const FLaneDamageNotice* Best = nullptr;
	for (const FLaneDamageNotice& Notice : Feed->GetFrame())
	{
		if (!LaneIsPlaceholderSingleTarget(Notice.Delivery) || !LaneIsEnemyHero(this, Notice.Source) || !LaneIsAlliedHero(this, Notice.Target))
		{
			continue;
		}
		if (!LaneIsVisibleTo(this, Notice.Source))
		{
			continue;
		}
		if (LaneHorizontalDistance(GetActorLocation(), Notice.Source->GetActorLocation()) > Stats.CallRadius)
		{
			continue;
		}
		if (Best == nullptr || Notice.Order < Best->Order)
		{
			Best = &Notice;
		}
	}

	if (Best != nullptr)
	{
		CurrentTarget = Best->Source;
		bStructureLocked = false;
	}
}

void ALaneMinion::ChaseAndAttack(AActor* Target, float DeltaSeconds)
{
	if (Target == nullptr)
	{
		return;
	}
	if (!TryAttack(Target, AttackRange, Stats.AttackInterval, Delivery))
	{
		MoveToward(Target->GetActorLocation(), Stats.MoveSpeed, DeltaSeconds);
	}
}

void ALaneMinion::FollowPath(float DeltaSeconds)
{
	if (!Path.IsValidIndex(PathIndex))
	{
		return;
	}
	const FVector Point = Path[PathIndex];
	if (LaneHorizontalDistance(GetActorLocation(), Point) < 80.f && PathIndex + 1 < Path.Num())
	{
		++PathIndex;
	}
	MoveToward(Path[PathIndex], Stats.MoveSpeed, DeltaSeconds);
}

void ALaneMinion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
	{
		return;
	}

	const ULaneMatchSubsystem* Match = GetWorld()->GetSubsystem<ULaneMatchSubsystem>();
	const bool bEnded = Match != nullptr && Match->IsMatchEnded();

	if (CurrentTarget != nullptr && !IsUsable(CurrentTarget))
	{
		const bool bDeadOrHidden = !LaneIsAlive(CurrentTarget) || !LaneIsVisibleTo(this, CurrentTarget);
		CurrentTarget = nullptr;
		bStructureLocked = false;
		if (bEnded)
		{
			return;
		}
		if (bDeadOrHidden)
		{
			CurrentTarget = ChooseByDistance();
		}
	}
	else if (CurrentTarget != nullptr && Cast<ALaneMinion>(CurrentTarget) != nullptr && !IsMinionStillInAggro(CurrentTarget))
	{
		CurrentTarget = nullptr;
	}

	if (bEnded && CurrentTarget == nullptr)
	{
		return;
	}

	ApplyCallForHelp();

	if (bStructureLocked && CurrentTarget != nullptr)
	{
		ChaseAndAttack(CurrentTarget, DeltaSeconds);
		return;
	}

	if (LaneIsEnemyHero(this, CurrentTarget) || Cast<ALaneMinion>(CurrentTarget) != nullptr)
	{
		ChaseAndAttack(CurrentTarget, DeltaSeconds);
		return;
	}

	if (ALaneMinion* Nearest = FindNearestEnemyMinion())
	{
		CurrentTarget = Nearest;
		ChaseAndAttack(Nearest, DeltaSeconds);
		return;
	}

	if (ALaneUnit* Structure = NextEnemyStructure())
	{
		if (LaneHorizontalDistance(GetActorLocation(), Structure->GetActorLocation()) <= AttackRange)
		{
			CurrentTarget = Structure;
			bStructureLocked = true;
			ChaseAndAttack(Structure, DeltaSeconds);
			return;
		}
	}

	FollowPath(DeltaSeconds);
}
