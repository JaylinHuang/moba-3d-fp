#include "Lane/LaneDamageFeed.h"

#include "HeroVisibilityQuery.h"

void ULaneDamageFeedSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Visibility = NewObject<UHeroVisibilityQuery>(this);
}

void ULaneDamageFeedSubsystem::BeginFrame()
{
	Notices.Reset();
	NextOrder = 0;
}

void ULaneDamageFeedSubsystem::Push(const FHeroDamageRequest& Request)
{
	FLaneDamageNotice Notice;
	Notice.Source = Request.Source;
	Notice.Target = Request.Target;
	Notice.Delivery = Request.Delivery;
	Notice.Order = NextOrder++;
	Notices.Add(Notice);
}

bool ULaneDamageFeedSubsystem::IsVisibleTo(const AActor* Viewer, const AActor* Target) const
{
	return Visibility != nullptr && Visibility->IsVisibleTo(Viewer, Target);
}
