#include "CreateLaneTestCommandlet.h"

#include "FileHelpers.h"
#include "Lane/LaneLayout.h"

UCreateLaneTestCommandlet::UCreateLaneTestCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UCreateLaneTestCommandlet::Main(const FString& Params)
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (World == nullptr)
	{
		return 1;
	}

	FLaneLayout::Ensure(World);
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, TEXT("/Game/Maps/LaneTest"));
	return bSaved ? 0 : 1;
}
