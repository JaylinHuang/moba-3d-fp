#include "Lane/LaneBase.h"

#include "Lane/LaneMatchSubsystem.h"

ALaneBase::ALaneBase()
{
	SetActorScale3D(FVector(2.2f, 2.2f, 2.2f));
	Kind = ELaneUnitKind::Base;
}

void ALaneBase::Setup(int32 InTeamId)
{
	SetTeamAndKind(InTeamId, ELaneUnitKind::Base);
}

void ALaneBase::NotifyDestroyed()
{
	if (!HasAuthority() || !bAlive)
	{
		return;
	}

	bAlive = false;
	ForceNetUpdate();
	if (ULaneMatchSubsystem* Match = GetWorld() != nullptr ? GetWorld()->GetSubsystem<ULaneMatchSubsystem>() : nullptr)
	{
		Match->NotifyBaseDestroyed(TeamId);
	}
}
