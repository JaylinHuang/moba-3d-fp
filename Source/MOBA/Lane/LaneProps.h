#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lane/LaneTypes.h"
#include "Lane/LaneUnit.h"
#include "LaneProps.generated.h"

class ALaneCamp;

/** 泉水。不是小兵会锁死的建筑。 */
UCLASS()
class MOBA_API ALaneFountain : public AActor
{
	GENERATED_BODY()

public:
	ALaneFountain();
	void Setup(int32 InTeamId);
	/** 现有队伍编号。战斗只读，用来分辨己方泉水。 */
	int32 GetTeamId() const { return TeamId; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	int32 TeamId = 1;
};

/** 另外两条路只留生成位置，本版不放建筑。 */
UCLASS()
class MOBA_API ALaneSlot : public AActor
{
	GENERATED_BODY()

public:
	ALaneSlot();
	void Setup(int32 InLaneNumber);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	int32 LaneNumber = 2;
};

/** 草丛体积。只记录进出，不修改可见性查询。 */
UCLASS()
class MOBA_API ALaneBrush : public AActor
{
	GENERATED_BODY()

public:
	ALaneBrush();
	bool Contains(const AActor* Actor) const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UBoxComponent> Volume;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Occupants;
};

/** 守在营地里的野怪。不走小兵的呼救和建筑锁。 */
UCLASS()
class MOBA_API ALaneMonster : public ALaneUnit
{
	GENERATED_BODY()

public:
	ALaneMonster();
	void Activate(const FVector& InHome, float InRadius, float InMoveSpeed);
	virtual void Tick(float DeltaSeconds) override;

private:
	FVector Home = FVector::ZeroVector;
	float HomeRadius = 650.f;
	float MoveSpeed = 300.f;
};

/** 打野营地。野怪离开后走回，全部不可攻击后按占位间隔刷新。 */
UCLASS()
class MOBA_API ALaneCamp : public AActor
{
	GENERATED_BODY()

public:
	ALaneCamp();
	void Setup(const FLanePlaceholderStats& InStats);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SpawnMonsters();
	int32 CountAlive() const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Marker;

	UPROPERTY()
	FLanePlaceholderStats Stats;

	UPROPERTY()
	TArray<TObjectPtr<ALaneMonster>> Monsters;

	float RespawnRemaining = -1.f;
	bool bHasSpawned = false;
};
