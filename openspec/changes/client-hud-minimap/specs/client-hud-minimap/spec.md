## ADDED Requirements

### Requirement: 小地图只画客户端上已有的英雄和兵线单位

客户端小地图 MUST 只绘制当前世界里已经存在的 `AMOBACharacter` 和 `ALaneUnit`。绘制 MUST 使用该 Actor 已有的位置、队伍、种类和死活。小地图 MUST NOT 调用视野查询来决定画谁。世界里不存在的单位 MUST NOT 出现在小地图上，也 MUST NOT 保留最后位置。

飞行物、泉水、草丛、营地标记和测试假人 MUST NOT 画在小地图上。

不同队伍 MUST 能从标记上区分。英雄和兵线单位 MUST 能从标记上区分。`IsAlive` 为假的单位 MUST 仍画在它当前的位置，并且 MUST 能与存活单位区分。本地玩家控制的英雄 MUST 能与其他英雄区分。

世界坐标到小地图的投影 MUST 使用表现模块里标明占位的固定矩形。这个矩形 MUST NOT 写进玩法规则。

#### Scenario: 已复制的敌方英雄出现在小地图上

- **WHEN** 客户端世界里存在一名敌方 `AMOBACharacter`
- **THEN** 小地图在对应该英雄当前位置的地方画出该英雄，并且标记能看出这是敌方英雄

#### Scenario: 看不见的敌人不留在小地图上

- **WHEN** 一名敌方英雄没有被复制到该客户端，世界里没有这个 Actor
- **THEN** 小地图不画出该英雄，也不保留该英雄先前的位置

#### Scenario: 同队草丛中的英雄仍然在小地图上

- **WHEN** 客户端世界里存在同队、站在草丛内的 `AMOBACharacter`
- **THEN** 小地图画出该英雄

#### Scenario: 死亡单位用另一种标记

- **WHEN** 客户端世界里的一名 `ALaneUnit` 或 `AMOBACharacter` 的 `IsAlive` 为假
- **THEN** 小地图仍在该单位当前位置画出它，并且标记与存活时不同

#### Scenario: 自己的英雄和其他英雄不同

- **WHEN** 本地玩家控制着一名 `AMOBACharacter`，客户端上还有其他英雄
- **THEN** 小地图上本地控制的那名英雄能与其他英雄区分开

#### Scenario: 飞行物不进小地图

- **WHEN** 客户端世界里存在普通攻击飞行物
- **THEN** 小地图不画出该飞行物

### Requirement: HUD 只显示自己英雄的生命和金币

HUD MUST 只读取本地玩家当前控制的 `AMOBACharacter`。该英雄身上有 `UCombatStateComponent` 时，HUD MUST 显示该组件的当前生命、生命上限和金币。该英雄身上没有这个组件时，HUD MUST NOT 显示生命、生命上限或金币的数字。

HUD MUST NOT 显示其他英雄的生命或金币。HUD MUST NOT 改写生命、金币、存活或购买结果。

#### Scenario: 组件已在自己的英雄身上

- **WHEN** 本地玩家控制的英雄身上有 `UCombatStateComponent`
- **THEN** HUD 显示该组件的当前生命、生命上限和金币

#### Scenario: 组件不在客户端时留空

- **WHEN** 本地玩家控制的英雄身上没有 `UCombatStateComponent`
- **THEN** HUD 不显示生命、生命上限或金币的数字

#### Scenario: 不显示别人的金币

- **WHEN** 客户端上另一名英雄身上有 `UCombatStateComponent`，本地玩家控制的是不同的英雄
- **THEN** HUD 不显示那名其他英雄的生命或金币

#### Scenario: 绘制不改金币

- **WHEN** HUD 读取并显示本地英雄的金币
- **THEN** 该英雄的金币、生命和存活状态保持读取前的值
