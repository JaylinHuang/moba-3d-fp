## Why

英雄和兵线已经会提交伤害请求，服务器却只把请求记下来，不改生命、金币和经验。单位死不掉，补刀和击杀也没有归属。战斗窗口这一刀要把占位接收方换成真正结算。商店购买和复活计时留到下一刀。

## What Changes

- `Submit` 在 `Push` 之后立刻结算这一条请求。本拍列表和最后一条请求都保留。不去掉 `Push`，不替换 `UDamageRequestSubsystem`。
- 友军请求不改生命。敌方请求按占位攻击和护甲扣生命。生命到零时把 `IsAlive` 写成假。同一帧里更晚 tick 的塔可以看见这次死亡。
- 基地生命到零时调用现有的 `NotifyDestroyed`。胜负状态机仍归兵线。
- 小兵死亡：最后一击是英雄才给该英雄补刀金币；击杀方附近的英雄共享经验，不看出手的是谁。
- 英雄死亡：最后一击是英雄时，该英雄获得击杀金币，击杀方附近的英雄共享经验。没有补上最后一击的人只有经验，没有击杀金币。最后一击是塔或小兵时，附近同队英雄只有经验，击杀金币没有人获得。塔和小兵自己不拿金币。
- 塔和野怪死亡只写 `IsAlive`，不加金币和经验。
- 活着的英雄按时间获得自然金币。经验累积并升级。技能点留给以后的技能槽。
- 战士、法师、射手的攻击距离和出手间隔改由战斗模块提供。英雄只读取，不写回。飞行物的伤害来源仍是英雄本人。
- 生命、护甲、金币、经验、攻速、攻击距离和共享半径使用标明占位的数字。

## Capabilities

### New Capabilities

- `combat-damage`: `Push` 之后结算生命，友军不掉血，生命到零写 `IsAlive`，基地走 `NotifyDestroyed`
- `combat-rewards`: 补刀金币、击杀金币、自然金币、附近友军共享经验，以及升级
- `combat-attack-stats`: 按英雄提供攻击距离和出手间隔，英雄只读

### Modified Capabilities

无。`damage-request-feed` 仍规定追加本身不改生命、金钱和经验，结算是追加之后的另一步。`hero-templates` 已经要求英雄在战斗模块提供数值后只读取。`basic-attack` 仍只规定何时提交。兵线的锁定、刷兵和胜负入口不变。

## Impact

- 新代码放在 `Source/MOBA/Combat/`。结算从 `Source/MOBA/DamageRequest.cpp` 的 `Push` 之后调用。`Push` 保持在覆盖最后一条请求之前。
- 小兵、塔和野怪用现有的 `SetAlive`。基地用现有的 `NotifyDestroyed`。英雄对塔的伤害读取 `GetIncomingHeroDamageScale`。
- 英雄的 `bAlive` 还没有对外写入口。改英雄文件前，先把这一处写进 `docs/agents/03-combat.md`，等统筹窗口登记。
- 不改普攻选目标、兵线路径、刷兵、营地、草丛、胜负状态机、`docs/contracts/`、`Build.cs`、`TestGym` 和默认地图。
- 商店购买、复活时间、回城和技能点不在本变更里。
