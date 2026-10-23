#include "CreateTestGymCommandlet.h"

#include "FileHelpers.h"
#include "HeroGym.h"

UCreateTestGymCommandlet::UCreateTestGymCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UCreateTestGymCommandlet::Main(const FString& Params)
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (World == nullptr)
	{
		return 1;
	}

	FHeroGymSpawner::Ensure(World);
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, TEXT("/Game/Maps/TestGym"));
	return bSaved ? 0 : 1;
}
