## Context

客户端已经能收到队伍看得见的英雄、兵线单位和飞行物。`AMOBACharacter` 复制队伍、死活、普攻阶段和位置。`ALaneUnit` 复制队伍、种类、死活和位置。看不见的敌方单位不留在客户端。

`UCombatStateComponent` 只在权威端由 `UCombatResolutionSubsystem::EnsureState` 创建。该子系统在客户端直接返回。生命、上限和金币没有复制，客户端上找不到这个组件。

`AMOBAGameMode` 没有指定 HUD 类。`MOBA.Build.cs` 只有 Core、CoreUObject、Engine、InputCore、EnhancedInput、HeroTemplates 和 ReplicationGraph。游戏模式、`Build.cs`、`.uproject` 和 `Config/` 这一刀不能改，要等统筹窗口登记。

`LaneTest` 的地板中心在 `(0, 3000)`，缩放是 `(100, 50, 1)`。按引擎默认平面，水平范围大约是 X 从 -5000 到 5000，Y 从 500 到 5500。兵线布局没有公开的地图范围查询。

## Goals / Non-Goals

**Goals:**

- 在有本地视口的客户端上画出小地图和自己英雄的生命、金币。
- 小地图只根据客户端上已经存在的 `AMOBACharacter` 和 `ALaneUnit` 绘制。
- HUD 只读本地控制的那名英雄。组件在就显示当前生命、生命上限和金币；不在就留空。
- 不新增模块依赖，不改共享工程文件，不改战斗组件。

**Non-Goals:**

- 复制生命、上限和金币。缺口留在 `docs/agents/05-presentation.md`，由同步窗口或统筹窗口处理。
- 别人的血条、别人的金币、商店、结算、受击和开火特效。
- 飞行物、泉水、草丛、营地标记和测试假人的小地图图标。
- 再次做视野查询，或在客户端保留已经消失的敌人位置。
- 修改 `AMOBAGameMode`、`MOBA.Build.cs`、`.uproject` 和 `Config/`。

## Decisions

### 用引擎画布，不引入界面模块

`UDebugDrawService::Register(TEXT("Game"), ...)` 在正常对局画面上回调 `UCanvas` 和本地 `APlayerController`。世界分区子系统已经用这个名字注册。表现侧用同一个入口，因此不需要 HUD 类，也不需要 UMG 或 Slate 模块。

绘制放在 `Source/MOBA/Presentation/` 的世界子系统里。专用服务器不创建它。有本地视口的客户端和独立进程才创建。

不采用改 `HUDClass` 或给 `Build.cs` 加 UMG。那两条都要先改共享文件。

### 小地图只遍历已经在客户端的两类 Actor

每帧找 `AMOBACharacter` 和 `ALaneUnit`。Actor 不在这个世界里就不画，也不记最后位置。不调用 `UHeroVisibilityQuery` 或 `UVisionWorldSubsystem`。

队伍用已复制的 `GetTeamId()` 区分。种类用 `ELaneUnitKind` 区分英雄和兵线单位。`IsAlive()` 为假时用另一种标记，仍然画在当前位置。本地 `APlayerController` 控制的那名英雄再用单独标记，这样自己和同队队友能分开。

飞行物即使被复制也不进入小地图。

### 地图框是表现侧占位

世界坐标投到小地图上时，使用一块标明占位的固定矩形：X 从 -5000 到 5000，Y 从 500 到 5500。这是按当前 `LaneTest` 地板量的，放在表现目录里，不写进玩法规则，也不去读兵线布局的私有坐标。矩形内的单位画在地图内。矩形外的单位夹在地图边缘，避免图标跑出地图。

`TestGym` 没有这块地板。同一套框仍可用，英雄会落在框内或边缘。本变更不为验收图另做一张地图。

### HUD 只读本地英雄上的战斗组件

本地控制的 Pawn 能转成 `AMOBACharacter` 时，用 `FindComponentByClass<UCombatStateComponent>()` 读取。找到了就显示 `GetHealth()`、`GetMaxHealth()` 和 `GetGold()`。找不到就不画这三个数，留出空位。金币为 0 且组件存在时显示 0，因为那是组件上的真实值，不是用 0 填缺口。

不读取其它英雄身上的组件，即使以后组件被复制到了别的 Pawn 上。不调用加金币、扣血、购买或改存活的方法。

## Risks / Trade-offs

- [生命和金币还没复制到客户端] → HUD 按规格留空。小地图不依赖这三个数，仍然可画。复制由其它窗口补，本变更不改 `Source/MOBA/Combat/`。
- [地图框写死在表现侧] → 兵线以后挪动地板时，小地图比例会偏。框标明占位，改框只动表现目录。
- [画布回调和正式 HUD 类不是同一条路] → 这一刀能在不改游戏模式的情况下出现在画面上。以后若要 UMG，再向统筹登记 `Build.cs`。
- [矩形外的单位被夹在边缘] → 多个单位可能叠在同一条边上。当前 `LaneTest` 的塔和兵线都在框内，边缘夹紧只留给跑出地板的情况。

## Migration Plan

没有要迁移的存档或协议。不要这层绘制时，去掉表现目录里的子系统即可，权威模拟不受影响。

## Open Questions

没有会挡住这一刀的未决问题。生命、上限和金币由谁复制，已经记给同步窗口或统筹窗口，不在本变更里实现。
