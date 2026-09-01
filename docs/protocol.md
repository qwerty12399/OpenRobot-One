# ROS 2 接口与串口协议

## 1. ROS 2 接口

| Topic | 类型 | 发布者 | 订阅者 | 状态 |
| --- | --- | --- | --- | --- |
| `/cmd_vel` | `geometry_msgs/Twist` | 遥控、Nav2 或任务管理三选一 | Gazebo 或真机驱动 | 已在仿真使用 |
| `/odom` | `nav_msgs/Odometry` | Gazebo 或真机驱动二选一 | SLAM、AMCL、Nav2 | 仿真已实现 |
| `/tf` | `tf2_msgs/TFMessage` | 按 TF 所有权表 | ROS 生态 | 仿真已实现 |
| `/joint_states` | `sensor_msgs/JointState` | Gazebo 或真机驱动 | `robot_state_publisher` | 仿真已实现 |
| `/scan` | `sensor_msgs/LaserScan` | 仿真/真机雷达 | SLAM、导航 | 仿真已实现 |
| `/vision/object` | 待在 `openrobot_msgs` 冻结 | `openrobot_vision` | `openrobot_task` | 待实现 |
| `/ai/task` | `std_msgs/String`（JSON）或后续自定义消息 | `openrobot_ai` | `openrobot_task` | 待实现 |
| `/task/status` | `std_msgs/String` | `openrobot_task` | UI/日志 | 待实现 |

Topic 名必须通过参数、Launch 参数或 remapping 配置。任务 JSON 在接口稳定前只允许白名单字段：

```json
{
  "task": "search",
  "target": "red cup"
}
```

允许的首版任务为 `search` 和 `stop`。未知字段、未知任务或空目标不得触发运动。

## 2. 目标观测语义

视觉消息最终至少表达：时间戳、目标类别、置信度、图像坐标、边界框或面积比例，以及距离估计是否有效。单目框面积不是可靠距离；没有标定或测距来源时距离字段必须标记无效，任务管理器只能限速搜索/对准，不得高速接近。

## 3. UART 帧

设计基线：

```text
SOF(2) | ADDRESS(1) | COMMAND(1) | LENGTH(1) | PAYLOAD(N) | CRC16(2)
```

- 帧头：建议固定为 `0xAA 0x55`，实现时必须用测试冻结。
- 多字节数值：统一 little-endian。
- 最大载荷：在实现前冻结，解析器必须拒绝超长帧。
- CRC：采用 CRC-16/CCITT-FALSE 候选参数；多项式、初值、反射和字节序必须在 PC/STM32 测试向量一致后冻结。
- 串口默认：115200、8N1；设备名和超时通过 ROS 参数配置。

本文件在协议实现前不宣称候选 CRC 参数已经兼容固件。

## 4. 命令建议

| 命令 | 方向 | 载荷 |
| --- | --- | --- |
| `SET_WHEEL_SPEED` | PC → STM32 | 左右目标轮速，定点整数 |
| `STOP` | PC → STM32 | 无载荷，立即停车 |
| `HEARTBEAT` | 双向 | 协议版本与序号 |
| `CHASSIS_STATE` | STM32 → PC | 左右反馈轮速、编码器、电压、状态位 |
| `FAULT` | STM32 → PC | 故障码与锁存状态 |

线速度/角速度到轮速的换算在 ROS 驱动完成：

```text
v_left  = v - ω × wheel_separation / 2
v_right = v + ω × wheel_separation / 2
```

轮径、轮距和编码器计数必须来自参数，不能写死在源码。

## 5. 安全时序

1. STM32 上电后 PWM 为零，驱动保持禁用。
2. PC 完成握手并验证协议版本。
3. 只有参数有效、无故障且先收到零速命令时才允许使能。
4. PC 以固定频率发送目标；STM32 超过 500 ms 未收到有效控制帧立即停车。
5. PC 驱动在 `/cmd_vel` 超时、串口断开或 CRC 连续失败时发送/保持零速并报告诊断。
6. 重连后不得恢复断线前的非零命令。

## 6. 实现前必须具备的测试向量

- 最小帧、最大合法帧和零速帧的完整十六进制字节。
- CRC 正确/错误、截断、粘包、噪声前缀和超长长度。
- 正/负左右轮速的端序与饱和行为。
- 序号回绕、重复帧和超时停车。
- PC 编码结果与 STM32 解码结果逐字节一致。
