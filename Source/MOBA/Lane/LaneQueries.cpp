#include "Lane/LaneQueries.h"

#include "Attackable.h"
#include "DamageRequest.h"
#include "Lane/LaneDamageFeed.h"
#include "Lane/LaneUnit.h"

float LaneHorizontalDistance(const FVector& A, const FVector& B)
{
	return FVector::Dist2D(A, B);
}

bool LaneIsHero(const AActor* Actor)
{
	if (Actor == nullptr || Actor->IsA<ALaneUnit>())
	{
		return false;
	}
	return Cast<IAttackable>(Actor) != nullptr;
}

bool LaneIsAlive(const AActor* Actor)
{
	const IAttackable* Attackable = Cast<IAttackable>(Actor);
	return Attackable != nullptr && Attackable->IsAlive();
}

int32 LaneTeamId(const AActor* Actor)
{
	const IAttackable* Attackable = Cast<IAttackable>(Actor);
	return Attackable != nullptr ? Attackable->GetTeamId() : INDEX_NONE;
}

bool LaneIsEnemyHero(const AActor* Viewer, const AActor* Actor)
{
	return LaneIsHero(Actor) && LaneIsAlive(Actor) && LaneTeamId(Viewer) != LaneTeamId(Actor);
}

bool LaneIsAlliedHero(const AActor* Viewer, const AActor* Actor)
{
	return LaneIsHero(Actor) && LaneIsAlive(Actor) && LaneTeamId(Viewer) == LaneTeamId(Actor);
}

bool LaneIsVisibleTo(const AActor* Viewer, const AActor* Target)
{
	const UWorld* World = Viewer != nullptr ? Viewer->GetWorld() : nullptr;
	const ULaneDamageFeedSubsystem* Feed = World != nullptr ? World->GetSubsystem<ULaneDamageFeedSubsystem>() : nullptr;
	return Feed != nullptr && Feed->IsVisibleTo(Viewer, Target);
}

bool LaneIsPlaceholderSingleTarget(EAttackDelivery Delivery)
{
	return Delivery == EAttackDelivery::Melee || Delivery == EAttackDelivery::Projectile;
}

void LaneSubmitDamage(UWorld* World, AActor* Source, AActor* Target, EAttackDelivery Delivery)
{
	if (World == nullptr || Source == nullptr || Target == nullptr)
	{
		return;
	}

	FHeroDamageRequest Request;
	Request.Source = Source;
	Request.Target = Target;
	Request.Delivery = Delivery;
	if (UDamageRequestSubsystem* Sink = World->GetSubsystem<UDamageRequestSubsystem>())
	{
		Sink->Submit(Request);
	}
}
