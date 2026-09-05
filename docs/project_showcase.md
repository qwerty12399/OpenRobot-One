# OpenRobot-One 项目成果与验证证据

本项目把仿真、嵌入式、通信、真实执行机构和安全验收组织成一条可追溯链路。

## 专业能力链

```text
需求与安全门
-> ROS 2/Gazebo/SLAM系统集成
-> 参数化差速运动学
-> UART协议与异常恢复
-> STM32实时控制
-> 双BTS7960/双电机/双编码器
-> 双PID与超时停车
-> ROS 2架空闭环与自动化验收
```

## 已验证成果

| 范围 | 状态 | 结果 |
| --- | --- | --- |
| ROS 2构建与测试 | 实际通过（本次复验） | 11 packages，106 tests，0 errors/failures，3 skipped |
| Gazebo与SLAM | 实际通过 | `/scan`、`/odom`、`/joint_states`、`/map`和目标TF |
| STM32构建/下载 | 实际通过 | STM32F407ZGT6下载、逐字节verify和复位 |
| UART压力 | 实际通过 | 10000/10000 PING/PONG |
| UART异常恢复 | 实际通过 | 错误帧、超长、半包、错误波特率和重连 |
| 双电机FF+PI | 实际通过 | 架空前进、后退、左右差速旋转 |
| ROS 2真机链路 | 实际通过 | `/cmd_vel`、`/joint_states`、架空估算 |
| 自动硬件回归 | 实际通过 | 7/7 PASS，含命令超时停车 |

历史结果来自 [项目验证报告](verification/2026-09-01-project-validation.md) 和
[H2报告](verification/2026-08-31-h2-board-bringup.md)；2026-09-05 收尾复验在
Docker中完成11个包、106项测试，0错误/失败、3项跳过。

## 当前真机目标

```text
/cmd_vel
-> 左右目标RPM
-> ROS 2串口驱动
-> STM32双PID
-> 两块IBT-2/BTS7960
-> 两台JGA25-370
-> 双AB编码器
-> wheel feedback
-> /bench/odom_estimate
```

这是架空双电机闭环，不是落地底盘。hardware模式只发布
`/bench/odom_estimate` 数据，不占用标准 `/odom`，也不广播底盘TF。

## 当前证据边界

- 双电机方向、±100 RPM闭环、ROS `/cmd_vel`、300 ms上层超时与500 ms
  STM32通信看门狗已有实测证据。
- `/joint_states` 和 `/bench/odom_estimate` 已在架空台架联调。
- 最新连续控制和看门狗结果由用户确认，尚缺对应原始终端日志与已烧录
  ELF哈希，不能据此报告精确时延统计。
- 当前ASCII遥测不含累计编码器计数、序号、CRC或故障码；重连、诊断与
  1000样本延迟/丢失统计仍未完成。
- 轮径和轮距仍是名义参数，架空积分不代表真实地面距离或转角精度。

因此可以宣称架空双电机控制闭环和架空估算已经跑通，但不能宣称落地里程计、
真机SLAM/Nav2、精确时延指标或完整容错协议已经完成。

## 可交付能力

### 已验证能力

- 在ROS 2 Humble/Gazebo中完成差速底盘、LaserScan、TF和SLAM链路验证。
- 完成STM32F407板级下载、UART压力、异常输入和复位重连测试。
- 完成双BTS7960、双编码器电机、100 Hz FF+PI和20 kHz PWM架空验证。
- 打通 `/cmd_vel` 到STM32双执行通道及 `/joint_states` 反馈闭环。
- 实现300 ms ROS命令超时与500 ms MCU通信看门狗双层停车保护。
- 开发一键硬件回归脚本，完成7/7真机测试。
- 维护Docker、CI、验收脚本、验证报告和UNKNOWN/BLOCKED清单。

不能写“完成落地真机底盘”“完成真实里程计精度”或“完成真机自主导航”。

## 证据入口

- [系统架构](architecture.md)
- [开发路线](roadmap.md)
- [阶段任务](stage1_tasks.md)
- [验收标准](acceptance.md)
- [硬件BOM](hardware_bom.md)
- [硬件事实](hardware_facts.md)
- [协议](protocol.md)
- [BTS7960双电机设计](superpowers/specs/2026-09-01-bts7960-dual-motor-bench-design.md)
- [UART验收脚本](../scripts/test_h2_uart.ps1)
- [最新真机验收](evidence/2026-09-05-hardware-acceptance.md)
