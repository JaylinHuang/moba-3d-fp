#include "MOBACharacter.h"

#include "Attackable.h"
#include "BasicAttackProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "CombatStatSource.h"
#include "DamageRequest.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HeroCombatProfile.h"
#include "HeroSkillComponent.h"
#include "HeroVisibilityQuery.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"

AMOBACharacter::AMOBACharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetMesh()->SetOwnerNoSee(true);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("第一人称镜头"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	ProfileClass = UWarriorHeroProfile::StaticClass();
	SkillLoadout = CreateDefaultSubobject<UHeroSkillComponent>(TEXT("技能"));
}

void AMOBACharacter::BeginPlay()
{
	Super::BeginPlay();
	VisibilityQuery = NewObject<UHeroVisibilityQuery>(this);
	if (Profile == nullptr && ProfileClass != nullptr)
	{
		SetHeroProfile(ProfileClass->GetDefaultObject<UHeroCombatProfile>());
	}
	if (AController* OwnedController = GetController())
	{
		StoredMouseLook = OwnedController->GetControlRotation();
	}
}

void AMOBACharacter::SetHeroProfile(const UHeroCombatProfile* InProfile)
{
	Profile = InProfile;
	if (SkillLoadout != nullptr)
	{
		SkillLoadout->LoadFromProfile(Profile);
	}
}

void AMOBACharacter::SetAlive(bool bInAlive)
{
	bAlive = bInAlive;
}

void AMOBACharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled() && (AttackPhase == EBasicAttackPhase::None || bPredictMove))
	{
		ApplyPlayerMovement();
	}

	if (!HasAuthority())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (AttackPhase == EBasicAttackPhase::Approach && LockedTarget != nullptr)
	{
		const FVector Direction = (LockedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		FHitResult Hit;
		AddActorWorldOffset(Direction * GetCharacterMovement()->MaxWalkSpeed * DeltaSeconds, true, &Hit);
		GetCharacterMovement()->Velocity = Direction * GetCharacterMovement()->MaxWalkSpeed;
		AimAtLockedTarget();
	}
	else if (AttackPhase == EBasicAttackPhase::Windup)
	{
		GetCharacterMovement()->StopMovementImmediately();
		AimAtLockedTarget();
	}
	else if (AttackPhase == EBasicAttackPhase::Interval)
	{
		GetCharacterMovement()->StopMovementImmediately();
		AimAtLockedTarget();
	}

	TickAttack(Now);
}

void AMOBACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!IsLocallyControlled())
	{
		return;
	}

	auto MakeAction = [this](EInputActionValueType ValueType) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = ValueType;
		return Action;
	};

	MoveForwardAction = MakeAction(EInputActionValueType::Boolean);
	MoveBackAction = MakeAction(EInputActionValueType::Boolean);
	MoveLeftAction = MakeAction(EInputActionValueType::Boolean);
	MoveRightAction = MakeAction(EInputActionValueType::Boolean);
	LookYawAction = MakeAction(EInputActionValueType::Axis1D);
	LookPitchAction = MakeAction(EInputActionValueType::Axis1D);
	PrimaryAttackAction = MakeAction(EInputActionValueType::Boolean);
	StopAttackAction = MakeAction(EInputActionValueType::Boolean);
	SkillSlotActions.Reset();
	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		SkillSlotActions.Add(MakeAction(EInputActionValueType::Boolean));
	}

	MappingContext = NewObject<UInputMappingContext>(this);
	MappingContext->MapKey(MoveForwardAction, EKeys::W);
	MappingContext->MapKey(MoveBackAction, EKeys::S);
	MappingContext->MapKey(MoveLeftAction, EKeys::A);
	MappingContext->MapKey(MoveRightAction, EKeys::D);
	MappingContext->MapKey(LookYawAction, EKeys::MouseX);
	MappingContext->MapKey(LookPitchAction, EKeys::MouseY);
	MappingContext->MapKey(PrimaryAttackAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(StopAttackAction, EKeys::RightMouseButton);
	MappingContext->MapKey(SkillSlotActions[0], EKeys::One);
	MappingContext->MapKey(SkillSlotActions[1], EKeys::Two);
	MappingContext->MapKey(SkillSlotActions[2], EKeys::Three);
	MappingContext->MapKey(SkillSlotActions[3], EKeys::Four);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				InputSystem->AddMappingContext(MappingContext, 0);
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveForwardAction, ETriggerEvent::Started, this, &AMOBACharacter::OnForwardPressed);
		EnhancedInput->BindAction(MoveForwardAction, ETriggerEvent::Completed, this, &AMOBACharacter::OnForwardReleased);
		EnhancedInput->BindAction(MoveBackAction, ETriggerEvent::Started, this, &AMOBACharacter::OnBackPressed);
		EnhancedInput->BindAction(MoveBackAction, ETriggerEvent::Completed, this, &AMOBACharacter::OnBackReleased);
		EnhancedInput->BindAction(MoveLeftAction, ETriggerEvent::Started, this, &AMOBACharacter::OnLeftPressed);
		EnhancedInput->BindAction(MoveLeftAction, ETriggerEvent::Completed, this, &AMOBACharacter::OnLeftReleased);
		EnhancedInput->BindAction(MoveRightAction, ETriggerEvent::Started, this, &AMOBACharacter::OnRightPressed);
		EnhancedInput->BindAction(MoveRightAction, ETriggerEvent::Completed, this, &AMOBACharacter::OnRightReleased);
		EnhancedInput->BindAction(LookYawAction, ETriggerEvent::Triggered, this, &AMOBACharacter::OnLookYaw);
		EnhancedInput->BindAction(LookPitchAction, ETriggerEvent::Triggered, this, &AMOBACharacter::OnLookPitch);
		EnhancedInput->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &AMOBACharacter::OnPrimaryAttack);
		EnhancedInput->BindAction(StopAttackAction, ETriggerEvent::Started, this, &AMOBACharacter::OnStopAttack);
		EnhancedInput->BindAction(SkillSlotActions[0], ETriggerEvent::Started, this, &AMOBACharacter::OnSkillSlot0);
		EnhancedInput->BindAction(SkillSlotActions[1], ETriggerEvent::Started, this, &AMOBACharacter::OnSkillSlot1);
		EnhancedInput->BindAction(SkillSlotActions[2], ETriggerEvent::Started, this, &AMOBACharacter::OnSkillSlot2);
		EnhancedInput->BindAction(SkillSlotActions[3], ETriggerEvent::Started, this, &AMOBACharacter::OnSkillSlot3);
	}
}

void AMOBACharacter::OnForwardPressed(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Forward, true); }
void AMOBACharacter::OnForwardReleased(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Forward, false); }
void AMOBACharacter::OnBackPressed(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Back, true); }
void AMOBACharacter::OnBackReleased(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Back, false); }
void AMOBACharacter::OnLeftPressed(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Left, true); }
void AMOBACharacter::OnLeftReleased(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Left, false); }
void AMOBACharacter::OnRightPressed(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Right, true); }
void AMOBACharacter::OnRightReleased(const FInputActionValue&) { OnMoveKeyTriggered(EMoveKey::Right, false); }

void AMOBACharacter::OnMoveKeyTriggered(EMoveKey Key, bool bPressed)
{
	if (!HasAuthority())
	{
		ApplyMoveKey(Key, bPressed);
		if (bPressed)
		{
			bPredictMove = true;
		}
	}
	if (!bPressed)
	{
		bPredictMove = false;
	}
	Server_SubmitMoveKey(Key, bPressed);
}

void AMOBACharacter::OnLookYaw(const FInputActionValue& Value)
{
	StoredMouseLook.Yaw += Value.Get<float>() * 0.12f;
	if (AttackPhase == EBasicAttackPhase::None)
	{
		if (AController* OwnedController = GetController())
		{
			OwnedController->SetControlRotation(StoredMouseLook);
		}
	}
	Server_SubmitMouseLook(StoredMouseLook);
}

void AMOBACharacter::OnLookPitch(const FInputActionValue& Value)
{
	StoredMouseLook.Pitch = FMath::Clamp(StoredMouseLook.Pitch + Value.Get<float>() * -0.12f, -80.f, 80.f);
	StoredMouseLook.Roll = 0.f;
	if (AttackPhase == EBasicAttackPhase::None)
	{
		if (AController* OwnedController = GetController())
		{
			OwnedController->SetControlRotation(StoredMouseLook);
		}
	}
	Server_SubmitMouseLook(StoredMouseLook);
}

void AMOBACharacter::OnSkillSlot0() { RequestSkillCast(0); }
void AMOBACharacter::OnSkillSlot1() { RequestSkillCast(1); }
void AMOBACharacter::OnSkillSlot2() { RequestSkillCast(2); }
void AMOBACharacter::OnSkillSlot3() { RequestSkillCast(3); }

void AMOBACharacter::RequestSkillCast(int32 SlotIndex)
{
	const FRotator Aim = GetController() != nullptr ? GetController()->GetControlRotation() : StoredMouseLook;
	Server_CastSkill(SlotIndex, Aim);
}

void AMOBACharacter::OnPrimaryAttack()
{
	const FRotator Aim = AttackPhase == EBasicAttackPhase::None
		? StoredMouseLook
		: (GetController() ? GetController()->GetControlRotation() : StoredMouseLook);
	Server_PrimaryAttack(Aim);
}

void AMOBACharacter::OnStopAttack()
{
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (bKeyDown[Index])
		{
			bSuppressUntilRelease[Index] = true;
		}
	}
	GetCharacterMovement()->StopMovementImmediately();
	Server_StopAttack();
}

void AMOBACharacter::Server_SubmitMoveKey_Implementation(EMoveKey Key, bool bPressed)
{
	ApplyMoveKey(Key, bPressed);
}

void AMOBACharacter::Server_PrimaryAttack_Implementation(FRotator Aim)
{
	BeginAttack(Aim);
}

void AMOBACharacter::Server_CastSkill_Implementation(int32 SlotIndex, FRotator Aim)
{
	if (SkillLoadout != nullptr)
	{
		SkillLoadout->TryCast(SlotIndex, Aim);
	}
}

void AMOBACharacter::Server_StopAttack_Implementation()
{
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (bKeyDown[Index])
		{
			bSuppressUntilRelease[Index] = true;
		}
	}
	if (AttackPhase != EBasicAttackPhase::None)
	{
		EndCommand(true);
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AMOBACharacter::Server_SubmitMouseLook_Implementation(FRotator Look)
{
	Look.Pitch = FMath::Clamp(Look.Pitch, -80.f, 80.f);
	Look.Roll = 0.f;
	StoredMouseLook = Look;
	if (AttackPhase == EBasicAttackPhase::None)
	{
		if (AController* OwnedController = GetController())
		{
			OwnedController->SetControlRotation(StoredMouseLook);
		}
	}
}

void AMOBACharacter::ApplyMoveKey(EMoveKey Key, bool bPressed)
{
	const int32 Index = static_cast<int32>(Key);
	if (bPressed)
	{
		const bool bNewPress = !bKeyDown[Index] && !bSuppressUntilRelease[Index];
		bKeyDown[Index] = true;
		if (bSuppressUntilRelease[Index])
		{
			return;
		}
		bSuppressUntilRelease[Index] = false;
		if (bNewPress && HasAuthority() && AttackPhase != EBasicAttackPhase::None)
		{
			EndCommand(false);
		}
	}
	else
	{
		bKeyDown[Index] = false;
		bSuppressUntilRelease[Index] = false;
	}
}

void AMOBACharacter::ApplyPlayerMovement()
{
	if (AttackPhase == EBasicAttackPhase::Approach || AttackPhase == EBasicAttackPhase::Windup)
	{
		return;
	}

	const float Forward = (bKeyDown[0] && !bSuppressUntilRelease[0] ? 1.f : 0.f)
		- (bKeyDown[1] && !bSuppressUntilRelease[1] ? 1.f : 0.f);
	const float Right = (bKeyDown[3] && !bSuppressUntilRelease[3] ? 1.f : 0.f)
		- (bKeyDown[2] && !bSuppressUntilRelease[2] ? 1.f : 0.f);
	if (FMath::IsNearlyZero(Forward) && FMath::IsNearlyZero(Right))
	{
		return;
	}

	const FRotator YawOnly(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Direction = (FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X) * Forward
		+ FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y) * Right).GetSafeNormal();
	AddMovementInput(Direction);
}

void AMOBACharacter::TickAttack(float Now)
{
	if (AttackPhase == EBasicAttackPhase::None)
	{
		return;
	}
	if (!IsTargetStillValid())
	{
		EndCommand(true);
		return;
	}

	if (AttackPhase == EBasicAttackPhase::Approach)
	{
		if (DistanceToTarget() <= GetAttackRange() && Now >= NextAttackTime)
		{
			BeginWindup(Now);
		}
	}
	else if (AttackPhase == EBasicAttackPhase::Windup)
	{
		if (Now >= WindupEndTime)
		{
			ReleaseAttack(Now);
		}
	}
	else if (AttackPhase == EBasicAttackPhase::Interval && Now >= NextAttackTime)
	{
		if (DistanceToTarget() <= GetAttackRange())
		{
			BeginWindup(Now);
		}
		else
		{
			AttackPhase = EBasicAttackPhase::Approach;
		}
	}
}

void AMOBACharacter::BeginAttack(const FRotator& Aim)
{
	AActor* Target = SelectTarget(Aim);
	if (Target == nullptr)
	{
		return;
	}

	LockedTarget = Target;
	const float Now = GetWorld()->GetTimeSeconds();
	if (DistanceToTarget() <= GetAttackRange() && Now >= NextAttackTime)
	{
		BeginWindup(Now);
	}
	else
	{
		AttackPhase = EBasicAttackPhase::Approach;
		bSwingReleased = false;
	}
	AimAtLockedTarget();
}

void AMOBACharacter::BeginWindup(float Now)
{
	AttackPhase = EBasicAttackPhase::Windup;
	bSwingReleased = false;
	const float Windup = Profile != nullptr ? Profile->PlaceholderWindupDuration : 0.3f;
	WindupEndTime = Now + Windup;
	GetCharacterMovement()->StopMovementImmediately();
}

void AMOBACharacter::ReleaseAttack(float Now)
{
	bSwingReleased = true;
	if (Profile != nullptr && Profile->Delivery == EAttackDelivery::Projectile)
	{
		SpawnProjectile();
	}
	else
	{
		TryMeleeHit();
	}
	NextAttackTime = Now + GetAttackInterval();
	AttackPhase = EBasicAttackPhase::Interval;
}

void AMOBACharacter::EndCommand(bool bStandStill)
{
	const bool bReleased = bSwingReleased;
	AttackPhase = EBasicAttackPhase::None;
	LockedTarget = nullptr;
	bSwingReleased = false;
	if (!bReleased)
	{
		// 这一击没有出手，攻速间隔不开始。
	}
	RestoreMouseLook();
	if (bStandStill)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	else if (bReleased)
	{
		// 已经出手的挥击或飞行物留在场上，不再连下一击。
	}
}

bool AMOBACharacter::IsTargetStillValid() const
{
	if (LockedTarget == nullptr || VisibilityQuery == nullptr)
	{
		return false;
	}
	const IAttackable* Attackable = Cast<IAttackable>(LockedTarget);
	if (Attackable == nullptr || !Attackable->IsAlive())
	{
		return false;
	}
	return VisibilityQuery->IsVisibleTo(this, LockedTarget);
}

AActor* AMOBACharacter::SelectTarget(const FRotator& Aim) const
{
	if (Profile == nullptr || VisibilityQuery == nullptr || FirstPersonCamera == nullptr)
	{
		return nullptr;
	}

	const FVector Origin = FirstPersonCamera->GetComponentLocation();
	const FVector Forward = Aim.Vector();
	AActor* BestTarget = nullptr;
	float BestAngle = Profile->SelectionAngleDegrees;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate == this || !Candidate->Implements<UAttackable>())
		{
			continue;
		}
		const IAttackable* Attackable = Cast<IAttackable>(Candidate);
		if (Attackable == nullptr || !Attackable->IsAlive() || Attackable->GetTeamId() == TeamId)
		{
			continue;
		}
		if (!VisibilityQuery->IsVisibleTo(this, Candidate))
		{
			continue;
		}

		const FVector ToTarget = (Candidate->GetActorLocation() - Origin).GetSafeNormal();
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f)));
		if (Angle <= BestAngle)
		{
			BestAngle = Angle;
			BestTarget = Candidate;
		}
	}
	return BestTarget;
}

float AMOBACharacter::GetAttackRange() const
{
	float CombatValue = 0.f;
	if (TryReadCombatStat(true, CombatValue))
	{
		return CombatValue;
	}
	return Profile != nullptr ? Profile->PlaceholderAttackRange : 0.f;
}

float AMOBACharacter::GetAttackInterval() const
{
	float CombatValue = 0.f;
	if (TryReadCombatStat(false, CombatValue))
	{
		return CombatValue;
	}
	return Profile != nullptr ? Profile->PlaceholderAttackInterval : 1.f;
}

float AMOBACharacter::DistanceToTarget() const
{
	if (LockedTarget == nullptr)
	{
		return TNumericLimits<float>::Max();
	}
	return FVector::Dist(GetActorLocation(), LockedTarget->GetActorLocation());
}

void AMOBACharacter::AimAtLockedTarget()
{
	if (LockedTarget == nullptr || FirstPersonCamera == nullptr)
	{
		return;
	}
	const FRotator LookAt = (LockedTarget->GetActorLocation() - FirstPersonCamera->GetComponentLocation()).Rotation();
	if (AController* OwnedController = GetController())
	{
		OwnedController->SetControlRotation(LookAt);
	}
}

void AMOBACharacter::RestoreMouseLook()
{
	if (AController* OwnedController = GetController())
	{
		OwnedController->SetControlRotation(StoredMouseLook);
	}
}

void AMOBACharacter::TryMeleeHit()
{
	if (LockedTarget == nullptr || DistanceToTarget() > GetAttackRange())
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeSwing), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		GetActorLocation(),
		LockedTarget->GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(36.f),
		Params);
	if (!bHit || Hit.GetActor() != LockedTarget)
	{
		return;
	}

	if (UDamageRequestSubsystem* Sink = GetWorld()->GetSubsystem<UDamageRequestSubsystem>())
	{
		FHeroDamageRequest Request;
		Request.Source = this;
		Request.Target = LockedTarget;
		Request.Delivery = EAttackDelivery::Melee;
		Sink->Submit(Request);
	}
}

void AMOBACharacter::SpawnProjectile()
{
	if (LockedTarget == nullptr || Profile == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	const FVector SpawnLocation = FirstPersonCamera != nullptr
		? FirstPersonCamera->GetComponentLocation() + GetControlRotation().Vector() * 40.f
		: GetActorLocation();
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABasicAttackProjectile* Projectile = GetWorld()->SpawnActor<ABasicAttackProjectile>(ABasicAttackProjectile::StaticClass(), SpawnLocation, GetControlRotation(), Params))
	{
		Projectile->Launch(this, LockedTarget, Profile->PlaceholderProjectileSpeed);
	}
}

bool AMOBACharacter::TryReadCombatStat(bool bRange, float& OutValue) const
{
	TInlineComponentArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component != nullptr && Component->Implements<UCombatStatSource>())
		{
			const ICombatStatSource* Source = Cast<ICombatStatSource>(Component);
			if (Source == nullptr)
			{
				continue;
			}
			return bRange ? Source->TryGetAttackRange(OutValue) : Source->TryGetAttackInterval(OutValue);
		}
	}
	return false;
}

void AMOBACharacter::OnRep_AttackPhase()
{
	if (AttackPhase == EBasicAttackPhase::None)
	{
		bPredictMove = false;
	}
}

void AMOBACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMOBACharacter, AttackPhase);
	DOREPLIFETIME(AMOBACharacter, LockedTarget);
	DOREPLIFETIME(AMOBACharacter, bAlive);
}
