#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MatchOverlaySubsystem.generated.h"

class APlayerController;
class UCanvas;

/** 客户端小地图和自己英雄的生命、金币。只读已经在本地的状态。 */
UCLASS()
class MOBA_API UMatchOverlaySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void DrawOverlay(UCanvas* Canvas, APlayerController* PlayerController);

	FDelegateHandle DrawHandle;
};
