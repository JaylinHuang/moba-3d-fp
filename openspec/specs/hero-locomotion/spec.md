# hero-locomotion Specification

## Purpose

TBD。增量规格未提供目的说明。

## Requirements

### Requirement: 第一人称鼠标朝向

英雄 MUST 使用第一人称镜头。没有普攻锁定时，鼠标 MUST 控制视角朝向，角色水平朝向跟随视角左右转向。

#### Scenario: 鼠标改变朝向

- **WHEN** 英雄没有普攻锁定，且玩家移动鼠标
- **THEN** 视角朝向跟随鼠标改变

#### Scenario: 松开移动键后停止

- **WHEN** 英雄没有普攻命令，且玩家松开全部 WASD 键
- **THEN** 英雄停止地面移动

### Requirement: WASD 地面移动

英雄 MUST 用 WASD 沿当前水平朝向在地面移动。客户端 MUST 只提交输入，专用服务器 MUST 作为位置的权威。

#### Scenario: 按键相对朝向移动

- **WHEN** 玩家按下前进键
- **THEN** 专用服务器使英雄沿当前水平朝向前进

#### Scenario: 客户端位置不是权威

- **WHEN** 客户端本地位置与专用服务器位置不一致
- **THEN** 专用服务器位置成为该英雄的权威位置

### Requirement: 新的移动指令

新的移动指令 MUST 是 WASD 某一键从松开变为按下。已经按住的键 MUST NOT 算作新的移动指令。

#### Scenario: 按住的键不是新指令

- **WHEN** 前进键已经处于按下状态
- **THEN** 该按住状态不是新的移动指令

#### Scenario: 松开后再按是新指令

- **WHEN** 玩家松开前进键后再次按下前进键
- **THEN** 这次按下是新的移动指令
