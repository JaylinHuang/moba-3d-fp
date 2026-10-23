#include "HeroSkillComponent.h"

#include "Attackable.h"
#include "Camera/CameraComponent.h"
#include "DamageRequest.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HeroVisibilityQuery.h"
#include "MOBACharacter.h"
#include "Net/UnrealNetwork.h"
#include "SkillShot.h"

UHeroSkillComponent::UHeroSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UHeroSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			BaseWalkSpeed = Movement->MaxWalkSpeed;
		}
	}
}

void UHeroSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority() || BuffRemaining <= 0.f)
	{
		return;
	}

	BuffRemaining = FMath::Max(0.f, BuffRemaining - DeltaTime);
	if (BuffRemaining <= 0.f)
	{
		BuffMoveBonus = 0.f;
		ApplyWalkSpeed();
	}
}

void UHeroSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHeroSkillComponent, BuffMoveBonus);
}

void UHeroSkillComponent::OnRep_BuffMove()
{
	ApplyWalkSpeed();
}

void UHeroSkillComponent::LoadFromProfile(const UHeroCombatProfile* Profile)
{
	Slots.Reset();
	if (Profile == nullptr)
	{
		return;
	}

	const int32 Count = FMath::Min(Profile->SkillSlots.Num(), 4);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Slots.Add(Profile->SkillSlots[Index]);
	}
}

void UHeroSkillComponent::TryCast(int32 SlotIndex, const FRotator& Aim)
{
	AMOBACharacter* Hero = Cast<AMOBACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (Hero == nullptr || World == nullptr || !Hero->HasAuthority() || !Hero->IsAlive())
	{
		return;
	}
	if (!Slots.IsValidIndex(SlotIndex) || SlotIndex < 0 || SlotIndex >= 4)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now < NextReadyTime[SlotIndex])
	{
		return;
	}

	const FHeroSkillSlot& Slot = Slots[SlotIndex];
	switch (Slot.Form)
	{
	case EHeroSkillForm::Projectile:
		CastProjectile(Hero, Slot, Aim);
		break;
	case EHeroSkillForm::SelfBuff:
		CastSelfBuff(Slot);
		break;
	default:
		CastHitScan(Hero, Slot, Aim);
		break;
	}

	// 释放即进冷却。打空也算出手。升级不在这里发技能点。
	NextReadyTime[SlotIndex] = Now + Slot.Cooldown;
	UE_LOG(LogTemp, Log, TEXT("技能已释放，槽位=%d。不改普攻命令，不发放技能点。"), SlotIndex);
}

void UHeroSkillComponent::CastHitScan(AMOBACharacter* Hero, const FHeroSkillSlot& Slot, const FRotator& Aim)
{
	UWorld* World = Hero->GetWorld();
	if (World == nullptr || Slot.Range <= 0.f)
	{
		return;
	}

	const UCameraComponent* Camera = Hero->FindComponentByClass<UCameraComponent>();
	const FVector Start = Camera != nullptr ? Camera->GetComponentLocation() : Hero->GetActorLocation();
	const FVector End = Start + Aim.Vector() * Slot.Range;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillHitScan), false, Hero);
	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(36.f),
		Params);
	if (!bHit)
	{
		return;
	}

	AActor* Target = Hit.GetActor();
	if (!IsEnemyVisible(Hero, Target))
	{
		return;
	}

	if (UDamageRequestSubsystem* Sink = World->GetSubsystem<UDamageRequestSubsystem>())
	{
		FHeroDamageRequest Request;
		Request.Source = Hero;
		Request.Target = Target;
		Request.Delivery = EAttackDelivery::Melee;
		Sink->Submit(Request);
	}
}

void UHeroSkillComponent::CastProjectile(AMOBACharacter* Hero, const FHeroSkillSlot& Slot, const FRotator& Aim)
{
	UWorld* World = Hero->GetWorld();
	if (World == nullptr || Slot.Range <= 0.f || Slot.ProjectileSpeed <= 0.f)
	{
		return;
	}

	const UCameraComponent* Camera = Hero->FindComponentByClass<UCameraComponent>();
	const FVector Start = Camera != nullptr
		? Camera->GetComponentLocation() + Aim.Vector() * 40.f
		: Hero->GetActorLocation();
	FActorSpawnParameters Params;
	Params.Owner = Hero;
	Params.Instigator = Hero;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ASkillShot* Shot = World->SpawnActor<ASkillShot>(ASkillShot::StaticClass(), Start, Aim, Params))
	{
		Shot->Launch(Hero, Aim.Vector(), Slot.ProjectileSpeed, Slot.Range);
	}
}

void UHeroSkillComponent::CastSelfBuff(const FHeroSkillSlot& Slot)
{
	BuffRemaining = Slot.BuffDuration;
	BuffMoveBonus = Slot.BuffMoveSpeed;
	ApplyWalkSpeed();
}

void UHeroSkillComponent::ApplyWalkSpeed()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	const float Bonus = GetOwner() != nullptr && GetOwner()->HasAuthority() && BuffRemaining <= 0.f ? 0.f : BuffMoveBonus;
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed + Bonus;
}

bool UHeroSkillComponent::IsEnemyVisible(const AMOBACharacter* Hero, AActor* Target) const
{
	if (Hero == nullptr || Target == nullptr || Target == Hero || !Target->Implements<UAttackable>())
	{
		return false;
	}

	const IAttackable* Attackable = Cast<IAttackable>(Target);
	if (Attackable == nullptr || !Attackable->IsAlive() || Attackable->GetTeamId() == Hero->GetTeamId())
	{
		return false;
	}

	return UHeroVisibilityQuery::CanSee(Hero, Target);
}
