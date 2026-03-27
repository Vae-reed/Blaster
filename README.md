# Blaster

基于 UE5 C++ 从零开发的多人第三人称射击游戏，重点实现了网络同步、动画系统和战斗系统的核心架构。

## 演示视频

[![点击观看演示视频](assets/Blaster.png)](https://www.bilibili.com/video/BV1t6X4BsEhi)

## 已实现功能

- **角色移动**：完整的移动、跳跃、蹲下系统，支持多方向动画混合（横移、倾斜）
- **战斗系统**：基于 Projectile 的武器系统，含碰撞检测、网络生成、粒子曳光特效及击中音效
- **动画系统**：AimOffset 上半身独立瞄准、Layered Blend Per Bone 上下身分离、FABRIK IK 左手贴合武器
- **准星系统**：动态准星扩散（随移动速度、跳跃、瞄准状态实时变化）、命中目标自动变色
- **受击反馈**：多端同步的受击动画广播
- **弹壳特效**：开枪时从抛壳口生成带物理模拟的弹壳，落地触发音效后自动销毁
- **多人联机**：基于 Steam Online Subsystem，支持双人联机对战

## 开发中

- [ ] 生命值与玩家状态系统（Health & Player State）
- [ ] 伤害系统与死亡复活
- [ ] 计分与游戏模式
- [ ] UI 系统（血量条、弹药显示、击杀信息）
- [ ] Server-Side Rewind 延迟补偿

## 技术亮点

**网络同步**
- 基于 Server RPC / Multicast RPC 设计核心逻辑的权威端验证，保证多端状态一致
- 修复网络层 uint16 压缩导致的 Pitch 角度失真问题，通过区间重映射还原正确值
- 结合 `OnRep_ReplicatedMovement` 与计时器补偿解决 AO_Yaw 多端同步抖动，SimulatedProxy 转身流畅无跳帧

**动画系统**
- Mesh Space AimOffset 实现上半身独立瞄准，解决 Local Space 下父骨骼旋转带偏瞄准方向的问题
- Layered Blend Per Bone 实现上下身动画完全分离，角色可同时奔跑与独立瞄准
- FABRIK IK + 右手旋转矫正，左手动态贴合武器握持点，消除持枪时的手部穿模

## 游玩方式

> 联机需要两人均已启动 Steam，且在 **Steam 设置 → 下载 → 下载地区** 中保持地区一致。

1. 一人点击 **Host** 作为主机创建房间
2. 另一人点击 **Join** 加入游戏

## 环境

- Unreal Engine 5.4
- C++
- Steam Online Subsystem
