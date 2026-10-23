#include "Vision/VisionWorldSubsystem.h"

#include "Attackable.h"
#include "BasicAttackProjectile.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Lane/LaneDamageFeed.h"
#include "Lane/LaneProps.h"
#include "Lane/LaneUnit.h"
#include "MOBACharacter.h"

/** 帧后段 tick。赶在下一帧清空伤害流水之前读取本帧出手。 */
struct FVisionEndOfFrameTickFunction : public FTickFunction
{
	FVisionEndOfFrameTickFunction()
	{
		bCanEverTick = true;
		bStartWithTickEnabled = true;
		TickGroup = TG_PostUpdateWork;
		EndTickGroup = TG_PostUpdateWork;
	}

	UVisionWorldSubsystem* Owner = nullptr;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override
	{
		if (Owner != nullptr)
		{
			Owner->HandleEndOfFrame();
		}
	}

	virtual FString DiagnosticMessage() override
	{
		return TEXT("视野帧后段");
	}
};

void UVisionWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld() || World->GetNetMode() == NM_Client || World->PersistentLevel == nullptr)
	{
		return;
	}

	EndOfFrameTick = new FVisionEndOfFrameTickFunction();
	EndOfFrameTick->Owner = this;
	EndOfFrameTick->RegisterTickFunction(World->PersistentLevel);

	if (!bLoggedPlaceholder)
	{
		bLoggedPlaceholder = true;
		UE_LOG(LogTemp, Log, TEXT("视野占位：距离 %.0f，显形 %.1f 秒，视线抬高 %.0f。"),
			Placeholder.SightRadius, Placeholder.RevealSeconds, Placeholder.TraceLift);
	}
}

void UVisionWorldSubsystem::Deinitialize()
{
	if (EndOfFrameTick != nullptr)
	{
		EndOfFrameTick->UnRegisterTickFunction();
		delete EndOfFrameTick;
		EndOfFrameTick = nullptr;
	}
	Super::Deinitialize();
}

void UVisionWorldSubsystem::HandleEndOfFrame()
{
	PruneExpired();
	EnsureFrameData();
	ConsumeDamageFeed();
	ScanProjectiles();
}

bool UVisionWorldSubsystem::IsVisibleTo(const AActor* Viewer, const AActor* Target)
{
	if (Viewer == nullptr || Target == nullptr)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World != nullptr && World->GetNetMode() == NM_Client)
	{
		return true;
	}

	EnsureFrameData();

	if (const ABasicAttackProjectile* Projectile = Cast<ABasicAttackProjectile>(Target))
	{
		return IsProjectileVisibleTo(Viewer, Projectile);
	}

	const int32 ViewerTeam = TeamOf(Viewer);
	const int32 TargetTeam = TeamOf(Target);
	if (ViewerTeam != INDEX_NONE && ViewerTeam == TargetTeam)
	{
		return true;
	}

	if (ViewerTeam != INDEX_NONE && IsRevealedTo(Target, ViewerTeam))
	{
		return true;
	}

	return AnyProviderSees(ViewerTeam, Target);
}

void UVisionWorldSubsystem::EnsureFrameData()
{
	UWorld* World = GetWorld();
	if (World == nullptr || CachedFrame == GFrameCounter)
	{
		return;
	}

	CachedFrame = GFrameCounter;
	Brushes.Reset();
	Units.Reset();
	UnitIndex.Reset();

	for (TActorIterator<ALaneBrush> It(World); It; ++It)
	{
		Brushes.Add(*It);
	}

	auto CacheActor = [this](AActor* Actor)
	{
		if (Actor == nullptr || UnitIndex.Contains(Actor))
		{
			return;
		}

		FCachedUnit& Cached = Units.AddDefaulted_GetRef();
		Cached.Actor = Actor;
		Cached.TeamId = TeamOf(Actor);
		Cached.bProvidesVision = ProvidesVision(Actor);
		for (const TWeakObjectPtr<ALaneBrush>& Brush : Brushes)
		{
			if (ALaneBrush* BrushActor = Brush.Get())
			{
				if (BrushActor->Contains(Actor))
				{
					Cached.Brushes.Add(BrushActor);
				}
			}
		}
		UnitIndex.Add(Actor, Units.Num() - 1);
	};

	for (TActorIterator<AMOBACharacter> It(World); It; ++It)
	{
		CacheActor(*It);
	}
	for (TActorIterator<ALaneUnit> It(World); It; ++It)
	{
		CacheActor(*It);
	}
}

void UVisionWorldSubsystem::CollectBrushesOf(const AActor* Actor, TArray<ALaneBrush*>& OutBrushes) const
{
	OutBrushes.Reset();
	if (Actor == nullptr)
	{
		return;
	}

	if (const int32* Index = UnitIndex.Find(Actor))
	{
		for (const TWeakObjectPtr<ALaneBrush>& Brush : Units[*Index].Brushes)
		{
			if (ALaneBrush* BrushActor = Brush.Get())
			{
				OutBrushes.Add(BrushActor);
			}
		}
		return;
	}

	for (const TWeakObjectPtr<ALaneBrush>& Brush : Brushes)
	{
		if (ALaneBrush* BrushActor = Brush.Get())
		{
			if (BrushActor->Contains(Actor))
			{
				OutBrushes.Add(BrushActor);
			}
		}
	}
}

bool UVisionWorldSubsystem::BrushAllows(const TArray<TWeakObjectPtr<ALaneBrush>>& ProviderBrushes, const TArray<ALaneBrush*>& TargetBrushes) const
{
	if (TargetBrushes.Num() == 0)
	{
		return true;
	}

	for (ALaneBrush* TargetBrush : TargetBrushes)
	{
		for (const TWeakObjectPtr<ALaneBrush>& ProviderBrush : ProviderBrushes)
		{
			if (ProviderBrush.Get() == TargetBrush)
			{
				return true;
			}
		}
	}
	return false;
}

bool UVisionWorldSubsystem::HasStaticLineOfSight(const AActor* From, const AActor* To) const
{
	UWorld* World = GetWorld();
	if (World == nullptr || From == nullptr || To == nullptr)
	{
		return false;
	}

	const FVector Lift(0.f, 0.f, Placeholder.TraceLift);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VisionSight), false, From);
	Params.AddIgnoredActor(To);
	for (const FCachedUnit& Unit : Units)
	{
		if (AActor* Ignored = Unit.Actor.Get())
		{
			Params.AddIgnoredActor(Ignored);
		}
	}
	for (const TWeakObjectPtr<ABasicAttackProjectile>& Projectile : SeenProjectiles)
	{
		if (ABasicAttackProjectile* Ignored = Projectile.Get())
		{
			Params.AddIgnoredActor(Ignored);
		}
	}

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(
		Hit,
		From->GetActorLocation() + Lift,
		To->GetActorLocation() + Lift,
		ECC_WorldStatic,
		Params);
	return !bBlocked;
}

bool UVisionWorldSubsystem::HasPersonalSight(const FCachedUnit& Provider, const AActor* Target) const
{
	const AActor* From = Provider.Actor.Get();
	if (From == nullptr || Target == nullptr || From == Target)
	{
		return false;
	}

	if (FVector::Dist2D(From->GetActorLocation(), Target->GetActorLocation()) > Placeholder.SightRadius)
	{
		return false;
	}

	TArray<ALaneBrush*> TargetBrushes;
	CollectBrushesOf(Target, TargetBrushes);
	if (!BrushAllows(Provider.Brushes, TargetBrushes))
	{
		return false;
	}

	return HasStaticLineOfSight(From, Target);
}

bool UVisionWorldSubsystem::AnyProviderSees(int32 TeamId, const AActor* Target) const
{
	if (TeamId == INDEX_NONE || Target == nullptr)
	{
		return false;
	}

	for (const FCachedUnit& Unit : Units)
	{
		if (!Unit.bProvidesVision || Unit.TeamId != TeamId)
		{
			continue;
		}
		if (HasPersonalSight(Unit, Target))
		{
			return true;
		}
	}
	return false;
}

bool UVisionWorldSubsystem::IsRevealedTo(const AActor* Actor, int32 TeamId) const
{
	if (Actor == nullptr || GetWorld() == nullptr)
	{
		return false;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	for (const FRevealRecord& Record : Reveals)
	{
		if (Record.TeamId == TeamId && Record.Actor.Get() == Actor && Now < Record.UntilTime)
		{
			return true;
		}
	}
	return false;
}

bool UVisionWorldSubsystem::IsProjectileRevealedTo(const ABasicAttackProjectile* Projectile, int32 TeamId) const
{
	if (Projectile == nullptr || GetWorld() == nullptr)
	{
		return false;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	for (const FProjectileReveal& Record : ProjectileReveals)
	{
		if (Record.TeamId == TeamId && Record.Projectile.Get() == Projectile && Now < Record.UntilTime)
		{
			return true;
		}
	}
	return false;
}

int32 UVisionWorldSubsystem::TeamOf(const AActor* Actor) const
{
	const IAttackable* Attackable = Cast<IAttackable>(Actor);
	return Attackable != nullptr ? Attackable->GetTeamId() : INDEX_NONE;
}

bool UVisionWorldSubsystem::ProvidesVision(const AActor* Actor) const
{
	const IAttackable* Attackable = Cast<IAttackable>(Actor);
	if (Attackable == nullptr || !Attackable->IsAlive())
	{
		return false;
	}

	if (Actor->IsA<AMOBACharacter>())
	{
		return true;
	}

	const ALaneUnit* Unit = Cast<ALaneUnit>(Actor);
	if (Unit == nullptr)
	{
		return false;
	}

	switch (Unit->GetLaneKind())
	{
	case ELaneUnitKind::MeleeMinion:
	case ELaneUnitKind::RangedMinion:
	case ELaneUnitKind::OuterTower:
	case ELaneUnitKind::InnerTower:
	case ELaneUnitKind::Base:
		return true;
	default:
		return false;
	}
}

void UVisionWorldSubsystem::GatherEnemyTeams(int32 TeamId, TArray<int32, TInlineAllocator<2>>& OutTeams) const
{
	OutTeams.Reset();
	if (TeamId == 1)
	{
		OutTeams.Add(2);
	}
	else if (TeamId == 2)
	{
		OutTeams.Add(1);
	}
}

void UVisionWorldSubsystem::TouchReveal(AActor* Actor, int32 TeamId, double UntilTime)
{
	if (Actor == nullptr || TeamId == INDEX_NONE)
	{
		return;
	}

	for (FRevealRecord& Record : Reveals)
	{
		if (Record.Actor.Get() == Actor && Record.TeamId == TeamId)
		{
			Record.UntilTime = FMath::Max(Record.UntilTime, UntilTime);
			return;
		}
	}

	FRevealRecord& Record = Reveals.AddDefaulted_GetRef();
	Record.Actor = Actor;
	Record.TeamId = TeamId;
	Record.UntilTime = UntilTime;
}

void UVisionWorldSubsystem::RevealActorToTeams(AActor* Actor, const TArray<int32, TInlineAllocator<2>>& Teams, double UntilTime)
{
	for (const int32 TeamId : Teams)
	{
		TouchReveal(Actor, TeamId, UntilTime);
	}
}

void UVisionWorldSubsystem::ConsumeDamageFeed()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const ULaneDamageFeedSubsystem* Feed = World->GetSubsystem<ULaneDamageFeedSubsystem>();
	if (Feed == nullptr)
	{
		return;
	}

	const double UntilTime = World->GetTimeSeconds() + Placeholder.RevealSeconds;
	for (const FLaneDamageNotice& Notice : Feed->GetFrame())
	{
		AActor* Source = Notice.Source;
		AActor* Target = Notice.Target;
		TArray<ALaneBrush*> SourceBrushes;
		TArray<ALaneBrush*> TargetBrushes;
		CollectBrushesOf(Source, SourceBrushes);
		CollectBrushesOf(Target, TargetBrushes);

		if (SourceBrushes.Num() > 0)
		{
			TArray<int32, TInlineAllocator<2>> EnemyTeams;
			GatherEnemyTeams(TeamOf(Source), EnemyTeams);
			RevealActorToTeams(Source, EnemyTeams, UntilTime);
		}
		if (TargetBrushes.Num() > 0)
		{
			TouchReveal(Target, TeamOf(Source), UntilTime);
		}
	}
}

void UVisionWorldSubsystem::ScanProjectiles()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<ABasicAttackProjectile> It(World); It; ++It)
	{
		ABasicAttackProjectile* Projectile = *It;
		if (Projectile == nullptr)
		{
			continue;
		}

		bool bAlreadySeen = false;
		for (const TWeakObjectPtr<ABasicAttackProjectile>& Seen : SeenProjectiles)
		{
			if (Seen.Get() == Projectile)
			{
				bAlreadySeen = true;
				break;
			}
		}
		if (bAlreadySeen)
		{
			continue;
		}

		SeenProjectiles.Add(Projectile);
		AActor* Source = Projectile->GetOwner();
		if (Source == nullptr)
		{
			Source = Projectile->GetInstigator();
		}

		TArray<ALaneBrush*> SourceBrushes;
		CollectBrushesOf(Source, SourceBrushes);
		if (Source == nullptr || SourceBrushes.Num() == 0)
		{
			continue;
		}

		TArray<int32, TInlineAllocator<2>> EnemyTeams;
		GatherEnemyTeams(TeamOf(Source), EnemyTeams);
		const double UntilTime = World->GetTimeSeconds() + Placeholder.RevealSeconds;
		RevealActorToTeams(Source, EnemyTeams, UntilTime);
		for (const int32 TeamId : EnemyTeams)
		{
			FProjectileReveal& Record = ProjectileReveals.AddDefaulted_GetRef();
			Record.Projectile = Projectile;
			Record.TeamId = TeamId;
			Record.UntilTime = UntilTime;
		}
	}
}

void UVisionWorldSubsystem::PruneExpired()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	Reveals.RemoveAll([Now](const FRevealRecord& Record)
	{
		return !Record.Actor.IsValid() || Now >= Record.UntilTime;
	});
	ProjectileReveals.RemoveAll([Now](const FProjectileReveal& Record)
	{
		return !Record.Projectile.IsValid() || Now >= Record.UntilTime;
	});
	SeenProjectiles.RemoveAll([](const TWeakObjectPtr<ABasicAttackProjectile>& Projectile)
	{
		return !Projectile.IsValid();
	});
}

bool UVisionWorldSubsystem::IsProjectileVisibleTo(const AActor* Viewer, const ABasicAttackProjectile* Projectile)
{
	EnsureFrameData();

	const int32 ViewerTeam = TeamOf(Viewer);
	AActor* Source = Projectile->GetOwner();
	if (Source == nullptr)
	{
		Source = Projectile->GetInstigator();
	}
	if (ViewerTeam != INDEX_NONE && TeamOf(Source) == ViewerTeam)
	{
		return true;
	}
	if (IsProjectileRevealedTo(Projectile, ViewerTeam) || IsRevealedTo(Source, ViewerTeam))
	{
		return true;
	}
	return AnyProviderSees(ViewerTeam, Projectile);
}
