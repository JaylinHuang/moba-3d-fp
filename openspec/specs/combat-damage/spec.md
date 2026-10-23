# combat-damage Specification

## Purpose

TBD。增量规格未提供目的说明。

## Requirements

### Requirement: Push 之后立刻结算

权威端的 `UDamageRequestSubsystem::Submit` MUST 在 `ULaneDamageFeedSubsystem::Push` 返回并且最后一条请求已经更新之后，结算这一条请求。本拍列表 MUST 仍包含这一条。`Push` 本身 MUST NOT 修改生命、金钱或经验。`Submit` MUST NOT 去掉这次 `Push`，也 MUST NOT 替换 `UDamageRequestSubsystem`。

#### Scenario: 结算后列表和最后一条都在

- **WHEN** 权威端提交一条敌方伤害请求
- **THEN** 本拍列表包含这一条，最后一条请求也是这一条，并且目标生命已经按这次结算变化

#### Scenario: 追加本身不改生命

- **WHEN** 只执行把请求放进本拍列表的那一步
- **THEN** 目标的生命、金钱和经验都没有因为这一步而改变

### Requirement: 友军请求不造成伤害

来源与目标的队伍编号相同时，结算 MUST NOT 修改目标生命，也 MUST NOT 把目标的 `IsAlive` 写成假。

#### Scenario: 同队命中

- **WHEN** 权威端提交一条来源与目标队伍编号相同的请求
- **THEN** 目标生命不变，并且 `IsAlive` 仍为真

### Requirement: 敌方命中扣减占位生命

来源与目标队伍编号不同、且目标仍然活着时，结算 MUST 用标明占位的攻击和护甲计算伤害，并扣减目标生命。这样的一次命中 MUST 使生命下降。来源是英雄、目标是外塔或内塔时，伤害 MUST 再乘该塔当前的 `GetIncomingHeroDamageScale()`。基地 MUST NOT 使用这个系数。

#### Scenario: 一次命中后仍然活着

- **WHEN** 活着的敌方目标所受的占位生命高于这一次结算伤害
- **THEN** 目标生命下降，并且 `IsAlive` 仍为真

#### Scenario: 英雄打塔时使用降低系数

- **WHEN** 英雄命中一座附近没有己方小兵的外塔或内塔
- **THEN** 这次伤害使用该塔在没有己方小兵时的系数

### Requirement: 生命到零时写死亡

目标生命达到零时，结算 MUST 把该目标的 `IsAlive` 写成假。目标是基地时，MUST 调用该基地的 `NotifyDestroyed`，并且 MUST NOT 另写一套胜负结果。已经死亡的目标再次被提交时，MUST NOT 再次扣生命，也 MUST NOT 再次触发死亡。

#### Scenario: 普通单位被打死

- **WHEN** 一次敌方命中使小兵、野怪、外塔、内塔、英雄或测试假人的生命达到零
- **THEN** 该目标的 `IsAlive` 为假

#### Scenario: 基地被打死

- **WHEN** 一次敌方命中使基地的生命达到零
- **THEN** 该基地的 `NotifyDestroyed` 被调用，并且其 `IsAlive` 为假

#### Scenario: 尸体不再结算

- **WHEN** 目标的 `IsAlive` 已经为假，又有一条以它为目标的请求被提交
- **THEN** 目标生命不再变化
