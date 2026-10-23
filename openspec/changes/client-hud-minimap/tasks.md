## 1. 画布入口

- [x] 1.1 在 `Source/MOBA/Presentation/` 增加世界子系统，向 `UDebugDrawService` 注册 `Game` 画布回调。专用服务器不创建该子系统
- [x] 1.2 不修改 `AMOBAGameMode`、`MOBA.Build.cs`、`.uproject`、`Config/` 和 `Source/MOBA/Combat/`

## 2. 小地图

- [x] 2.1 在表现目录放入标明占位的地图矩形：X 从 -5000 到 5000，Y 从 500 到 5500。不写入玩法规则
- [x] 2.2 每帧只绘制世界里已有的 `AMOBACharacter` 和 `ALaneUnit`。用位置、队伍、种类和死活区分标记。本地控制的英雄单独标记。矩形外的单位夹在地图边缘
- [x] 2.3 不调用视野查询，不缓存已经不在世界里的单位，不绘制飞行物、泉水、草丛、营地标记和测试假人

## 3. 自己的生命和金币

- [x] 3.1 HUD 只读本地玩家控制的 `AMOBACharacter` 上的 `UCombatStateComponent`。有组件时显示当前生命、生命上限和金币
- [x] 3.2 没有组件时不显示这三个数字。不读取其他英雄的组件，不改写生命、金币或存活

## 4. 进度

- [x] 4.1 编辑器目标编译通过后，在 `docs/agents/05-presentation.md` 追加进度日志，写明小地图和 HUD 已画、生命金币在组件缺失时留空、复制缺口仍留给同步或统筹
