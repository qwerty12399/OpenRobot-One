# OpenRobot-One 项目成果与验证证据

本项目面向技术支持、FAE、项目交付、实施和机器人系统/硬件测试岗位。核心价值是把仿真、嵌入式、通信、真实执行机构和安全验收组织成一条可追溯链路。

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
| ROS 2构建与测试 | 实际通过（本次复验） | 11 packages，88 tests，0 errors/failures/skipped |
| Gazebo与SLAM | 实际通过 | `/scan`、`/odom`、`/joint_states`、`/map`和目标TF |
| STM32构建/下载 | 实际通过 | STM32F407ZGT6下载、逐字节verify和复位 |
| UART压力 | 实际通过 | 10000/10000 PING/PONG |
| UART异常恢复 | 实际通过 | 错误帧、超长、半包、错误波特率和重连 |

历史结果来自 [项目验证报告](verification/2026-09-01-project-validation.md) 和 [H2报告](verification/2026-08-31-h2-board-bringup.md)；本次88项结果见 [双电机转向验证记录](verification/2026-09-01-bts7960-dual-motor-transition.md)。

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

这是架空双电机闭环，不是落地底盘。`/bench/odom_estimate`不广播TF。

## 当前阻塞

- H0/H1仍未通过：缺少完整保护、逐块逻辑阈值、编码器电平和电流证据。
- 当前固件只有H2 UART，没有四路PWM、四EN、双编码器、PID或电机看门狗。
- ROS hardware Launch仍是安全占位，不发送运动命令。
- 商品页“43A”“3.3–5V”和方向定义均未由实物验证。

因此不得宣称双电机、编码器、PID、架空估算里程计或真机超时停车已经完成。

## 可交付能力

### 当前可写入简历

- 在ROS 2 Humble/Gazebo中完成差速底盘、LaserScan、TF和SLAM链路验证。
- 完成STM32F407板级下载、UART压力、异常输入和复位重连测试。
- 设计H0–H5双电机真机安全门、无冲突引脚和统一协议边界。
- 维护Docker、CI、验收脚本、验证报告和UNKNOWN/BLOCKED清单。

### H4/H5实际通过后才可写

- 完成双编码器测速、双电机速度PID和500ms命令看门狗。
- 打通 `/cmd_vel` 到STM32双执行通道及反馈的ROS 2闭环。
- 发布并验证架空 `/bench/odom_estimate`，记录真实延迟和错误统计。

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
