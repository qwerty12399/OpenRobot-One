# ROS 2接口与双电机串口协议

## ROS 2接口

### 已验证仿真接口

- 输入：`/cmd_vel`。
- 输出：`/odom`、`/tf`、`/joint_states`、`/scan`、`/map`。

### 计划中的真机台架接口

- `/bench/left_target_rpm`、`/bench/right_target_rpm`；
- `/bench/left_measured_rpm`、`/bench/right_measured_rpm`；
- `/bench/left_encoder_count`、`/bench/right_encoder_count`；
- `/bench/state`、`/bench/fault`、`/diagnostics`；
- `/bench/odom_estimate`。

台架估算不占用标准 `/odom`，也不发布真机TF。

## 差速运动学

```text
v_left  = v - omega * wheel_separation / 2
v_right = v + omega * wheel_separation / 2
rpm = linear_wheel_speed / (2*pi*wheel_radius) * 60
```

轮径、轮距、限速和符号来自版本化参数。未装轮时结果只用于目标换算和架空估算。

## UART帧候选

```text
SOF | version | type | length | sequence | payload | CRC16
```

- 默认115200 8N1，设备名和超时由ROS参数配置。
- CRC候选为CRC-16/CCITT-FALSE，参数和字节序由测试向量冻结。
- 解析器拒绝超长、截断、噪声和错误CRC。

## 命令

| 消息 | 方向 | 语义 |
| --- | --- | --- |
| `SET_WHEEL_RPM` | PC→STM32 | 左右目标RPM、序号和有效期 |
| `STOP` | PC→STM32 | 立即禁用双通道 |
| `HEARTBEAT` | PC→STM32 | 仅传输存活，不刷新运动命令 |
| `WHEEL_STATE` | STM32→PC | 左右计数、RPM、目标、输出和状态 |
| `FAULT` | STM32→PC | 故障码和安全状态 |

## 安全时序

1. 上电EN低、PWM零。
2. 完成自检、收到合法零速和显式授权后才能使能。
3. 只有新 `SET_WHEEL_RPM` 刷新500ms运动看门狗。
4. `STOP`、超时、CRC连续失败、串口断开和Fault立即安全停车。
5. 传输恢复后仍需新的合法运动命令，不恢复旧目标。

## 测试向量

- 零速、正负左右轮速、边界和饱和；
- 正确/错误CRC、截断、粘包、噪声和超长；
- 序号重复、回绕、乱序和丢失；
- 心跳在线但运动命令过期；
- PC/STM32逐字节一致。
