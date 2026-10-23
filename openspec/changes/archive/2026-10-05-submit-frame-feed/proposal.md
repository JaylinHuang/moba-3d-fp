## Why

塔要在同一拍里比较所有「敌方英雄打中己方英雄」的请求，再锁离塔最近的那一个。`Submit` 现在只保留最后一条，后到的小兵或另一名英雄会盖掉前面的记录。统筹窗口已经登记：在覆盖最后一条之前调用 `Push`。

## What Changes

- `UDamageRequestSubsystem::Submit` 在写入 `LastRequest` 之前，调用 `ULaneDamageFeedSubsystem::Push`，把这一条追加进本拍列表。
- 最后一条请求仍然留给现在的占位接收方。不替换该子系统，不扣生命、不加钱、不加经验。
- 塔和小兵继续在本拍更新结束后读取这份列表。同一拍收齐、距离相同按广播先后，这些选择规则不变。
- 在 `docs/agents/02-lane.md` 记下广播已经接上。

## Capabilities

### New Capabilities

- `damage-request-feed`: `Submit` 把每一条伤害请求追加进本拍列表，再覆盖最后一条。列表按提交先后编号。不结算生命。

### Modified Capabilities

无。`lane-structures` 和 `lane-minions` 里的锁定、呼救规则不变，只是现在能读到本拍的完整请求。`basic-attack` 仍只规定何时提交，不规定列表怎么留。

## Impact

- 只改 `Source/MOBA/DamageRequest.cpp` 这一处调用。`Push`、本拍清空和塔的读取已经在兵线模块里。
- 不改 `docs/agents/00-orchestrator.md`、`docs/contracts/`、英雄攻击、`Build.cs`、`TestGym` 和默认地图。
- 玩法细则以统筹文件里刚登记的那一条为准。
