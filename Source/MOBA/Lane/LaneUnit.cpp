#include "Lane/LaneUnit.h"

#include "Lane/LaneQueries.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ALaneUnit::ALaneUnit()
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
}

void ALaneUnit::BeginPlay()
{
	Super::BeginPlay();
	ApplyColor();
}

void ALaneUnit::SetTeamAndKind(int32 InTeamId, ELaneUnitKind InKind)
{
	TeamId = InTeamId;
	Kind = InKind;
	ApplyColor();
}

void ALaneUnit::ApplyColor()
{
	if (Body == nullptr)
	{
		return;
	}

	FLinearColor Color(0.6f, 0.45f, 0.2f);
	if (TeamId == 1)
	{
		Color = FLinearColor(0.2f, 0.4f, 0.9f);
	}
	else if (TeamId == 2)
	{
		Color = FLinearColor(0.85f, 0.2f, 0.15f);
	}
	if (LaneIsStructureKind(Kind))
	{
		Color *= 0.75f;
	}

	if (UMaterialInstanceDynamic* Material = Body->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ALaneUnit::MoveToward(const FVector& WorldPoint, float Speed, float DeltaSeconds)
{
	const FVector Current = GetActorLocation();
	const FVector Desired(WorldPoint.X, WorldPoint.Y, Current.Z);
	const FVector Offset = Desired - Current;
	if (Offset.SizeSquared2D() < 16.f)
	{
		return;
	}

	const FVector Step = Offset.GetSafeNormal2D() * Speed * DeltaSeconds;
	FVector NewLocation = Current + Step;
	if (Step.SizeSquared2D() >= Offset.SizeSquared2D())
	{
		NewLocation = Desired;
	}
	SetActorLocation(NewLocation);
	SetActorRotation(Offset.GetSafeNormal2D().Rotation());
}

bool ALaneUnit::TryAttack(AActor* Target, float Range, float Interval, EAttackDelivery Delivery)
{
	if (Target == nullptr || GetWorld() == nullptr)
	{
		return false;
	}
	if (LaneHorizontalDistance(GetActorLocation(), Target->GetActorLocation()) > Range)
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextAttackTime)
	{
		LaneSubmitDamage(GetWorld(), this, Target, Delivery);
		NextAttackTime = Now + Interval;
	}
	return true;
}

void ALaneUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALaneUnit, TeamId);
	DOREPLIFETIME(ALaneUnit, Kind);
	DOREPLIFETIME(ALaneUnit, bAlive);
}
