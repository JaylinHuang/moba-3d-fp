#include "AttackDummy.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAttackDummy::AAttackDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("身体"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("Pawn"));
	Body->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Body->SetStaticMesh(CubeMesh.Object);
	}
	SetActorScale3D(FVector(0.6f, 0.6f, 1.6f));
}

void AAttackDummy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAttackDummy, bAlive);
}
