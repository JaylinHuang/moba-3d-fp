#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MOBAGameMode.generated.h"

UCLASS()
class MOBA_API AMOBAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMOBAGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
