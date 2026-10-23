#include "Lane/LaneProps.h"

#include "Components/BoxComponent.h"
#include "Lane/LaneQueries.h"
#include "UObject/ConstructorHelpers.h"

ALaneFountain::ALaneFountain()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("泉水"));
	SetRootComponent(Body);
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Body->SetStaticMesh(SphereMesh.Object);
	}
	SetActorScale3D(FVector(0.8f));
}

void ALaneFountain::Setup(int32 InTeamId)
{
	TeamId = InTeamId;
}

void ALaneFountain::BeginPlay()
{
	Super::BeginPlay();
	if (Body == nullptr)
	{
		return;
	}
	const FLinearColor Color = TeamId == 1 ? FLinearColor(0.4f, 0.8f, 1.f) : FLinearColor(1.f, 0.55f, 0.4f);
	if (UMaterialInstanceDynamic* Material = Body->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

ALaneSlot::ALaneSlot()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("路位"));
	SetRootComponent(Body);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Body->SetStaticMesh(CubeMesh.Object);
	}
	SetActorScale3D(FVector(0.4f, 0.4f, 0.15f));
}

void ALaneSlot::Setup(int32 InLaneNumber)
{
	LaneNumber = InLaneNumber;
}

ALaneBrush::ALaneBrush()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("草丛体积"));
	SetRootComponent(Volume);
	Volume->SetBoxExtent(FVector(500.f, 400.f, 200.f));
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Overlap);
	Volume->SetGenerateOverlapEvents(true);
}

void ALaneBrush::BeginPlay()
{
	Super::BeginPlay();
	Volume->OnComponentBeginOverlap.AddDynamic(this, &ALaneBrush::OnVolumeBeginOverlap);
	Volume->OnComponentEndOverlap.AddDynamic(this, &ALaneBrush::OnVolumeEndOverlap);
}

bool ALaneBrush::Contains(const AActor* Actor) const
{
	return Occupants.Contains(Actor);
}

void ALaneBrush::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != nullptr && OtherActor != this)
	{
		Occupants.AddUnique(OtherActor);
	}
}

void ALaneBrush::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Occupants.Remove(OtherActor);
}

ALaneMonster::ALaneMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	Kind = ELaneUnitKind::Monster;
	TeamId = 0;
	SetActorScale3D(FVector(0.7f));
}

void ALaneMonster::Activate(const FVector& InHome, float InRadius, float InMoveSpeed)
{
	Home = InHome;
	HomeRadius = InRadius;
	MoveSpeed = InMoveSpeed;
	SetTeamAndKind(0, ELaneUnitKind::Monster);
	SetActorLocation(FVector(InHome.X, InHome.Y, GetActorLocation().Z));
}

void ALaneMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !IsAlive() || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
	{
		return;
	}
	if (LaneHorizontalDistance(GetActorLocation(), Home) > HomeRadius)
	{
		MoveToward(Home, MoveSpeed, DeltaSeconds);
	}
}

ALaneCamp::ALaneCamp()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("营地"));
	SetRootComponent(Marker);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Marker->SetStaticMesh(CylinderMesh.Object);
	}
	SetActorScale3D(FVector(1.4f, 1.4f, 0.08f));
}

void ALaneCamp::Setup(const FLanePlaceholderStats& InStats)
{
	Stats = InStats;
}

void ALaneCamp::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && GetWorld() != nullptr && GetWorld()->IsGameWorld())
	{
		SpawnMonsters();
	}
}

void ALaneCamp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !bHasSpawned || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
	{
		return;
	}

	if (CountAlive() > 0)
	{
		RespawnRemaining = -1.f;
		return;
	}

	if (RespawnRemaining < 0.f)
	{
		RespawnRemaining = Stats.CampRespawnSeconds;
	}
	RespawnRemaining -= DeltaSeconds;
	if (RespawnRemaining <= 0.f)
	{
		SpawnMonsters();
	}
}

void ALaneCamp::SpawnMonsters()
{
	Monsters.Reset();
	RespawnRemaining = -1.f;
	bHasSpawned = true;
	if (GetWorld() == nullptr)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	for (int32 Index = 0; Index < Stats.MonsterCount; ++Index)
	{
		const float Side = Index % 2 == 0 ? -120.f : 120.f;
		const FVector SpawnLocation(Origin.X + Side, Origin.Y, 70.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ALaneMonster* Monster = GetWorld()->SpawnActor<ALaneMonster>(ALaneMonster::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Monster->Activate(Origin, Stats.CampRadius, Stats.MonsterMoveSpeed);
			Monsters.Add(Monster);
		}
	}
}

int32 ALaneCamp::CountAlive() const
{
	int32 Alive = 0;
	for (const TObjectPtr<ALaneMonster>& Monster : Monsters)
	{
		if (Monster != nullptr && Monster->IsAlive())
		{
			++Alive;
		}
	}
	return Alive;
}
