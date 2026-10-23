#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Attackable.h"
#include "BasicAttackProjectile.generated.h"

class AMOBACharacter;

/** 法师和射手的飞行物。生成时不提交伤害，击中锁定目标时提交一次。 */
UCLASS()
class MOBA_API ABasicAttackProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABasicAttackProjectile();

	void Launch(AMOBACharacter* InSource, AActor* InTarget, float Speed);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class USphereComponent> Collision;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> LockedTarget;

	UPROPERTY(Replicated)
	TObjectPtr<AMOBACharacter> SourceHero;

	float FlightSpeed = 1800.f;
	bool bDamageSubmitted = false;

	void SubmitDamage();

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
