#pragma once

#include "CoreMinimal.h"
#include "DamageRequest.h"
#include "Subsystems/WorldSubsystem.h"
#include "LaneDamageFeed.generated.h"

class UHeroVisibilityQuery;

/** 这一拍里的一条伤害请求，按广播先后编号。 */
USTRUCT()
struct FLaneDamageNotice
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY()
	EAttackDelivery Delivery = EAttackDelivery::Melee;

	UPROPERTY()
	int32 Order = 0;
};

/**
 * 本拍伤害流水。塔要等这一拍收齐再选最近的英雄，所以不能在广播到达时立刻锁定。
 * 统筹窗口登记前，UDamageRequestSubsystem::Submit 还不会调用 Push。
 * 登记后应在写入最后一条请求之前调用本子系统的 Push。
 */
UCLASS()
class MOBA_API ULaneDamageFeedSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void BeginFrame();
	void Push(const FHeroDamageRequest& Request);
	const TArray<FLaneDamageNotice>& GetFrame() const { return Notices; }
	bool IsVisibleTo(const AActor* Viewer, const AActor* Target) const;

private:
	UPROPERTY()
	TArray<FLaneDamageNotice> Notices;

	int32 NextOrder = 0;

	UPROPERTY()
	TObjectPtr<UHeroVisibilityQuery> Visibility;
};
