#include "Vision/VisionReplicationGraph.h"

#include "BasicAttackProjectile.h"
#include "Lane/LaneUnit.h"
#include "MOBACharacter.h"
#include "UObject/UObjectIterator.h"
#include "Vision/VisionWorldSubsystem.h"

namespace
{
	/** 这条连接用来做视野判断的英雄。优先看当前视点，否则用控制器上的 Pawn。 */
	const AMOBACharacter* ConnectionHero(const FNetViewer& Viewer)
	{
		if (const AMOBACharacter* ViewHero = Cast<AMOBACharacter>(Viewer.ViewTarget))
		{
			return ViewHero;
		}
		const AController* Controller = Cast<AController>(Viewer.InViewer);
		return Controller != nullptr ? Cast<AMOBACharacter>(Controller->GetPawn()) : nullptr;
	}
}

void UVisionReplicationGraphNode::NotifyAddNetworkActor(const FNewReplicatedActorInfo& ActorInfo)
{
	// 关卡名不影响过滤，全部放进同一份名单，避免子关卡绕过视野。
	ReplicationActorList.Add(ActorInfo.Actor);
}

bool UVisionReplicationGraphNode::NotifyRemoveNetworkActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound)
{
	const bool bRemoved = ReplicationActorList.RemoveFast(ActorInfo.Actor);
	if (!bRemoved && bWarnIfNotFound)
	{
		UE_LOG(LogReplicationGraph, Warning, TEXT("视野复制图移除 %s 时未找到"), *GetNameSafe(ActorInfo.Actor));
	}
	return bRemoved;
}

void UVisionReplicationGraphNode::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	TArray<const AMOBACharacter*, TInlineAllocator<4>> Heroes;
	for (const FNetViewer& Viewer : Params.Viewers)
	{
		if (const AMOBACharacter* Hero = ConnectionHero(Viewer))
		{
			Heroes.AddUnique(Hero);
		}
	}

	// 连接上还没有英雄时，不发送这批单位。英雄被控制后下一帧会按同队规则带上自己。
	if (Heroes.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	UVisionWorldSubsystem* Vision = World != nullptr ? World->GetSubsystem<UVisionWorldSubsystem>() : nullptr;

	Gathered.Reset(ReplicationActorList.Num());
	for (const FActorRepListType& Actor : ReplicationActorList)
	{
		if (!IsActorValidForReplicationGather(Actor))
		{
			continue;
		}

		// 子系统缺失时与查询占位一致：双方都在就发送。正常服务器上会走到 IsVisibleTo。
		bool bSend = Vision == nullptr;
		if (Vision != nullptr)
		{
			for (const AMOBACharacter* Hero : Heroes)
			{
				if (Vision->IsVisibleTo(Hero, Actor))
				{
					bSend = true;
					break;
				}
			}
		}
		if (bSend)
		{
			Gathered.ConditionalAdd(Actor);
		}
	}

	if (Gathered.Num() > 0)
	{
		Params.OutGatheredReplicationLists.AddReplicationActorList(Gathered);
	}
}

bool UVisionReplicationGraph::IsVisionFiltered(const AActor* Actor) const
{
	return Actor->IsA<AMOBACharacter>() || Actor->IsA<ALaneUnit>() || Actor->IsA<ABasicAttackProjectile>();
}

void UVisionReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (ActorCDO == nullptr || !ActorCDO->GetIsReplicated())
		{
			continue;
		}
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		FClassReplicationInfo ClassInfo;
		const float Frequency = ActorCDO->GetNetUpdateFrequency();
		if (Frequency > 0.f)
		{
			// 沿用 Actor 自己的同步频率，不在这里改小兵、野怪和塔的节奏。
			ClassInfo.ReplicationPeriodFrame = GetReplicationPeriodFrameForFrequency(Frequency);
		}

		if (IsVisionFiltered(ActorCDO))
		{
			// 距离裁剪交给视野。0 表示复制图不再按距离丢掉同队单位。
			ClassInfo.SetCullDistanceSquared(0.f);
			ClassInfo.DistancePriorityScale = 0.f;
			// 不再收集后尽快关掉通道。客户端会销毁该单位，不留最后位置。
			ClassInfo.ActorChannelFrameTimeout = 1;
		}
		else if (ActorCDO->bAlwaysRelevant || ActorCDO->bOnlyRelevantToOwner)
		{
			ClassInfo.SetCullDistanceSquared(0.f);
		}
		else
		{
			ClassInfo.SetCullDistanceSquared(ActorCDO->GetNetCullDistanceSquared());
		}

		GlobalActorReplicationInfoMap.SetClassInfo(Class, ClassInfo);
	}
}

void UVisionReplicationGraph::InitGlobalGraphNodes()
{
	GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>();
	GridNode->CellSize = 10000.f;
	GridNode->SpatialBias = FVector2D(-UE_OLD_WORLD_MAX, -UE_OLD_WORLD_MAX);
	AddGlobalGraphNode(GridNode);

	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AddGlobalGraphNode(AlwaysRelevantNode);

	VisionNode = CreateNewNode<UVisionReplicationGraphNode>();
	AddGlobalGraphNode(VisionNode);

	UE_LOG(LogReplicationGraph, Log, TEXT("复制图已启用：英雄、兵线单位和飞行物按队伍视野过滤。"));
}

void UVisionReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager)
{
	Super::InitConnectionGraphNodes(ConnectionManager);

	UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantForConnection = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForConnection>();
	AddConnectionGraphNode(AlwaysRelevantForConnection, ConnectionManager);
}

UReplicationGraphNode_AlwaysRelevant_ForConnection* UVisionReplicationGraph::GetAlwaysRelevantNodeForConnection(UNetConnection* Connection)
{
	UNetReplicationGraphConnection* Manager = Connection != nullptr ? FindConnectionManager(Connection) : nullptr;
	if (Manager == nullptr)
	{
		return nullptr;
	}
	for (UReplicationGraphNode* Node : Manager->GetConnectionGraphNodes())
	{
		if (UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevant = Cast<UReplicationGraphNode_AlwaysRelevant_ForConnection>(Node))
		{
			return AlwaysRelevant;
		}
	}
	return nullptr;
}

void UVisionReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo)
{
	AActor* Actor = ActorInfo.Actor;
	if (IsVisionFiltered(Actor))
	{
		VisionNode->NotifyAddNetworkActor(ActorInfo);
		return;
	}

	ensureMsgf((Actor->bAlwaysRelevant && Actor->bOnlyRelevantToOwner) == false, TEXT("单位 %s 同时勾了始终相关和只对主人相关。"), *GetNameSafe(Actor));
	if (Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
	}
	else if (Actor->bOnlyRelevantToOwner)
	{
		ActorsWithoutNetConnection.Add(Actor);
	}
	else
	{
		// 草丛、地板、泉水、营地标记走这里，按原有净裁剪距离。不休眠视野单位。
		GridNode->AddActor_Dormancy(ActorInfo, GlobalInfo);
	}
}

void UVisionReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	AActor* Actor = ActorInfo.Actor;
	if (IsVisionFiltered(Actor))
	{
		VisionNode->NotifyRemoveNetworkActor(ActorInfo);
		return;
	}

	if (Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
		SetActorDestructionInfoToIgnoreDistanceCulling(Actor);
	}
	else if (Actor->bOnlyRelevantToOwner)
	{
		if (UReplicationGraphNode* Node = Actor->GetNetConnection() != nullptr ? GetAlwaysRelevantNodeForConnection(Actor->GetNetConnection()) : nullptr)
		{
			Node->NotifyRemoveNetworkActor(ActorInfo);
		}
	}
	else
	{
		GridNode->RemoveActor_Dormancy(ActorInfo);
	}
}

void UVisionReplicationGraph::RouteRenameNetworkActorToNodes(const FRenamedReplicatedActorInfo& ActorInfo)
{
	AActor* Actor = ActorInfo.NewActorInfo.Actor;
	if (IsVisionFiltered(Actor))
	{
		VisionNode->NotifyActorRenamed(ActorInfo);
		return;
	}

	if (Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyActorRenamed(ActorInfo);
	}
	else if (Actor->bOnlyRelevantToOwner)
	{
		if (UReplicationGraphNode* Node = Actor->GetNetConnection() != nullptr ? GetAlwaysRelevantNodeForConnection(Actor->GetNetConnection()) : nullptr)
		{
			Node->NotifyActorRenamed(ActorInfo);
		}
	}
	else
	{
		GridNode->RenameActor_Dormancy(ActorInfo);
	}
}

int32 UVisionReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{
	for (int32 Index = ActorsWithoutNetConnection.Num() - 1; Index >= 0; --Index)
	{
		bool bRemove = false;
		if (AActor* Actor = ActorsWithoutNetConnection[Index])
		{
			if (UNetConnection* Connection = Actor->GetNetConnection())
			{
				bRemove = true;
				if (UReplicationGraphNode_AlwaysRelevant_ForConnection* Node = GetAlwaysRelevantNodeForConnection(Connection))
				{
					Node->NotifyAddNetworkActor(FNewReplicatedActorInfo(Actor));
				}
			}
		}
		else
		{
			bRemove = true;
		}

		if (bRemove)
		{
			ActorsWithoutNetConnection.RemoveAtSwap(Index, EAllowShrinking::No);
		}
	}

	return Super::ServerReplicateActors(DeltaSeconds);
}
