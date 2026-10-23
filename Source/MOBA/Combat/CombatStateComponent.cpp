#include "Combat/CombatStateComponent.h"

#include "HeroCombatProfile.h"
#include "MOBACharacter.h"
#include "Net/UnrealNetwork.h"

UCombatStateComponent::UCombatStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCombatStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCombatStateComponent, Health, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCombatStateComponent, MaxHealth, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCombatStateComponent, Gold, COND_OwnerOnly);
}

ELifetimeCondition UCombatStateComponent::GetReplicationCondition() const
{
	// 所属连接是控制这名英雄的玩家。其他单位没有这条连接，组件不发出去。
	if (GetOwner() != nullptr && GetOwner()->IsA<AMOBACharacter>())
	{
		return COND_OwnerOnly;
	}
	return COND_Never;
}

void UCombatStateComponent::InitializeFromPlaceholder(const FCombatPlaceholderStats& InStats)
{
	bPlaceholder = InStats.bPlaceholder;
	Health = InStats.Health;
	MaxHealth = InStats.Health;
	Armor = InStats.Armor;
	MagicResist = InStats.MagicResist;
	OutgoingDamageType = InStats.OutgoingDamageType;
	PhysicalDamage = InStats.PhysicalDamage;
	MagicDamage = InStats.MagicDamage;
	TrueDamage = InStats.TrueDamage;
	Gold = 0.f;
	Experience = 0.f;
	Level = 1;
	MoveSpeedBonus = 0.f;
	bOwnsBoots = false;
	bOwnsAttackItem = false;
	bOwnsHealthItem = false;
	bRespawnPending = false;
	RespawnRemaining = 0.f;
	bRecallPending = false;
	RecallRemaining = 0.f;
	RecallOrigin = FVector::ZeroVector;
	bHealthSampleReady = false;
	SampledHealth = 0.f;
	bAttackStatsReady = false;
}

void UCombatStateComponent::CopyAttackStatsFromProfile(const UHeroCombatProfile* Profile)
{
	if (bAttackStatsReady || Profile == nullptr)
	{
		return;
	}

	// 只读配置上的占位距离、间隔和伤害。以后物品改的是本组件，不写回配置。
	AttackRange = Profile->PlaceholderAttackRange;
	AttackInterval = Profile->PlaceholderAttackInterval;
	OutgoingDamageType = Profile->OutgoingDamageType;
	PhysicalDamage = Profile->PhysicalDamage;
	MagicDamage = Profile->MagicDamage;
	TrueDamage = Profile->TrueDamage;
	bPlaceholder = true;
	bAttackStatsReady = true;
}

float UCombatStateComponent::ApplyDamage(float Amount)
{
	const float Before = Health;
	Health = FMath::Max(0.f, Health - Amount);
	return Before - Health;
}

void UCombatStateComponent::AddGold(float Amount)
{
	Gold += Amount;
}

void UCombatStateComponent::AddExperience(float Amount, float ExperiencePerLevel)
{
	Experience += Amount;
	if (ExperiencePerLevel <= 0.f)
	{
		return;
	}

	// 经验累积过阈值就升级。不发放技能点。
	while (Experience >= Level * ExperiencePerLevel)
	{
		++Level;
	}
}

void UCombatStateComponent::AddOutgoingBonus(ECombatDamageType Type, float Amount)
{
	switch (Type)
	{
	case ECombatDamageType::Magic:
		MagicDamage += Amount;
		break;
	case ECombatDamageType::TrueDamage:
		TrueDamage += Amount;
		break;
	default:
		PhysicalDamage += Amount;
		break;
	}
}

bool UCombatStateComponent::OwnsItem(ECombatShopItem Item) const
{
	switch (Item)
	{
	case ECombatShopItem::Boots:
		return bOwnsBoots;
	case ECombatShopItem::Attack:
		return bOwnsAttackItem;
	case ECombatShopItem::Health:
		return bOwnsHealthItem;
	default:
		return true;
	}
}

bool UCombatStateComponent::TryPurchase(ECombatShopItem Item, const FCombatPlaceholderStats& Stats)
{
	if (OwnsItem(Item))
	{
		return false;
	}

	float Cost = 0.f;
	switch (Item)
	{
	case ECombatShopItem::Boots:
		Cost = Stats.BootsCost;
		break;
	case ECombatShopItem::Attack:
		Cost = Stats.AttackItemCost;
		break;
	case ECombatShopItem::Health:
		Cost = Stats.HealthItemCost;
		break;
	default:
		return false;
	}

	if (Gold < Cost)
	{
		return false;
	}

	Gold -= Cost;
	switch (Item)
	{
	case ECombatShopItem::Boots:
		MoveSpeedBonus += Stats.BootsMoveSpeed;
		bOwnsBoots = true;
		break;
	case ECombatShopItem::Attack:
		AddOutgoingBonus(Stats.AttackItemDamageType, Stats.AttackItemBonus);
		bOwnsAttackItem = true;
		break;
	case ECombatShopItem::Health:
		MaxHealth += Stats.HealthItemBonus;
		Health += Stats.HealthItemBonus;
		bOwnsHealthItem = true;
		break;
	default:
		break;
	}
	return true;
}

void UCombatStateComponent::StartRespawn(float Seconds)
{
	RespawnRemaining = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	bRespawnPending = true;
}

bool UCombatStateComponent::AdvanceRespawn(float DeltaTime)
{
	if (!bRespawnPending)
	{
		return false;
	}

	RespawnRemaining -= DeltaTime;
	if (RespawnRemaining > 0.f)
	{
		return false;
	}

	RespawnRemaining = 0.f;
	bRespawnPending = false;
	return true;
}

void UCombatStateComponent::FillHealth()
{
	Health = MaxHealth;
}

void UCombatStateComponent::HealTowardMax(float Amount)
{
	if (Amount <= 0.f || Health >= MaxHealth)
	{
		return;
	}
	Health = FMath::Min(MaxHealth, Health + Amount);
}

bool UCombatStateComponent::TryBeginRecall(const FVector& Origin, float Seconds)
{
	if (bRecallPending)
	{
		return false;
	}
	RecallOrigin = Origin;
	RecallRemaining = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	bRecallPending = true;
	return true;
}

void UCombatStateComponent::CancelRecall()
{
	bRecallPending = false;
	RecallRemaining = 0.f;
}

bool UCombatStateComponent::AdvanceRecall(float DeltaTime)
{
	if (!bRecallPending)
	{
		return false;
	}
	RecallRemaining -= DeltaTime;
	if (RecallRemaining > 0.f)
	{
		return false;
	}
	RecallRemaining = 0.f;
	bRecallPending = false;
	return true;
}

bool UCombatStateComponent::HasHealthDropped() const
{
	return bHealthSampleReady && Health < SampledHealth;
}

void UCombatStateComponent::RefreshHealthSample()
{
	SampledHealth = Health;
	bHealthSampleReady = true;
}

bool UCombatStateComponent::TryGetAttackRange(float& OutRange) const
{
	if (!bAttackStatsReady)
	{
		return false;
	}
	OutRange = AttackRange;
	return true;
}

bool UCombatStateComponent::TryGetAttackInterval(float& OutInterval) const
{
	if (!bAttackStatsReady)
	{
		return false;
	}
	OutInterval = AttackInterval;
	return true;
}
