#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "VisionReplicationGraph.generated.h"

/** 按连接上的英雄收集看得见的单位。只放英雄、兵线单位和普通攻击飞行物。 */
UCLASS()
class MOBA_API UVisionReplicationGraphNode : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:
	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo& ActorInfo) override;
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true) override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;

private:
	/** 本连接这一帧要发送的单位。收集完就拷走，下一条连接会清空重填。 */
	FActorRepListRefView Gathered;
};

/** 服务器复制图。视野单位按队伍视线过滤，其余单位保持距离或始终相关。 */
UCLASS(transient, config = Engine)
class MOBA_API UVisionReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()

public:
	virtual void InitGlobalActorClassSettings() override;
	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager) override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;
	virtual int32 ServerReplicateActors(float DeltaSeconds) override;

protected:
	virtual void RouteRenameNetworkActorToNodes(const FRenamedReplicatedActorInfo& ActorInfo) override;

private:
	bool IsVisionFiltered(const AActor* Actor) const;
	UReplicationGraphNode_AlwaysRelevant_ForConnection* GetAlwaysRelevantNodeForConnection(UNetConnection* Connection);

	UPROPERTY()
	TObjectPtr<UReplicationGraphNode_GridSpatialization2D> GridNode;

	UPROPERTY()
	TObjectPtr<UReplicationGraphNode_ActorList> AlwaysRelevantNode;

	UPROPERTY()
	TObjectPtr<UVisionReplicationGraphNode> VisionNode;

	/** 生成时还没有所属连接、只该发给主人的 Actor。 */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActorsWithoutNetConnection;
};
