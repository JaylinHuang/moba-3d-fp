#include "Lane/LaneLayout.h"

#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Lane/LaneBase.h"
#include "Lane/LaneDirector.h"
#include "Lane/LaneProps.h"
#include "Lane/LaneTower.h"

namespace
{
	AStaticMeshActor* SpawnFloor(UWorld* World, const FVector& Location, const FVector& Scale)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
		if (Floor == nullptr)
		{
			return nullptr;
		}
		if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			Floor->GetStaticMeshComponent()->SetStaticMesh(Plane);
		}
		Floor->SetActorScale3D(Scale);
		Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
		Floor->Tags.Add(TEXT("LaneTestFloor"));
		return Floor;
	}

	template <typename T>
	T* SpawnAt(UWorld* World, const FVector& Location)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<T>(T::StaticClass(), Location, FRotator::ZeroRotator, Params);
	}
}

void FLaneLayout::Ensure(UWorld* World)
{
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("LaneTestLayout")))
		{
			return;
		}
	}

	const float LaneY = 2500.f;
	SpawnFloor(World, FVector(0.f, 3000.f, 0.f), FVector(100.f, 50.f, 1.f));

	const FLanePlaceholderStats Stats;
	ALaneDirector* Director = SpawnAt<ALaneDirector>(World, FVector(0.f, LaneY, 200.f));

	ALaneFountain* Fountain1 = SpawnAt<ALaneFountain>(World, FVector(-4300.f, LaneY, 80.f));
	ALaneFountain* Fountain2 = SpawnAt<ALaneFountain>(World, FVector(4300.f, LaneY, 80.f));
	if (Fountain1 != nullptr)
	{
		Fountain1->Setup(1);
	}
	if (Fountain2 != nullptr)
	{
		Fountain2->Setup(2);
	}

	ALaneBase* Base1 = SpawnAt<ALaneBase>(World, FVector(-3600.f, LaneY, 110.f));
	ALaneBase* Base2 = SpawnAt<ALaneBase>(World, FVector(3600.f, LaneY, 110.f));
	if (Base1 != nullptr)
	{
		Base1->Setup(1);
	}
	if (Base2 != nullptr)
	{
		Base2->Setup(2);
	}

	ALaneTower* Inner1 = SpawnAt<ALaneTower>(World, FVector(-2400.f, LaneY, 120.f));
	ALaneTower* Outer1 = SpawnAt<ALaneTower>(World, FVector(-1200.f, LaneY, 120.f));
	ALaneTower* Outer2 = SpawnAt<ALaneTower>(World, FVector(1200.f, LaneY, 120.f));
	ALaneTower* Inner2 = SpawnAt<ALaneTower>(World, FVector(2400.f, LaneY, 120.f));
	if (Inner1 != nullptr)
	{
		Inner1->Setup(Stats, 1, ELaneUnitKind::InnerTower);
	}
	if (Outer1 != nullptr)
	{
		Outer1->Setup(Stats, 1, ELaneUnitKind::OuterTower);
	}
	if (Outer2 != nullptr)
	{
		Outer2->Setup(Stats, 2, ELaneUnitKind::OuterTower);
	}
	if (Inner2 != nullptr)
	{
		Inner2->Setup(Stats, 2, ELaneUnitKind::InnerTower);
	}

	TArray<FVector> PathTeam1;
	TArray<FVector> PathTeam2;
	for (int32 Step = 0; Step <= 8; ++Step)
	{
		PathTeam1.Add(FVector(-3400.f + Step * 850.f, LaneY, 80.f));
	}
	for (int32 Index = PathTeam1.Num() - 1; Index >= 0; --Index)
	{
		PathTeam2.Add(PathTeam1[Index]);
	}

	if (Director != nullptr)
	{
		TArray<ALaneUnit*> SiegeTeam1;
		SiegeTeam1.Add(Outer2);
		SiegeTeam1.Add(Inner2);
		SiegeTeam1.Add(Base2);
		TArray<ALaneUnit*> SiegeTeam2;
		SiegeTeam2.Add(Outer1);
		SiegeTeam2.Add(Inner1);
		SiegeTeam2.Add(Base1);
		Director->Configure(Stats, PathTeam1, PathTeam2, SiegeTeam1, SiegeTeam2);
	}

	if (ALaneCamp* Camp = SpawnAt<ALaneCamp>(World, FVector(0.f, 4800.f, 20.f)))
	{
		Camp->Setup(Stats);
	}
	SpawnAt<ALaneBrush>(World, FVector(800.f, 1400.f, 100.f));
	if (ALaneSlot* Top = SpawnAt<ALaneSlot>(World, FVector(0.f, 5200.f, 40.f)))
	{
		Top->Setup(2);
	}
	if (ALaneSlot* Bottom = SpawnAt<ALaneSlot>(World, FVector(0.f, 800.f, 40.f)))
	{
		Bottom->Setup(3);
	}
}
