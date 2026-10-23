## MODIFIED Requirements

### Requirement: 草丛只记录进出

`LaneTest` MUST 至少包含一块草丛体积。单位进入和离开该体积时，系统 MUST 记录谁在体积内。可见与否 MUST 只通过 `UHeroVisibilityQuery` 判断。草丛 Actor MUST NOT 自行返回某个单位是否可见。查询的队伍视野实现由同步模块提供，不由草丛体积提供。

#### Scenario: 进入体积被记录

- **WHEN** 一个单位走进草丛体积
- **THEN** 记录显示该单位在这块草丛内

#### Scenario: 离开体积后不再记录为在内

- **WHEN** 一个已经在草丛内的单位离开该体积
- **THEN** 记录不再把该单位算作在这块草丛内

#### Scenario: 草丛不自行判断可见

- **WHEN** 一个单位站在草丛体积内
- **THEN** 看得见与否仍只由 `UHeroVisibilityQuery::IsVisibleTo` 回答
