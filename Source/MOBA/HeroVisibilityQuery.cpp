#include "HeroVisibilityQuery.h"

#include "Vision/VisionWorldSubsystem.h"

bool UHeroVisibilityQuery::CanSee(const AActor* Viewer, const AActor* Target)
{
	if (Viewer == nullptr || Target == nullptr)
	{
		return false;
	}

	UWorld* World = Viewer->GetWorld();
	if (UVisionWorldSubsystem* Vision = World != nullptr ? World->GetSubsystem<UVisionWorldSubsystem>() : nullptr)
	{
		return Vision->IsVisibleTo(Viewer, Target);
	}

	return true;
}

bool UHeroVisibilityQuery::IsVisibleTo(const AActor* Viewer, const AActor* Target) const
{
	return CanSee(Viewer, Target);
}
