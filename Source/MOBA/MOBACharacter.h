#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Attackable.h"
#include "MOBACharacter.generated.h"

class UCameraComponent;
class UHeroCombatProfile;
class UHeroSkillComponent;
class UHeroVisibilityQuery;
class UInputAction;
class UInputMappingContext;

UENUM()
enum class EMoveKey : uint8
{
	Forward,
	Back,
	Left,
	Right
};

UENUM()
enum class EBasicAttackPhase : uint8
{
	None,
	Approach,
	Windup,
	Interval
};

/** 第一人称英雄。移动和普通攻击命令在专用服务器上推进。 */
UCLASS()
class MOBA_API AMOBACharacter : public ACharacter, public IAttackable
{
	GENERATED_BODY()

public:
	AMOBACharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual int32 GetTeamId() const override { return TeamId; }
	virtual bool IsAlive() const override { return bAlive; }

	void SetHeroProfile(const UHeroCombatProfile* InProfile);

	/** 战斗结算写入存活。不改移动、镜头和普攻。 */
	void SetAlive(bool bInAlive);

	/** 当前战斗配置。战斗模块只读，不写回。 */
	const UHeroCombatProfile* GetHeroCombatProfile() const { return Profile; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "镜头")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(EditDefaultsOnly, Category = "英雄")
	TSubclassOf<UHeroCombatProfile> ProfileClass;

	UPROPERTY(EditAnywhere, Category = "队伍")
	int32 TeamId = 1;

	UPROPERTY(Replicated, EditAnywhere, Category = "队伍")
	bool bAlive = true;

	/** 四个技能槽。命中扫描、飞行物和自身增益。不改普攻命令。 */
	UPROPERTY(VisibleAnywhere, Category = "技能")
	TObjectPtr<UHeroSkillComponent> SkillLoadout;

	UPROPERTY(ReplicatedUsing = OnRep_AttackPhase)
	EBasicAttackPhase AttackPhase = EBasicAttackPhase::None;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> LockedTarget;

	const UHeroCombatProfile* Profile = nullptr;
	TObjectPtr<UHeroVisibilityQuery> VisibilityQuery;

	bool bKeyDown[4] = {};
	bool bSuppressUntilRelease[4] = {};
	FRotator StoredMouseLook = FRotator::ZeroRotator;
	float WindupEndTime = 0.f;
	float NextAttackTime = 0.f;
	bool bSwingReleased = false;
	bool bPredictMove = false;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveForwardAction;
	UPROPERTY()
	TObjectPtr<UInputAction> MoveBackAction;
	UPROPERTY()
	TObjectPtr<UInputAction> MoveLeftAction;
	UPROPERTY()
	TObjectPtr<UInputAction> MoveRightAction;
	UPROPERTY()
	TObjectPtr<UInputAction> LookYawAction;
	UPROPERTY()
	TObjectPtr<UInputAction> LookPitchAction;
	UPROPERTY()
	TObjectPtr<UInputAction> PrimaryAttackAction;
	UPROPERTY()
	TObjectPtr<UInputAction> StopAttackAction;
	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> SkillSlotActions;
	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext;

	UFUNCTION()
	void OnRep_AttackPhase();

	void OnForwardPressed(const struct FInputActionValue& Value);
	void OnForwardReleased(const struct FInputActionValue& Value);
	void OnBackPressed(const struct FInputActionValue& Value);
	void OnBackReleased(const struct FInputActionValue& Value);
	void OnLeftPressed(const struct FInputActionValue& Value);
	void OnLeftReleased(const struct FInputActionValue& Value);
	void OnRightPressed(const struct FInputActionValue& Value);
	void OnRightReleased(const struct FInputActionValue& Value);
	void OnMoveKeyTriggered(EMoveKey Key, bool bPressed);
	void OnLookYaw(const struct FInputActionValue& Value);
	void OnLookPitch(const struct FInputActionValue& Value);
	void OnPrimaryAttack();
	void OnStopAttack();
	void OnSkillSlot0();
	void OnSkillSlot1();
	void OnSkillSlot2();
	void OnSkillSlot3();
	void RequestSkillCast(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void Server_SubmitMoveKey(EMoveKey Key, bool bPressed);

	UFUNCTION(Server, Reliable)
	void Server_PrimaryAttack(FRotator Aim);

	UFUNCTION(Server, Reliable)
	void Server_StopAttack();

	UFUNCTION(Server, Reliable)
	void Server_CastSkill(int32 SlotIndex, FRotator Aim);

	UFUNCTION(Server, Unreliable)
	void Server_SubmitMouseLook(FRotator Look);

	void ApplyMoveKey(EMoveKey Key, bool bPressed);
	void ApplyPlayerMovement();
	void TickAttack(float Now);
	void BeginAttack(const FRotator& Aim);
	void BeginWindup(float Now);
	void ReleaseAttack(float Now);
	void EndCommand(bool bStandStill);
	bool IsTargetStillValid() const;
	AActor* SelectTarget(const FRotator& Aim) const;
	float GetAttackRange() const;
	float GetAttackInterval() const;
	float DistanceToTarget() const;
	void AimAtLockedTarget();
	void RestoreMouseLook();
	void TryMeleeHit();
	void SpawnProjectile();
	bool TryReadCombatStat(bool bRange, float& OutValue) const;
};
