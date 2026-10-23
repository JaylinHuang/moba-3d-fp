#include "MOBAGameMode.h"

#include "HeroGym.h"
#include "MOBACharacter.h"

AMOBAGameMode::AMOBAGameMode()
{
	DefaultPawnClass = AMOBACharacter::StaticClass();
}

void AMOBAGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	FHeroGymSpawner::Ensure(GetWorld());
}
