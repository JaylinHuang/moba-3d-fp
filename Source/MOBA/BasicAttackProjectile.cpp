#include "BasicAttackProjectile.h"

#include "Components/SphereComponent.h"
#include "DamageRequest.h"
#include "MOBACharacter.h"
#include "Net/UnrealNetwork.h"

ABasicAttackProjectile::ABasicAttackProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("碰撞"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(16.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ABasicAttackProjectile::OnOverlap);

	InitialLifeSpan = 5.f;
}

void ABasicAttackProjectile::Launch(AMOBACharacter* InSource, AActor* InTarget, float Speed)
{
	SourceHero = InSource;
	LockedTarget = InTarget;
	SetOwner(InSource);
	if (InSource)
	{
		SetInstigator(InSource);
	}

	const FVector Direction = (InTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetActorRotation(Direction.Rotation());
	FlightSpeed = Speed;
	Collision->IgnoreActorWhenMoving(InSource, true);
}

void ABasicAttackProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || LockedTarget == nullptr)
	{
		return;
	}

	const FVector Direction = GetActorForwardVector();
	FHitResult Hit;
	AddActorWorldOffset(Direction * FlightSpeed * DeltaSeconds, true, &Hit);
	if (Hit.GetActor() == LockedTarget)
	{
		SubmitDamage();
	}
	else if (Hit.bBlockingHit)
	{
		Destroy();
	}
}

void ABasicAttackProjectile::SubmitDamage()
{
	if (!HasAuthority() || bDamageSubmitted)
	{
		return;
	}

	bDamageSubmitted = true;
	if (UDamageRequestSubsystem* Sink = GetWorld()->GetSubsystem<UDamageRequestSubsystem>())
	{
		FHeroDamageRequest Request;
		Request.Source = SourceHero;
		Request.Target = LockedTarget;
		Request.Delivery = EAttackDelivery::Projectile;
		Sink->Submit(Request);
	}
	Destroy();
}

void ABasicAttackProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == LockedTarget)
	{
		SubmitDamage();
	}
}

void ABasicAttackProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABasicAttackProjectile, LockedTarget);
	DOREPLIFETIME(ABasicAttackProjectile, SourceHero);
}
