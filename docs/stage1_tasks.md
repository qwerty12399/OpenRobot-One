# 阶段 1 可执行任务清单(真机底盘)

> 目标:从 `/cmd_vel` 到双轮闭环,再回到 `/odom` 和 TF。
> 依据:`docs/roadmap.md` 阶段 1、`docs/protocol.md`、`docs/hardware_facts.md`、`docs/hardware_bom.md`、`docs/acceptance.md`、`AGENTS.md`。
> 原则:每个任务必须给出可复现的验收标准;未实测的参数一律不写入固件与里程计公式;未标定前禁止非零电机命令。

## 依赖关系

```text
T0 安全装备 ──► T1 电气参数实测 ──► T2 引脚冻结 ──► T3 编码器标定 ──► T4 开环测试 ──► T5 PID 闭环
                                          │                              │
                                          └──────────────────────────────┘
T6 协议/看门狗(可并行于 T4/T5)──► T7 ROS 2 驱动与里程计
```

- T0 是 T1 与 T4 的前置,先备齐装备再动硬件。
- T2 依赖 T1 的编码器电平与引脚候选确认;不得在编码器电平未确认前创建会驱动电机的固件代码。
- T6 的测试向量可先行编写,与 T4/T5 并行。

## T0 安全测试装备准备

- **输入**:`docs/hardware_bom.md` 第 3 节
- **动作**:备齐可调限流电源(0–15 V / ≥3 A)、数字万用表、保险丝与座、总电源开关(12 V / 5 A+)、1000 µF/25 V 电解电容、0.1 µF 陶瓷电容、20–22 AWG 电机导线、热缩管/端子
- **交付物**:装备清单核对表(可勾选)
- **验收**:每项可实际使用;缺项不得进入 T1/T4

## T1 电气参数实测与冻结

- **输入**:`docs/hardware_facts.md` 的 "仍为 UNKNOWN" 清单
- **动作**:
  1. 编码器:供电电压、A/B 输出电平、PPR/CPR、减速比、每输出轴一圈计数
  2. 电机:左右各测空载/启动/堵转电流
  3. 驱动:两块 DRV8871 的模块版本、ILIM/限流设定、温升与 VM 去耦配置
  4. 机械:驱动轮有效直径、左右轮接地点中心距
  5. 电源:额定电压、持续电流、保险丝规格
- **交付物**:更新 `docs/hardware_facts.md`;把实测值回填 `openrobot_bringup/config/robot.yaml`(标注来源)
- **验收**:原 UNKNOWN 项全部有实测记录;`encoder_counts_per_wheel_rev > 0` 且标注来源

## T2 STM32 引脚冻结与 CubeMX 配置

- **输入**:`hardware_facts.md` 的候选资源表;T1 的编码器电平结论
- **动作**:
  1. 分配左右编码器定时器(TIM2/TIM3 编码器模式候选)
  2. 按实物模块控制逻辑分配两路电机定时器/GPIO
  3. 若模块引出故障信号，分配并验证对应输入 GPIO
  4. 冲突检查:不得占用 PA13/PA14(SWD)、PA9/PA10(USART1)、PC13(LED)
- **交付物**:更新 `firmware/openrobot_firmware/OpenRobotFirmware.ioc`;引脚分配表写入文档
- **验收**:CubeMX 生成无冲突;板级验收仍通过(PC13 熄灭、USART1 115200 8N1 双向收发,不接电机驱动)

## T3 双编码器计数与每轮标定

- **输入**:T2 冻结的引脚配置
- **动作**:实现固件编码器计数读取(不接电机驱动);手转左右轮各标定 3 次,记录每输出轴一圈计数
- **交付物**:标定表(左右各 3 次);`robot.yaml` 回填每轮计数
- **验收**:每轮 3 次计数离散度 ≤1%(`acceptance.md` §4.1);与 T1 实测一致

## T4 单/双电机开环测试

- **输入**:T0 装备、T3 标定完成
- **动作**:轮子架空、限流电源、保险丝、总开关、有人值守;先单电机低占空比正反转,再双电机;核对电机方向与编码器符号一致
- **交付物**:方向-符号对照记录
- **验收**:`acceptance.md` §4.2;任一安全项失败立即停止后续测试

## T5 双轮速度 PID 闭环

- **输入**:T4 通过
- **动作**:在 STM32 实现 100 Hz 双轮速度 PID 与输出限幅;PID 参数、限幅、轮径轮距全部来自参数,不硬编码
- **交付物**:固件实现 + 参数表
- **验收**:多目标点无持续振荡,稳态误差 ≤10%(`acceptance.md` §4.3);落地直行/原地转向无失控

## T6 冻结二进制协议与通信看门狗

- **输入**:`docs/protocol.md` §3–§5
- **动作**:
  1. 实现帧格式:`SOF(0xAA 0x55)|ADDRESS|COMMAND|LENGTH|PAYLOAD|CRC16`,little-endian,冻结最大载荷
  2. 实现 CRC-16/CCITT-FALSE 与 PC/STM32 测试向量对齐
  3. 实现 500 ms 通信看门狗与安全时序(上电零速、握手、先零速后使能)
  4. 完成 §6 全部测试向量:最小/最大帧、CRC 错/截断/粘包/噪声/超长、端序饱和、序号回绕、超时停车
- **交付物**:协议实现 + 测试向量表(PC 与 STM32 逐字节一致)
- **验收**:测试向量全通过;命令超时/拔串口/ROS 进程退出/MCU 复位均在 500 ms 级停车(`acceptance.md` §4.4)

## T7 ROS 2 串口驱动与里程计/TF

- **输入**:T5、T6 完成
- **动作**:
  1. `openrobot_driver`:串口节点、运动学(`/cmd_vel`→左右轮速)、`/odom`、`odom→base_footprint` TF、诊断、断线重连
  2. `openrobot_control`:速度限制与底盘控制协调(不与固件 PID 重复)
  3. 串口设备/波特率/超时/协议版本经 ROS 参数配置,不硬编码
- **交付物**:驱动与控制包实现;`hardware.launch.py` 从占位改为真实启动
- **验收**:真机轨发布 `/odom` 与 `odom→base_footprint` 且与仿真互斥;TF 无冲突(`architecture.md` §4);重连后不恢复断线前非零命令;断线停车可复现

## 阶段 1 完成标准(汇总)

- 架空与落地测试均可复现
- 断线停车;无 TF 冲突
- 闭环误差达到 `acceptance.md` 要求
- 提交前 `git diff`/`git status` 检查,无越界改动

## 每任务交付前必须执行

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y
colcon build --base-paths ros2_ws/src --event-handlers console_direct+
source install/setup.bash
colcon test --base-paths ros2_ws/src --event-handlers console_direct+
colcon test-result --verbose
```

> 注:当前开发环境为 Windows,ROS 2 Humble 命令未实际执行;所有结果需在 Ubuntu 22.04 / Docker 容器内复跑验证。
