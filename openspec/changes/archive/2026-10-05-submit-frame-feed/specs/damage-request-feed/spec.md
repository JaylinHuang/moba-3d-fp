## ADDED Requirements

### Requirement: 覆盖之前把请求放进本拍列表

`UDamageRequestSubsystem::Submit` MUST 在覆盖最后一条请求之前，调用 `ULaneDamageFeedSubsystem::Push`，把这一条 `FHeroDamageRequest` 追加进本拍列表。同一拍里更早提交的请求 MUST 仍留在该列表中。列表中的先后 MUST 与 `Submit` 的调用顺序一致。

#### Scenario: 英雄之后又有小兵提交

- **WHEN** 同一拍里先提交一名英雄打中己方英雄的请求，随后一只小兵也提交了请求
- **THEN** 本拍列表里仍有那条英雄请求，并且它排在小兵请求之前

#### Scenario: 两名英雄同一拍都打中己方英雄

- **WHEN** 同一拍里两名敌方英雄先后提交了打中己方英雄的请求
- **THEN** 本拍列表里两条都在，先提交的排在前面

### Requirement: 最后一条请求仍然留给占位接收方

追加进本拍列表之后，`Submit` MUST 仍把这一条记为最后一条请求，并增加提交计数。读取最后一条请求的现有接口 MUST 返回这一条，而不是本拍列表里更早的条目。

#### Scenario: 连续提交两条

- **WHEN** 连续提交请求 A 和请求 B
- **THEN** 最后一条请求是 B，提交计数增加两次，本拍列表里 A 和 B 都在

### Requirement: 广播不结算生命

这次追加 MUST NOT 修改生命、金钱或经验，也 MUST NOT 替换 `UDamageRequestSubsystem`。

#### Scenario: 提交后生命不变

- **WHEN** 一次伤害请求被提交并追加进本拍列表
- **THEN** 目标的生命没有因为这次追加而改变
