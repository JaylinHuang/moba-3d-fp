#pragma once

#include "CoreMinimal.h"
#include "HeroCombatProfile.h"
#include "Subsystems/WorldSubsystem.h"
#include "DamageRequest.generated.h"

/** 交给战斗模块之前的占位伤害请求。接收方不改生命和金钱。 */
USTRUCT(BlueprintType)
struct FHeroDamageRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "伤害")
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "伤害")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "伤害")
	EAttackDelivery Delivery = EAttackDelivery::Melee;
};

UCLASS()
class MOBA_API UDamageRequestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Submit(const FHeroDamageRequest& Request);

	const FHeroDamageRequest& GetLastRequest() const { return LastRequest; }
	int32 GetSubmitCount() const { return SubmitCount; }

private:
	UPROPERTY()
	FHeroDamageRequest LastRequest;

	int32 SubmitCount = 0;
};
