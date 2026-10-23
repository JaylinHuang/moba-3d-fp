#pragma once

#include "CoreMinimal.h"
#include "CombatPlaceholder.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatResolutionSubsystem.generated.h"

class AActor;
class ALaneFountain;
class AMOBACharacter;
struct FHeroDamageRequest;
class UCombatStateComponent;

/** 权威端在伤害请求追加进本拍列表之后结算生命、金币和经验。 */
UCLASS()
class MOBA_API UCombatResolutionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	/** 结算这一条请求。只在权威端改生命和钱包。 */
	void Resolve(const FHeroDamageRequest& Request);

	/**
	 * 权威端购买固定商品。
	 * 泉水队伍还没有只读口，这一刀不按位置拒绝。
	 * 返回 false 时金币和属性都不变。
	 */
	bool TryBuy(AMOBACharacter* Hero, ECombatShopItem Item);

	/**
	 * 权威端开始回城读条。
	 * 失败时不改位置，也不重置已经在走的读条。
	 */
	bool TryStartRecall(AMOBACharacter* Hero);

private:
	UCombatStateComponent* EnsureState(AActor* Actor);
	void GrantDeathRewards(AActor* Source, AActor* Target);
	void GrantSharedExperience(const AActor* Source, const AActor* Origin, AMOBACharacter* AlwaysInclude, float Amount);
	void WriteDeath(AActor* Target);

	float RespawnSeconds() const;
	ALaneFountain* FindAlliedFountain(const AMOBACharacter* Hero) const;
	bool IsInsideAlliedFountain(const AMOBACharacter* Hero) const;
	void TickRecall(AMOBACharacter* Hero, UCombatStateComponent* State, float DeltaTime, bool bHealthDropped);

	UPROPERTY()
	FCombatPlaceholderStats Placeholder;

	/** 本子系统累计的对局秒数。复活时长用它，不读兵线比赛状态。 */
	UPROPERTY()
	float MatchElapsed = 0.f;
};
