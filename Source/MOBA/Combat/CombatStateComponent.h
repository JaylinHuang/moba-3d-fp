#pragma once

#include "CoreMinimal.h"
#include "CombatPlaceholder.h"
#include "CombatStatSource.h"
#include "Components/ActorComponent.h"
#include "CombatStateComponent.generated.h"

class UHeroCombatProfile;

/** 一个可攻击单位的生命、护甲、魔抗和三种伤害。英雄另外保存金币、经验和出手数值。 */
UCLASS()
class MOBA_API UCombatStateComponent : public UActorComponent, public ICombatStatSource
{
	GENERATED_BODY()

public:
	UCombatStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** 英雄的组件只跟所属连接走。小兵、野怪和塔的组件不复制。 */
	virtual ELifetimeCondition GetReplicationCondition() const override;

	void InitializeFromPlaceholder(const FCombatPlaceholderStats& InStats);
	/** 从英雄配置抄攻击距离、出手间隔、伤害类型和三种伤害。已抄过，或配置为空时不动。不写回配置。 */
	void CopyAttackStatsFromProfile(const UHeroCombatProfile* Profile);

	ECombatDamageType GetOutgoingDamageType() const { return OutgoingDamageType; }
	float GetPhysicalDamage() const { return PhysicalDamage; }
	float GetMagicDamage() const { return MagicDamage; }
	float GetTrueDamage() const { return TrueDamage; }
	float GetArmor() const { return Armor; }
	float GetMagicResist() const { return MagicResist; }
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetGold() const { return Gold; }
	float GetExperience() const { return Experience; }
	int32 GetLevel() const { return Level; }
	float GetMoveSpeedBonus() const { return MoveSpeedBonus; }

	/** 扣生命。返回扣掉的数量。 */
	float ApplyDamage(float Amount);
	void AddGold(float Amount);
	void AddExperience(float Amount, float ExperiencePerLevel);

	/**
	 * 权威端购买。活着与否由调用方判断。
	 * 金币不够或已经拥有时，金币和属性都不变。
	 * 鞋子只记下移速加成，不写角色移动组件。
	 */
	bool TryPurchase(ECombatShopItem Item, const FCombatPlaceholderStats& Stats);

	/** 英雄死亡后开始计时。秒数由调用方按对局时间算好。 */
	void StartRespawn(float Seconds);
	bool IsRespawnPending() const { return bRespawnPending; }
	/** 减少剩余时间。到零时清掉计时并返回 true。 */
	bool AdvanceRespawn(float DeltaTime);
	/** 复活时把当前生命补到上限。 */
	void FillHealth();
	/** 往上限补生命。已满或增量不是正数时保持不变。 */
	void HealTowardMax(float Amount);

	bool IsRecallPending() const { return bRecallPending; }
	FVector GetRecallOrigin() const { return RecallOrigin; }
	/** 已在读条时返回 false，不重置剩余时间。 */
	bool TryBeginRecall(const FVector& Origin, float Seconds);
	void CancelRecall();
	/** 减少剩余时间。到零时清掉读条并返回 true。 */
	bool AdvanceRecall(float DeltaTime);
	/** 当前生命低于上一拍采样时为真。 */
	bool HasHealthDropped() const;
	void RefreshHealthSample();

	virtual bool TryGetAttackRange(float& OutRange) const override;
	virtual bool TryGetAttackInterval(float& OutInterval) const override;

private:
	UPROPERTY()
	bool bPlaceholder = true;

	/** 只复制给这名英雄所属的连接。 */
	UPROPERTY(Replicated)
	float Health = 0.f;

	/** 只复制给这名英雄所属的连接。 */
	UPROPERTY(Replicated)
	float MaxHealth = 0.f;

	UPROPERTY()
	float Armor = 0.f;

	UPROPERTY()
	float MagicResist = 0.f;

	UPROPERTY()
	ECombatDamageType OutgoingDamageType = ECombatDamageType::Physical;

	UPROPERTY()
	float PhysicalDamage = 0.f;

	UPROPERTY()
	float MagicDamage = 0.f;

	UPROPERTY()
	float TrueDamage = 0.f;

	/** 只复制给这名英雄所属的连接，不发给其他连接。 */
	UPROPERTY(Replicated)
	float Gold = 0.f;

	UPROPERTY()
	float Experience = 0.f;

	UPROPERTY()
	int32 Level = 1;

	UPROPERTY()
	float AttackRange = 0.f;

	UPROPERTY()
	float AttackInterval = 1.f;

	/** 鞋子记下的移速加成。本组件不写移动组件。 */
	UPROPERTY()
	float MoveSpeedBonus = 0.f;

	UPROPERTY()
	bool bOwnsBoots = false;

	UPROPERTY()
	bool bOwnsAttackItem = false;

	UPROPERTY()
	bool bOwnsHealthItem = false;

	UPROPERTY()
	bool bRespawnPending = false;

	UPROPERTY()
	float RespawnRemaining = 0.f;

	UPROPERTY()
	bool bRecallPending = false;

	UPROPERTY()
	float RecallRemaining = 0.f;

	UPROPERTY()
	FVector RecallOrigin = FVector::ZeroVector;

	bool bHealthSampleReady = false;
	float SampledHealth = 0.f;

	bool bAttackStatsReady = false;

	bool OwnsItem(ECombatShopItem Item) const;
	/** 把商品加成记到配置指定的那一种伤害上。 */
	void AddOutgoingBonus(ECombatDamageType Type, float Amount);
};
