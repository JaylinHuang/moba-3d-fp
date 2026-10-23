#pragma once

#include "Commandlets/Commandlet.h"
#include "CreateLaneTestCommandlet.generated.h"

/** 生成兵线验收图 /Game/Maps/LaneTest。不改 TestGym 和默认地图。 */
UCLASS()
class MOBAEDITOR_API UCreateLaneTestCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UCreateLaneTestCommandlet();

	virtual int32 Main(const FString& Params) override;
};
