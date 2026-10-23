#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vision/VisionPlaceholder.h"
#include "VisionWorldSubsystem.generated.h"

class ABasicAttackProjectile;
class ALaneBrush;
struct FVisionEndOfFrameTickFunction;

/** 服务器上的一份队伍视野。英雄和兵线的查询都读这里。 */
UCLASS()
class MOBA_API UVisionWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FVisionPlaceholder& GetPlaceholder() const { return Placeholder; }

	/** 观察者队伍是否看得见目标。客户端不推断草丛，只承认已经在本地的双方。 */
	bool IsVisibleTo(const AActor* Viewer, const AActor* Target);

	/** 帧后段收集显形。由本子系统自己的 tick 调用。 */
	void HandleEndOfFrame();

private:
	struct FCachedUnit
	{
		TWeakObjectPtr<AActor> Actor;
		int32 TeamId = INDEX_NONE;
		bool bProvidesVision = false;
		TArray<TWeakObjectPtr<ALaneBrush>> Brushes;
	};

	struct FRevealRecord
	{
		TWeakObjectPtr<AActor> Actor;
		int32 TeamId = INDEX_NONE;
		double UntilTime = 0.0;
	};

	struct FProjectileReveal
	{
		TWeakObjectPtr<ABasicAttackProjectile> Projectile;
		int32 TeamId = INDEX_NONE;
		double UntilTime = 0.0;
	};

	void EnsureFrameData();
	void CollectBrushesOf(const AActor* Actor, TArray<ALaneBrush*>& OutBrushes) const;
	bool BrushAllows(const TArray<TWeakObjectPtr<ALaneBrush>>& ProviderBrushes, const TArray<ALaneBrush*>& TargetBrushes) const;
	bool HasStaticLineOfSight(const AActor* From, const AActor* To) const;
	bool HasPersonalSight(const FCachedUnit& Provider, const AActor* Target) const;
	bool AnyProviderSees(int32 TeamId, const AActor* Target) const;
	bool IsRevealedTo(const AActor* Actor, int32 TeamId) const;
	bool IsProjectileRevealedTo(const ABasicAttackProjectile* Projectile, int32 TeamId) const;
	int32 TeamOf(const AActor* Actor) const;
	bool ProvidesVision(const AActor* Actor) const;
	void GatherEnemyTeams(int32 TeamId, TArray<int32, TInlineAllocator<2>>& OutTeams) const;
	void TouchReveal(AActor* Actor, int32 TeamId, double UntilTime);
	void RevealActorToTeams(AActor* Actor, const TArray<int32, TInlineAllocator<2>>& Teams, double UntilTime);
	void ConsumeDamageFeed();
	void ScanProjectiles();
	void PruneExpired();
	bool IsProjectileVisibleTo(const AActor* Viewer, const ABasicAttackProjectile* Projectile);

	UPROPERTY()
	FVisionPlaceholder Placeholder;

	FVisionEndOfFrameTickFunction* EndOfFrameTick = nullptr;

	uint64 CachedFrame = MAX_uint64;
	TArray<TWeakObjectPtr<ALaneBrush>> Brushes;
	TArray<FCachedUnit> Units;
	TMap<const AActor*, int32> UnitIndex;

	TArray<FRevealRecord> Reveals;
	TArray<FProjectileReveal> ProjectileReveals;
	TArray<TWeakObjectPtr<ABasicAttackProjectile>> SeenProjectiles;

	bool bLoggedPlaceholder = false;
};
