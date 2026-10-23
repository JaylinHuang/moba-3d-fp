#include "HeroGym.h"

#include "AttackDummy.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

void FHeroGymSpawner::Ensure(UWorld* World)
{
	if (World == nullptr)
	{
		return;
	}

	bool bHasFloor = false;
	bool bHasDummy = false;
	bool bHasStart = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TestGymFloor")))
		{
			bHasFloor = true;
		}
		if (It->IsA<AAttackDummy>())
		{
			bHasDummy = true;
		}
		if (It->IsA<APlayerStart>())
		{
			bHasStart = true;
		}
	}

	if (!bHasFloor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
		{
			if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
			{
				Floor->GetStaticMeshComponent()->SetStaticMesh(Plane);
			}
			Floor->SetActorScale3D(FVector(40.f, 40.f, 1.f));
			Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
			Floor->Tags.Add(TEXT("TestGymFloor"));
		}
	}

	if (!bHasStart)
	{
		World->SpawnActor<APlayerStart>(FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator);
	}

	if (!bHasDummy)
	{
		World->SpawnActor<AAttackDummy>(FVector(450.f, 0.f, 90.f), FRotator::ZeroRotator);
	}

	bool bHasLight = false;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		bHasLight = true;
		break;
	}
	if (!bHasLight)
	{
		World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 400.f), FRotator(-45.f, 40.f, 0.f));
	}
}
