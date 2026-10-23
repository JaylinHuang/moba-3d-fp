#include "Presentation/MatchOverlaySubsystem.h"

#include "CanvasItem.h"
#include "Combat/CombatStateComponent.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Lane/LaneTypes.h"
#include "Lane/LaneUnit.h"
#include "MOBACharacter.h"
#include "Presentation/MatchMapPlaceholder.h"

namespace
{
	const FLinearColor TeamOneColor(0.25f, 0.5f, 1.f, 1.f);
	const FLinearColor TeamTwoColor(1.f, 0.28f, 0.22f, 1.f);
	const FLinearColor OtherTeamColor(0.75f, 0.75f, 0.75f, 1.f);
	const FLinearColor MapFill(0.02f, 0.03f, 0.05f, 0.72f);
	const FLinearColor MapBorder(0.85f, 0.85f, 0.85f, 0.9f);
	const FLinearColor LocalRing(1.f, 0.92f, 0.35f, 1.f);

	FLinearColor ColorForTeam(int32 TeamId)
	{
		if (TeamId == 1)
		{
			return TeamOneColor;
		}
		if (TeamId == 2)
		{
			return TeamTwoColor;
		}
		return OtherTeamColor;
	}

	/** 兵线种类决定点的大小，英雄另算，这样种类能看出来。 */
	float LaneMarkerSize(ELaneUnitKind Kind)
	{
		switch (Kind)
		{
		case ELaneUnitKind::MeleeMinion:
			return 4.f;
		case ELaneUnitKind::RangedMinion:
			return 3.f;
		case ELaneUnitKind::Monster:
			return 5.f;
		case ELaneUnitKind::OuterTower:
			return 7.f;
		case ELaneUnitKind::InnerTower:
			return 8.f;
		case ELaneUnitKind::Base:
			return 10.f;
		default:
			return 4.f;
		}
	}

	void DrawFilled(UCanvas* Canvas, float X, float Y, float Size, const FLinearColor& Color)
	{
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(Size, Size), Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}

	/** 把世界坐标投进占位矩形，出界就夹在地图边缘。 */
	FVector2D ProjectToMap(const FVector& WorldLocation, float MapX, float MapY, float MapW, float MapH, float Marker)
	{
		const float SpanX = FMatchMapPlaceholder::MaxX - FMatchMapPlaceholder::MinX;
		const float SpanY = FMatchMapPlaceholder::MaxY - FMatchMapPlaceholder::MinY;
		const float NormX = (WorldLocation.X - FMatchMapPlaceholder::MinX) / SpanX;
		const float NormY = (WorldLocation.Y - FMatchMapPlaceholder::MinY) / SpanY;
		const float Half = Marker * 0.5f;
		const float PixelX = MapX + FMath::Clamp(NormX, 0.f, 1.f) * MapW;
		const float PixelY = MapY + (1.f - FMath::Clamp(NormY, 0.f, 1.f)) * MapH;
		return FVector2D(
			FMath::Clamp(PixelX, MapX + Half, MapX + MapW - Half) - Half,
			FMath::Clamp(PixelY, MapY + Half, MapY + MapH - Half) - Half);
	}

	void DrawMarker(UCanvas* Canvas, float X, float Y, float Size, const FLinearColor& Color, bool bAlive, bool bLocalHero)
	{
		if (bAlive)
		{
			DrawFilled(Canvas, X, Y, Size, Color);
		}
		else
		{
			Canvas->K2_DrawBox(FVector2D(X, Y), FVector2D(Size, Size), 1.5f, Color);
		}

		if (bLocalHero)
		{
			Canvas->K2_DrawBox(FVector2D(X - 2.f, Y - 2.f), FVector2D(Size + 4.f, Size + 4.f), 1.5f, LocalRing);
		}
	}
}

bool UMatchOverlaySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr || !World->IsGameWorld())
	{
		return false;
	}
	return World->GetNetMode() != NM_DedicatedServer;
}

void UMatchOverlaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	DrawHandle = UDebugDrawService::Register(
		TEXT("Game"),
		FDebugDrawDelegate::CreateUObject(this, &UMatchOverlaySubsystem::DrawOverlay));
}

void UMatchOverlaySubsystem::Deinitialize()
{
	if (DrawHandle.IsValid())
	{
		UDebugDrawService::Unregister(DrawHandle);
		DrawHandle.Reset();
	}
	Super::Deinitialize();
}

void UMatchOverlaySubsystem::DrawOverlay(UCanvas* Canvas, APlayerController* PlayerController)
{
	if (Canvas == nullptr || Canvas->Canvas == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UFont* Font = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
	const AMOBACharacter* LocalHero = PlayerController != nullptr
		? Cast<AMOBACharacter>(PlayerController->GetPawn())
		: nullptr;

	if (Font != nullptr)
	{
		const UCombatStateComponent* State = LocalHero != nullptr
			? LocalHero->FindComponentByClass<UCombatStateComponent>()
			: nullptr;
		if (State != nullptr)
		{
			Canvas->K2_DrawText(
				Font,
				FString::Printf(TEXT("生命 %.0f / %.0f"), State->GetHealth(), State->GetMaxHealth()),
				FVector2D(24.f, 24.f),
				FVector2D(1.1f, 1.1f),
				FLinearColor::White);
			Canvas->K2_DrawText(
				Font,
				FString::Printf(TEXT("金币 %.0f"), State->GetGold()),
				FVector2D(24.f, 46.f),
				FVector2D(1.1f, 1.1f),
				FLinearColor::White);
		}
		else
		{
			Canvas->K2_DrawText(Font, TEXT("生命"), FVector2D(24.f, 24.f), FVector2D(1.1f, 1.1f), FLinearColor(1.f, 1.f, 1.f, 0.45f));
			Canvas->K2_DrawText(Font, TEXT("金币"), FVector2D(24.f, 46.f), FVector2D(1.1f, 1.1f), FLinearColor(1.f, 1.f, 1.f, 0.45f));
		}
	}

	const float MapW = 220.f;
	const float MapH = 110.f;
	const float MapX = 24.f;
	const float MapY = FMath::Max(24.f, Canvas->SizeY - MapH - 24.f);
	FCanvasTileItem MapTile(FVector2D(MapX, MapY), FVector2D(MapW, MapH), MapFill);
	MapTile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(MapTile);
	Canvas->K2_DrawBox(FVector2D(MapX, MapY), FVector2D(MapW, MapH), 1.f, MapBorder);

	for (TActorIterator<ALaneUnit> It(World); It; ++It)
	{
		const ALaneUnit* Unit = *It;
		if (Unit == nullptr)
		{
			continue;
		}
		const float Size = LaneMarkerSize(Unit->GetLaneKind());
		const FVector2D Origin = ProjectToMap(Unit->GetActorLocation(), MapX, MapY, MapW, MapH, Size);
		DrawMarker(Canvas, Origin.X, Origin.Y, Size, ColorForTeam(Unit->GetTeamId()), Unit->IsAlive(), false);
	}

	for (TActorIterator<AMOBACharacter> It(World); It; ++It)
	{
		const AMOBACharacter* Hero = *It;
		if (Hero == nullptr)
		{
			continue;
		}
		const float Size = 12.f;
		const FVector2D Origin = ProjectToMap(Hero->GetActorLocation(), MapX, MapY, MapW, MapH, Size);
		DrawMarker(Canvas, Origin.X, Origin.Y, Size, ColorForTeam(Hero->GetTeamId()), Hero->IsAlive(), Hero == LocalHero);
	}
}
