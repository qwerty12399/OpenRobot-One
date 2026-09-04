# STM32 双电机安全短脉冲诊断设计

## 目标

在现有 STM32F407ZGT6、两块 IBT-2/BTS7960 和双编码器接线基础上，增加一个仅用于 H3 逐通道验收的串口诊断功能。测试必须由一次性授权触发，每次只允许一台电机按固定 20% PWM 运行 100 ms，并由 TIM6 中断独立强制停车。

本阶段只验证驱动方向、74HC244 对 3.3 V 控制信号的识别、编码器符号和自动停车，不实现持续运动、双电机同时运行、PID 或 ROS 2 正式协议。

## 当前硬件与引脚基线

| 功能 | 引脚/外设 |
| --- | --- |
| 左 RPWM / LPWM | PC6/TIM3_CH1、PC7/TIM3_CH2 |
| 右 RPWM / LPWM | PB0/TIM3_CH3、PB1/TIM3_CH4 |
| 左 R_EN / L_EN | PC0、PC1 |
| 右 R_EN / L_EN | PC2、PC3 |
| 左编码器 | PA0/TIM2_CH1、PA1/TIM2_CH2 |
| 右编码器 | PE9/TIM1_CH1、PE11/TIM1_CH2 |
| 控制周期 | TIM6，100 Hz |
| 主机串口 | USART1，PA9/PA10，115200 8N1 |

TIM3 周期为 4199，因此 20% 占空比比较值固定为 840。上电、复位和异常状态下四路 PWM 比较值必须为 0，PC0-PC3 必须为低。

## 方案选择

采用“主循环解析命令 + TIM6 中断强制停机”的非阻塞状态机。

- 不使用 `HAL_Delay(100)`，避免脉冲期间失去串口和 STOP 响应能力。
- 不只依赖主循环 `HAL_GetTick()`，避免当前阻塞式 UART 接收令 100 ms 停车明显延迟。
- TIM6 每 10 ms 更新一次脉冲倒计时；第 10 次中断必须直接清零四路 PWM、拉低四个 EN，并通知主循环输出结果。
- UART 发送、字符串格式化和结果报告不得在中断中执行。

## 状态机

状态只有三种：

1. `IDLE`：所有 PWM 为 0、所有 EN 为低。
2. `ARMED`：所有输出仍关闭，一次性授权最长保留 5 秒。
3. `PULSING`：仅一个电机、一个方向以 20% PWM 运行，最长 10 个 TIM6 周期。

状态转换：

```text
启动/复位/异常 -> IDLE
IDLE -- MOTOR-ARM --> ARMED
ARMED -- 5秒超时 --> IDLE
ARMED -- 合法MOTOR-PULSE --> PULSING
PULSING -- 10个TIM6周期 --> IDLE
任意状态 -- MOTOR-STOP/串口错误/解析错误 --> IDLE
```

授权在接受合法 `MOTOR-PULSE` 时立即消耗。一次授权只能产生一次脉冲，不允许排队、重入或自动重复。

## 串口命令

所有命令使用 ASCII，以 `\n` 结束，并兼容前置 `\r`。

| 命令 | 行为 |
| --- | --- |
| `H2-PING` | 保留原有响应 `H2-PONG` |
| `MOTOR-ARM` | 仅在 IDLE 接受，进入 ARMED，回复 `MOTOR-ARMED` |
| `MOTOR-PULSE LEFT FWD` | 左 RPWM=20%、左 LPWM=0，运行 100 ms |
| `MOTOR-PULSE LEFT REV` | 左 RPWM=0、左 LPWM=20%，运行 100 ms |
| `MOTOR-PULSE RIGHT FWD` | 右 RPWM=20%、右 LPWM=0，运行 100 ms |
| `MOTOR-PULSE RIGHT REV` | 右 RPWM=0、右 LPWM=20%，运行 100 ms |
| `MOTOR-STOP` | 任意状态立即停车并回到 IDLE |
| `MOTOR-STATUS` | 返回状态、PWM/EN安全状态和两个编码器当前计数 |

商品资料中的 RPWM/LPWM 正反转文字存在冲突，因此 `FWD/REV` 在本阶段只是两个互斥的电气方向标签。实测后再冻结它们与车体前进方向的对应关系。

未授权的脉冲命令回复 `MOTOR-DENIED` 并保持安全停车。未知命令沿用 `H2-ERR`。ARM、PULSE、STOP 和状态响应均不得刷新未来正式运动看门狗；该看门狗不属于本阶段。

响应格式固定如下，所有响应均以 `\r\n` 结束：

```text
MOTOR-ARMED
MOTOR-DENIED
MOTOR-STOPPED
MOTOR-STATUS STATE=<IDLE|ARMED|PULSING> LEFT=<0..65535> RIGHT=<0..65535>
MOTOR-DONE SIDE=<LEFT|RIGHT> DIR=<FWD|REV> START=<0..65535> END=<0..65535> DELTA=<-32768..32767>
```

在非 `IDLE` 状态再次发送 `MOTOR-ARM`，或在非 `ARMED` 状态发送 PULSE，均回复 `MOTOR-DENIED`、调用安全停车并回到 `IDLE`。状态响应只报告观测值，不改变授权或脉冲状态。

## 脉冲执行

接受合法脉冲命令时按以下顺序执行：

1. 调用统一安全停车，确保四路 PWM=0、四个 EN=0。
2. 保存对应编码器的 16 位起始计数。
3. 清除完成标志，设置剩余周期为 10，状态改为 `PULSING`。
4. 仅设置所选方向的 TIM3 比较值为 840，另一方向保持 0。
5. 仅拉高所选电机的 R_EN 和 L_EN；另一电机两个 EN 保持低。
6. TIM6 每次中断递减剩余周期；变为 0 时先清 PWM，再清 EN。
7. 中断保存结束计数并设置完成标志；主循环读取快照并发送结果。

停车函数还必须清除 ARMED 授权、脉冲倒计时和当前通道。为避免主循环与中断竞争，共享状态使用 `volatile`；主循环复制多字段结果时短暂关闭 TIM6 中断或进入临界区，复制后立即恢复。

## 编码器结果

左右计数器均为 16 位。脉冲计数差使用回绕安全计算：

```c
delta = (int16_t)(end_count - start_count);
```

完成响应至少包含：通道、命令方向、起始计数、结束计数和有符号增量。报告只用于验收，不将计数增量换算为 RPM，因为 100 ms 脉冲可能包含静摩擦和加减速过程。

## 异常处理

以下情况统一调用安全停车：

- 上电与复位；
- `MOTOR-STOP`；
- ARM 超时；
- 未授权或状态不允许的脉冲请求；
- UART PE、FE、NE、ORE 错误；
- 命令缓冲区溢出或半包超时；
- `Error_Handler()`；
- HardFault 及其他已接管的致命异常。

TIM6 到期停车不依赖主循环、UART或主机进程。断开串口不能延长已经开始的脉冲。

## 验证顺序与通过标准

1. 无 12 V 动力构建并烧录，确认启动后四个 EN 为 0 V、四路 PWM 为 0。
2. 不发送 ARM 直接发送四种 PULSE，全部返回 `MOTOR-DENIED`，输出保持关闭。
3. 发送 ARM 后等待超过 5 秒再发 PULSE，应拒绝且保持关闭。
4. 发送 ARM 和左 FWD，仅左路产生一次 20%/100 ms 脉冲，自动停车并返回 TIM2 增量。
5. 左 REV、右 FWD、右 REV 逐项重复，每个台阶都重新 ARM。
6. 脉冲期间发送 STOP，应提前停车。
7. 分别在脉冲期间断开CH340、复位STM32和退出上位机，均不得产生持续输出。
8. 保存四次脉冲的串口原始输出、编码器增量、电流和视频证据。

通过标准：无授权不输出；任一授权只触发一台电机的一次 20% 脉冲；正常到期时输出持续时间为 100 ms，允许一个 TIM6 周期的测量分辨率；停止后四路 PWM 和四个 EN 全部归零；左右编码器均返回非零且方向可解释的增量。20% 是本阶段上限，不允许自动递增或连续重试。

## 明确不在本阶段实现

- 连续PWM或任意占空比命令；
- 双电机同时运行；
- 编码器每圈计数最终标定；
- RPM估算、滤波和PID；
- 正式二进制协议、CRC和序号；
- ROS 2串口节点及 `/cmd_vel`；
- `/bench/odom_estimate`。
