#include "Combat/CombatResolutionSubsystem.h"

#include "AttackDummy.h"
#include "Combat/CombatStateComponent.h"
#include "DamageRequest.h"
#include "EngineUtils.h"
#include "Lane/LaneBase.h"
#include "Lane/LaneMinion.h"
#include "Lane/LaneProps.h"
#include "Lane/LaneQueries.h"
#include "Lane/LaneTower.h"
#include "MOBACharacter.h"

/** 物理或魔法：max(1, 伤害 × (1 - 抗性 / (100 + 抗性)))。分母接近 0 时不除。 */
static float MitigatedDamage(float Amount, float Resist)
{
	const float Denominator = 100.f + Resist;
	if (FMath::IsNearlyZero(Denominator))
	{
		return FMath::Max(1.f, Amount);
	}
	return FMath::Max(1.f, Amount * (1.f - Resist / Denominator));
}

bool UCombatResolutionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}

bool UCombatResolutionSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

TStatId UCombatResolutionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatResolutionSubsystem, STATGROUP_Tickables);
}

void UCombatResolutionSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_Client || !World->IsGameWorld())
	{
		return;
	}

	MatchElapsed += DeltaTime;

	// 英雄还没挨打也要有钱包，自然金币不依赖第一击。
	for (TActorIterator<AMOBACharacter> It(World); It; ++It)
	{
		AMOBACharacter* Hero = *It;
		UCombatStateComponent* State = EnsureState(Hero);
		const bool bHealthDropped = State->HasHealthDropped();
		if (Hero->IsAlive())
		{
			State->AddGold(Placeholder.PassiveGoldPerSecond * DeltaTime);
			TickRecall(Hero, State, DeltaTime, bHealthDropped);
			if (IsInsideAlliedFountain(Hero))
			{
				State->HealTowardMax(Placeholder.FountainHealPerSecond * DeltaTime);
			}
		}
		else
		{
			if (State->IsRecallPending())
			{
				State->CancelRecall();
			}
			if (State->AdvanceRespawn(DeltaTime))
			{
				State->FillHealth();
				Hero->SetAlive(true);
				if (ALaneFountain* Fountain = FindAlliedFountain(Hero))
				{
					// 只写位置。不改移动组件速度，也不改普攻状态。
					Hero->SetActorLocation(Fountain->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
					UE_LOG(LogTemp, Log, TEXT("英雄复活，落到己方泉水。英雄=%s"), *GetNameSafe(Hero));
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("英雄复活，读不到己方泉水，位置保持不变。英雄=%s"), *GetNameSafe(Hero));
				}
			}
		}
		State->RefreshHealthSample();
	}
}

void UCombatResolutionSubsystem::Resolve(const FHeroDamageRequest& Request)
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_Client || !World->IsGameWorld())
	{
		return;
	}

	AActor* Source = Request.Source;
	AActor* Target = Request.Target;
	if (Source == nullptr || Target == nullptr || !LaneIsAlive(Target))
	{
		return;
	}
	if (LaneTeamId(Source) == LaneTeamId(Target))
	{
		return;
	}

	UCombatStateComponent* SourceState = EnsureState(Source);
	UCombatStateComponent* TargetState = EnsureState(Target);
	float Damage = 0.f;
	switch (SourceState->GetOutgoingDamageType())
	{
	case ECombatDamageType::Magic:
		Damage = MitigatedDamage(SourceState->GetMagicDamage(), TargetState->GetMagicResist());
		break;
	case ECombatDamageType::TrueDamage:
		// 真实伤害按配置原值计入，不减护甲、魔抗，也不套最低为 1。
		Damage = SourceState->GetTrueDamage();
		break;
	default:
		Damage = MitigatedDamage(SourceState->GetPhysicalDamage(), TargetState->GetArmor());
		break;
	}
	if (Source->IsA<AMOBACharacter>())
	{
		if (const ALaneTower* Tower = Cast<ALaneTower>(Target))
		{
			Damage *= Tower->GetIncomingHeroDamageScale();
		}
	}

	const float Applied = TargetState->ApplyDamage(Damage);
	if (Applied <= 0.f || TargetState->GetHealth() > 0.f)
	{
		return;
	}

	WriteDeath(Target);
	GrantDeathRewards(Source, Target);
}

bool UCombatResolutionSubsystem::TryBuy(AMOBACharacter* Hero, ECombatShopItem Item)
{
	UWorld* World = GetWorld();
	if (Hero == nullptr || World == nullptr || World->GetNetMode() == NM_Client || !World->IsGameWorld())
	{
		return false;
	}
	if (!Hero->IsAlive())
	{
		return false;
	}
	// 半径外、敌方泉水或读不到己方泉水都不扣金币。
	if (!IsInsideAlliedFountain(Hero))
	{
		return false;
	}

	UCombatStateComponent* State = EnsureState(Hero);
	const float GoldBefore = State->GetGold();
	if (!State->TryPurchase(Item, Placeholder))
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("商店购买。英雄=%s 金币 %.0f -> %.0f"), *GetNameSafe(Hero), GoldBefore, State->GetGold());
	return true;
}

bool UCombatResolutionSubsystem::TryStartRecall(AMOBACharacter* Hero)
{
	UWorld* World = GetWorld();
	if (Hero == nullptr || World == nullptr || World->GetNetMode() == NM_Client || !World->IsGameWorld())
	{
		return false;
	}
	if (!Hero->IsAlive())
	{
		return false;
	}

	UCombatStateComponent* State = EnsureState(Hero);
	if (State->IsRecallPending())
	{
		return false;
	}
	if (FindAlliedFountain(Hero) == nullptr)
	{
		return false;
	}

	const FVector Origin = Hero->GetActorLocation();
	if (!State->TryBeginRecall(Origin, Placeholder.RecallChannelSeconds))
	{
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("开始回城。英雄=%s"), *GetNameSafe(Hero));
	return true;
}

ALaneFountain* UCombatResolutionSubsystem::FindAlliedFountain(const AMOBACharacter* Hero) const
{
	UWorld* World = GetWorld();
	if (Hero == nullptr || World == nullptr)
	{
		return nullptr;
	}

	ALaneFountain* Nearest = nullptr;
	float NearestDistance = 0.f;
	for (TActorIterator<ALaneFountain> It(World); It; ++It)
	{
		ALaneFountain* Fountain = *It;
		if (Fountain->GetTeamId() != Hero->GetTeamId())
		{
			continue;
		}
		const float Distance = LaneHorizontalDistance(Hero->GetActorLocation(), Fountain->GetActorLocation());
		if (Nearest == nullptr || Distance < NearestDistance)
		{
			Nearest = Fountain;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

bool UCombatResolutionSubsystem::IsInsideAlliedFountain(const AMOBACharacter* Hero) const
{
	UWorld* World = GetWorld();
	if (Hero == nullptr || World == nullptr)
	{
		return false;
	}

	for (TActorIterator<ALaneFountain> It(World); It; ++It)
	{
		const ALaneFountain* Fountain = *It;
		if (Fountain->GetTeamId() != Hero->GetTeamId())
		{
			continue;
		}
		if (LaneHorizontalDistance(Hero->GetActorLocation(), Fountain->GetActorLocation()) <= Placeholder.FountainRadius)
		{
			return true;
		}
	}
	return false;
}

void UCombatResolutionSubsystem::TickRecall(AMOBACharacter* Hero, UCombatStateComponent* State, float DeltaTime, bool bHealthDropped)
{
	if (!State->IsRecallPending())
	{
		return;
	}

	const float Moved = LaneHorizontalDistance(Hero->GetActorLocation(), State->GetRecallOrigin());
	if (bHealthDropped || Moved > Placeholder.RecallMoveTolerance)
	{
		State->CancelRecall();
		UE_LOG(LogTemp, Log, TEXT("回城取消。英雄=%s"), *GetNameSafe(Hero));
		return;
	}

	if (!State->AdvanceRecall(DeltaTime))
	{
		return;
	}

	ALaneFountain* Fountain = FindAlliedFountain(Hero);
	if (Fountain == nullptr || !Hero->IsAlive())
	{
		return;
	}

	// 只写位置。不改移动组件速度，也不改普攻状态。
	Hero->SetActorLocation(Fountain->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogTemp, Log, TEXT("回城完成。英雄=%s"), *GetNameSafe(Hero));
}

float UCombatResolutionSubsystem::RespawnSeconds() const
{
	const float Minutes = MatchElapsed / 60.f;
	return Placeholder.RespawnBaseSeconds + Placeholder.RespawnGrowthPerMinute * Minutes;
}

UCombatStateComponent* UCombatResolutionSubsystem::EnsureState(AActor* Actor)
{
	UCombatStateComponent* State = Actor->FindComponentByClass<UCombatStateComponent>();
	if (State == nullptr)
	{
		State = NewObject<UCombatStateComponent>(Actor, TEXT("战斗状态"));
		State->RegisterComponent();
		Actor->AddInstanceComponent(State);
		State->InitializeFromPlaceholder(Placeholder);
	}

	if (AMOBACharacter* Hero = Cast<AMOBACharacter>(Actor))
	{
		State->CopyAttackStatsFromProfile(Hero->GetHeroCombatProfile());
	}
	return State;
}

void UCombatResolutionSubsystem::WriteDeath(AActor* Target)
{
	if (ALaneBase* Base = Cast<ALaneBase>(Target))
	{
		Base->NotifyDestroyed();
		return;
	}
	if (ALaneUnit* Unit = Cast<ALaneUnit>(Target))
	{
		Unit->SetAlive(false);
		return;
	}
	if (AAttackDummy* Dummy = Cast<AAttackDummy>(Target))
	{
		Dummy->SetAlive(false);
		return;
	}
	if (AMOBACharacter* Hero = Cast<AMOBACharacter>(Target))
	{
		Hero->SetAlive(false);
		EnsureState(Hero)->CancelRecall();
		EnsureState(Hero)->StartRespawn(RespawnSeconds());
	}
}

void UCombatResolutionSubsystem::GrantDeathRewards(AActor* Source, AActor* Target)
{
	const ALaneMinion* Minion = Cast<ALaneMinion>(Target);
	const bool bLaneMinion = Minion != nullptr
		&& (Minion->GetLaneKind() == ELaneUnitKind::MeleeMinion || Minion->GetLaneKind() == ELaneUnitKind::RangedMinion);
	if (bLaneMinion)
	{
		if (AMOBACharacter* Killer = Cast<AMOBACharacter>(Source))
		{
			EnsureState(Killer)->AddGold(Placeholder.MinionGold);
		}
		GrantSharedExperience(Source, Target, nullptr, Placeholder.MinionExperience);
		return;
	}

	if (Cast<AMOBACharacter>(Target) != nullptr)
	{
		AMOBACharacter* Killer = Cast<AMOBACharacter>(Source);
		if (Killer != nullptr)
		{
			EnsureState(Killer)->AddGold(Placeholder.HeroKillGold);
		}
		GrantSharedExperience(Source, Target, Killer, Placeholder.HeroKillExperience);
	}
}

void UCombatResolutionSubsystem::GrantSharedExperience(const AActor* Source, const AActor* Origin, AMOBACharacter* AlwaysInclude, float Amount)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Origin == nullptr || Amount <= 0.f)
	{
		return;
	}

	const int32 Team = LaneTeamId(Source);
	if (AlwaysInclude != nullptr)
	{
		EnsureState(AlwaysInclude)->AddExperience(Amount, Placeholder.ExperiencePerLevel);
	}

	for (TActorIterator<AMOBACharacter> It(World); It; ++It)
	{
		AMOBACharacter* Hero = *It;
		if (Hero == AlwaysInclude || !Hero->IsAlive() || Hero->GetTeamId() != Team)
		{
			continue;
		}
		if (LaneHorizontalDistance(Hero->GetActorLocation(), Origin->GetActorLocation()) > Placeholder.ShareRadius)
		{
			continue;
		}
		EnsureState(Hero)->AddExperience(Amount, Placeholder.ExperiencePerLevel);
	}
}
