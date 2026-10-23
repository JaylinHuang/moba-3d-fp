#include "SkillShot.h"

#include "Attackable.h"
#include "Components/SphereComponent.h"
#include "DamageRequest.h"
#include "HeroVisibilityQuery.h"
#include "MOBACharacter.h"

ASkillShot::ASkillShot()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("碰撞"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(16.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASkillShot::OnOverlap);

	InitialLifeSpan = 5.f;
}

void ASkillShot::Launch(AMOBACharacter* InSource, const FVector& Direction, float Speed, float InMaxDistance)
{
	SourceHero = InSource;
	SetOwner(InSource);
	if (InSource)
	{
		SetInstigator(InSource);
		Collision->IgnoreActorWhenMoving(InSource, true);
	}

	const FVector FlatDirection = Direction.GetSafeNormal();
	SetActorRotation(FlatDirection.Rotation());
	FlightSpeed = Speed;
	MaxDistance = InMaxDistance;
}

void ASkillShot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || bDamageSubmitted)
	{
		return;
	}

	const float Step = FlightSpeed * DeltaSeconds;
	FHitResult Hit;
	AddActorWorldOffset(GetActorForwardVector() * Step, true, &Hit);
	Traveled += Step;
	if (Hit.bBlockingHit)
	{
		Destroy();
		return;
	}
	if (Traveled >= MaxDistance)
	{
		Destroy();
	}
}

void ASkillShot::SubmitDamage(AActor* Target)
{
	if (!HasAuthority() || bDamageSubmitted || SourceHero == nullptr || Target == nullptr)
	{
		return;
	}

	bDamageSubmitted = true;
	if (UDamageRequestSubsystem* Sink = GetWorld()->GetSubsystem<UDamageRequestSubsystem>())
	{
		FHeroDamageRequest Request;
		Request.Source = SourceHero;
		Request.Target = Target;
		Request.Delivery = EAttackDelivery::Projectile;
		Sink->Submit(Request);
	}
	Destroy();
}

void ASkillShot::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bDamageSubmitted || OtherActor == nullptr || OtherActor == SourceHero || OtherActor == GetOwner())
	{
		return;
	}
	if (!OtherActor->Implements<UAttackable>())
	{
		return;
	}

	const IAttackable* Attackable = Cast<IAttackable>(OtherActor);
	if (Attackable == nullptr || !Attackable->IsAlive() || SourceHero == nullptr || Attackable->GetTeamId() == SourceHero->GetTeamId())
	{
		return;
	}

	if (!UHeroVisibilityQuery::CanSee(SourceHero, OtherActor))
	{
		return;
	}

	SubmitDamage(OtherActor);
}
