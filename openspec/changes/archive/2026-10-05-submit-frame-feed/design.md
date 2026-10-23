## Context

`Submit` 只把 `LastRequest` 换成最新一条。`ULaneDamageFeedSubsystem::Push` 会按调用顺序编号并追加，但没有人调用它。`ALaneDirector` 在 `TG_PrePhysics` 清空列表，塔和小兵在 `TG_PostUpdateWork` 读取。统筹窗口已允许只改 `Source/MOBA/DamageRequest.cpp`。

## Goals / Non-Goals

**Goals:**

- 每次 `Submit` 先把这一条放进本拍列表，再覆盖 `LastRequest`。
- 最后一条请求、提交计数和现有日志保持原样。
- 进度文件写明广播已经接上。

**Non-Goals:**

- 不改塔的锁定规则、小兵呼救规则，也不改它们的读取时机。
- 不在 `Submit` 里按伤害类型过滤。过滤仍由塔和小兵在读列表时做。
- 不扣生命，不替换子系统，不改英雄出手代码、统筹文件和契约。

## Decisions

### 在 Submit 内部调用 Push

`UDamageRequestSubsystem` 自己是世界子系统，用 `GetWorld()` 取得 `ULaneDamageFeedSubsystem`，在 `LastRequest = Request` 之前调用 `Push`。包含关系只加在 `DamageRequest.cpp`，避免头文件互相包含。

备选是让每个出手点自己 `Push`。那样英雄、小兵和塔都会漏调用，而且后写的出手又会盖住问题。调用收口在 `Submit`。

世界或兵线子系统不存在时，跳过 `Push`，仍写入最后一条请求。当前游戏世界里该子系统会随模块加载。

### 不在 Submit 里再判权威

现有出手已经在服务器上调用 `Submit`。列表和最后一条请求都在这次调用所在的世界里。不在 `Submit` 里增加第二套权威判断。

### 清空时机不动

列表仍由兵线导演在物理更新前清空。`Submit` 只追加。塔仍在本拍更新结束后读列表，因此同一拍里先提交的请求不会被清空掉。

## Risks / Trade-offs

- [客户端若也调用 Submit] → 只会写入那个世界自己的列表。当前普攻和小兵出手都在服务器上提交。
- [本拍列表变长] → 第一版每拍的请求量就是场上单位的出手次数。不在这里做裁剪。

## Migration Plan

没有旧列表需要迁移。回退时去掉 `Submit` 里的 `Push` 调用即可，兵线会回到只打最近敌方小兵的状态。

## Open Questions

无。
