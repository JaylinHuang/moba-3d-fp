#pragma once

#include "Commandlets/Commandlet.h"
#include "CreateTestGymCommandlet.generated.h"

/** 生成平坦测试图 /Game/Maps/TestGym。 */
UCLASS()
class MOBAEDITOR_API UCreateTestGymCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UCreateTestGymCommandlet();

	virtual int32 Main(const FString& Params) override;
};
