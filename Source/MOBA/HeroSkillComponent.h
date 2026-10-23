#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeroCombatProfile.h"
#include "HeroSkillComponent.generated.h"

class AMOBACharacter;

/** 四个技能槽。只做命中扫描、飞行物和自身增益，不改普攻命令，也不发放技能点。 */
UCLASS()
class MOBA_API UHeroSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeroSkillComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void LoadFromProfile(const UHeroCombatProfile* Profile);

	/** 权威端释放。冷却中、已死亡或槽位空着时什么都不做，也不改普攻。 */
	void TryCast(int32 SlotIndex, const FRotator& Aim);

private:
	UPROPERTY()
	TArray<FHeroSkillSlot> Slots;

	float NextReadyTime[4] = {};
	float BaseWalkSpeed = 600.f;
	float BuffRemaining = 0.f;

	/** 复制给客户端，用来改本地移速预测。归零表示增益结束。 */
	UPROPERTY(ReplicatedUsing = OnRep_BuffMove)
	float BuffMoveBonus = 0.f;

	UFUNCTION()
	void OnRep_BuffMove();

	void CastHitScan(AMOBACharacter* Hero, const FHeroSkillSlot& Slot, const FRotator& Aim);
	void CastProjectile(AMOBACharacter* Hero, const FHeroSkillSlot& Slot, const FRotator& Aim);
	void CastSelfBuff(const FHeroSkillSlot& Slot);
	void ApplyWalkSpeed();
	bool IsEnemyVisible(const AMOBACharacter* Hero, AActor* Target) const;
};
