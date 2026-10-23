#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lane/LaneTypes.h"
#include "LaneDirector.generated.h"

class ALaneUnit;

/** 只在 LaneTest 上刷兵。占位数值放在这个 Actor 上。 */
UCLASS()
class MOBA_API ALaneDirector : public AActor
{
	GENERATED_BODY()

public:
	ALaneDirector();

	void Configure(const FLanePlaceholderStats& InStats, const TArray<FVector>& InPathTeam1, const TArray<FVector>& InPathTeam2, const TArray<ALaneUnit*>& InSiegeTeam1, const TArray<ALaneUnit*>& InSiegeTeam2);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SpawnBothWaves();
	void SpawnWave(int32 TeamId, const TArray<FVector>& Path, const TArray<TObjectPtr<ALaneUnit>>& SiegeTargets);

	UPROPERTY()
	FLanePlaceholderStats Stats;

	UPROPERTY()
	TArray<FVector> PathTeam1;

	UPROPERTY()
	TArray<FVector> PathTeam2;

	UPROPERTY()
	TArray<TObjectPtr<ALaneUnit>> SiegeTeam1;

	UPROPERTY()
	TArray<TObjectPtr<ALaneUnit>> SiegeTeam2;

	FTimerHandle WaveTimer;
	bool bLoggedPlaceholder = false;
};
