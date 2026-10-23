## ADDED Requirements

### Requirement: 打野营地按间隔刷新并让野怪回到营地

`LaneTest` MUST 至少包含一个打野营地。营地内的野怪 MUST 实现 `IAttackable`。战斗模块写入生命之前，野怪的 `IsAlive` MUST 保持为真。野怪离开营地范围时 MUST 回到营地。营地变空后，MUST 按配置的间隔重新生成野怪。野怪 MUST NOT 使用兵线上小兵的呼救、路点推进或建筑锁。

#### Scenario: 离开营地后返回

- **WHEN** 一只野怪移动到自己营地的范围之外
- **THEN** 它回到该营地

#### Scenario: 营地变空后重新生成

- **WHEN** 一个营地里已经没有野怪，且配置的刷新间隔已经过去
- **THEN** 该营地重新生成野怪

### Requirement: 草丛只记录进出

`LaneTest` MUST 至少包含一块草丛体积。单位进入和离开该体积时，系统 MUST 记录谁在体积内。可见与否 MUST 只通过现有的 `UHeroVisibilityQuery` 判断。本变更 MUST NOT 修改这个查询。

#### Scenario: 进入体积被记录

- **WHEN** 一个单位走进草丛体积
- **THEN** 记录显示该单位在这块草丛内

#### Scenario: 离开体积后不再记录为在内

- **WHEN** 一个已经在草丛内的单位离开该体积
- **THEN** 记录不再把该单位算作在这块草丛内

#### Scenario: 查询类保持原样

- **WHEN** 本变更完成
- **THEN** `UHeroVisibilityQuery` 的实现没有被本变更修改
