#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkillShot.generated.h"

class AMOBACharacter;

/** 技能飞行物。生成时不提交伤害，第一次命中可见敌人时提交一次。 */
UCLASS()
class MOBA_API ASkillShot : public AActor
{
	GENERATED_BODY()

public:
	ASkillShot();

	void Launch(AMOBACharacter* InSource, const FVector& Direction, float Speed, float MaxDistance);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class USphereComponent> Collision;

	UPROPERTY()
	TObjectPtr<AMOBACharacter> SourceHero;

	float FlightSpeed = 1400.f;
	float MaxDistance = 800.f;
	float Traveled = 0.f;
	bool bDamageSubmitted = false;

	void SubmitDamage(AActor* Target);

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
