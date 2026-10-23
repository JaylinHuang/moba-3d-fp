#pragma once

#include "CoreMinimal.h"
#include "VisionPlaceholder.generated.h"

/** 视野占位。数字不写进玩法规则，也不写入共享配置。 */
USTRUCT()
struct FVisionPlaceholder
{
	GENERATED_BODY()

	/** 标明这些是占位，正式手感以后再调。 */
	UPROPERTY(EditAnywhere, Category = "占位")
	bool bPlaceholder = true;

	/** 个人视线的水平距离，单位厘米。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "1"))
	float SightRadius = 1500.f;

	/** 草丛中出手或受伤后的显形时长，单位秒。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float RevealSeconds = 1.f;

	/** 视线检测相对脚底的抬高，用来避开地板。 */
	UPROPERTY(EditAnywhere, Category = "占位", meta = (ClampMin = "0"))
	float TraceLift = 50.f;
};
