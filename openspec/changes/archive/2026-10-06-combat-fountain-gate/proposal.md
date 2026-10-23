## Why

己方泉水的队伍编号已经能读到，但购买仍然不看站位，复活仍然留在死亡地点。商店和复活应该使用这座泉水。

## What Changes

- 权威端购买增加一个条件：活着的英雄与一座己方泉水的水平距离不超过占位回血半径。半径外、读不到己方泉水、金币不足、已拥有或已死亡时，金币和属性都不变。
- 复活计时结束且能确定己方泉水时，英雄在写成存活的同时落到该泉水的位置。等级、经验、金币和已购商品仍保留。读不到己方泉水时，复活仍留在原地。
- 商品种类、价格、属性效果、复活秒数公式、回血和回城读条不改。回城按键、鞋子改走路速度、技能点不在这一刀。

## Capabilities

### New Capabilities

无。

### Modified Capabilities

- `combat-shop`: 购买必须发生在己方泉水的占位半径内
- `combat-respawn`: 能读到己方泉水时，复活落到泉水位置

## Impact

- `UCombatResolutionSubsystem` 的 `TryBuy` 和复活完成处。位置只在复活完成时写到己方泉水，不改移动组件速度和普攻状态。
- `combat-shop` 与 `combat-respawn` 的主规格尚未从变更 `combat-shop-respawn` 同步。本变更的修改块要等那份主规格存在之后才能归档合并。
- 不改 `LaneProps.h`、`docs/contracts`、工程文件和 `TestGym`。
