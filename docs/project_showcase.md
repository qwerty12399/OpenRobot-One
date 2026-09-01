# OpenRobot-One 项目成果与验证证据

本文是仓库的技术核验入口，面向技术支持、FAE、项目交付、实施、机器人系统/硬件测试和售前岗位。所有结论只引用仓库中现有实现、测试脚本或验证记录。

## 项目角色与职责

OpenRobot-One 是个人主导的 ROS 2 与 STM32 双轨移动机器人项目，职责覆盖：

- 将目标拆分为仿真轨、真机轨和 H0–H4 验收门；
- 统一 `/cmd_vel`、`/odom`、`/tf`、`/joint_states`、`/scan` 接口；
- 搭建 Docker、colcon、ament 和 GitHub Actions 工程基线；
- 编写 ROS 2 检查脚本、STM32 UART 验收脚本和阶段验证报告；
- 对接 ST-Link、CH340、STM32F407 和候选电机驱动方案；
- 记录 UNKNOWN、BLOCKED 和禁止外推项，避免在安全条件不足时执行电机测试。

## 按岗位查看

### 技术支持 / FAE / 售后支持

- [H2 板级验证报告](verification/2026-08-31-h2-board-bringup.md)：从无法连接目标到完成构建、烧录、verify 和 UART 验收。
- [UART 自动化测试脚本](../scripts/test_h2_uart.ps1)：覆盖压力、错误输入、半包、错误波特率和复位重连。
- [接口与协议](protocol.md)：定义 ROS Topic、UART 候选帧和安全时序，并明确尚未冻结的内容。
- [真机硬件手册](OpenRobot-One_真机硬件开发保姆级手册.docx)：面向零基础执行者的步骤化交付文档。

### 项目交付 / 实施

- [系统架构](architecture.md)：双轨运行模式、接口边界和 TF 唯一发布者。
- [功能验收标准](acceptance.md)：按阶段定义入口条件、证据和停止条件。
- [开发路线](roadmap.md)：按依赖关系安排仿真、真机、视觉和任务模块。
- [环境配置](environment.md)：固定 Ubuntu 22.04、ROS 2 Humble、Gazebo Classic 11 和容器化入口。

### 机器人系统 / 硬件测试

- [项目闭环验证](verification/2026-09-01-project-validation.md)：ROS 2、Gazebo/SLAM、STM32 与 UART 的最新汇总证据。
- [ROS 2 仓库测试](../ros2_ws/src/openrobot_tests/test)：结构、包边界和脚本入口检查。
- [Topic 检查](../scripts/check_topics.sh)与[SLAM 检查](../scripts/check_slam.sh)：运行态 Topic、TF 和 SLAM 验收。
- [STM32 固件](../firmware/openrobot_firmware)：可构建、下载和回归的 H2 板级基线。

### 售前 / 技术销售

- [系统架构](architecture.md)：解释上位机、ROS 2、串口、MCU 和执行器边界。
- [硬件 BOM](hardware_bom.md)：区分目标方案、已有物料和必须实测参数。
- [硬件事实](hardware_facts.md)：区分照片/资料确认、用户提供信息和 UNKNOWN。
- [完成度与限制](#当前限制与安全边界)：明确当前可交付范围，避免不受控承诺。

## 验证总览

| 范围 | 状态 | 环境 / 日期 | 结果 | 证据 |
| --- | --- | --- | --- | --- |
| ROS 2 构建与测试 | **实际通过** | Ubuntu 22.04 / ROS 2 Humble Docker，2026-09-01 | 11 packages；87 tests；0 errors / failures / skipped | [闭环验证](verification/2026-09-01-project-validation.md#ros-2-构建与测试)、[CI 工作流](../.github/workflows/ros2_build.yml) |
| Gazebo + SLAM | **实际通过** | Gazebo Classic 11，2026-09-01 | 必需 Topic 与四条 TF 存在；`/scan` 9.990 Hz；检查脚本 0 失败、0 警告 | [运行结果](verification/2026-09-01-project-validation.md#gazebo-与-slam) |
| STM32 构建与下载 | **实际通过** | STM32F407ZGT6、ST-Link、GNU Tools for STM32 14.3，2026-09-01 | 构建、下载、逐字节 verify、复位通过 | [H2 报告](verification/2026-08-31-h2-board-bringup.md) |
| UART 双向与压力测试 | **实际通过** | CH340、USART1 115200 8N1，2026-09-01 | `10000/10000` PING/PONG；`20/20` 复位重连 | [闭环验证](verification/2026-09-01-project-validation.md#stm32-与-uart) |
| UART 边界与恢复 | **实际通过** | 同上，2026-09-01 | 未知/超长输入、半包超时、错误波特率、串口重开后恢复 | [通信风险](verification/2026-09-01-project-validation.md#已解决的通信风险)、[测试脚本](../scripts/test_h2_uart.ps1) |
| MCU、时钟与引脚基线 | **静态检查 + 部分实测** | CubeMX `.ioc`、寄存器、实物板 | MCU、SWD、USART1、PC13 基线确认；电机/编码器引脚未冻结 | [H2 检查表](verification/2026-08-31-h2-board-bringup.md#h2-检查表) |
| 电机、编码器、PID、真机里程计 | **未验证** | H0/H1 仍 BLOCKED | 禁止宣称完成或发送非零命令 | [未通过与禁止外推](verification/2026-09-01-project-validation.md#未通过与禁止外推) |

## 典型问题定位案例

### 1. ST-Link 可枚举但无法连接 MCU

- **现象：** ST-Link 能被 Windows 识别，但连接返回 `Unable to get core ID` / `No STM32 target found`。
- **假设：** 供电方式、复位链路、SWD 接线或针脚接触存在问题。
- **检查：** 更换不适用的 C-to-C 供电方式，分离检查供电和复位，增加 RST，使用 400 kHz connect-under-reset，并重新核对实际接线。
- **结论：** 调整连接后成功读取 Device ID `0x413`、1 MiB Flash 和 Cortex-M4，随后完成下载与 verify。
- **防复发：** 验收步骤固定记录探针枚举、目标电压、Device ID、下载、verify 和复位结果，不用“电源灯亮”代替 SWD 证据。

证据：[H2 板级验证记录](verification/2026-08-31-h2-board-bringup.md)。

### 2. UART 异常输入后的恢复能力

- **现象：** 半包、错误波特率、超长输入或串口重开可能让解析器停留在异常状态。
- **假设：** 接收状态未及时清理，错误标志或残留输入影响下一条合法命令。
- **检查：** 增加 500 ms 半包清理、USART 错误状态清除和解析状态复位；自动发送边界输入后立即执行合法 PING。
- **结论：** 半包后返回 `H2-ERR`，错误波特率切回 115200、串口重开和大量孤立回车后均可恢复 `H2-PONG`。
- **防复发：** 将异常恢复固化在 `scripts/test_h2_uart.ps1`，与压力和复位重连一起回归。

证据：[已解决的通信风险](verification/2026-09-01-project-validation.md#已解决的通信风险)。

### 3. 固件链接段权限异常

- **现象：** GNU ld 14.3 报告 Flash `LOAD` segment 同时具有读、写、执行权限。
- **假设：** 链接脚本缺少显式程序头，导致输出段权限过宽。
- **检查：** 修改链接脚本程序头，使用 `readelf -lW` 检查最终 ELF。
- **结论：** Flash 固定为 `R E`，RAM 为 `RW`；警告消失，重新下载、verify 和完整 UART 回归通过。
- **防复发：** 将 ELF 段权限纳入固件验证记录，不仅依赖“能够烧录”。

证据：[ELF 段权限验证](verification/2026-09-01-project-validation.md#elf-段权限)。

## 交付物索引

| 交付物 | 用途 |
| --- | --- |
| [README](../README.md) | 项目概览、岗位入口和快速复现 |
| [系统架构](architecture.md) | 系统边界、运行模式、数据流和 TF 所有权 |
| [功能验收标准](acceptance.md) | 阶段门、通过条件和安全停止条件 |
| [接口与协议](protocol.md) | ROS Topic、UART 帧候选和安全时序 |
| [硬件 BOM](hardware_bom.md) | 物料、选型依据和待测参数 |
| [硬件事实](hardware_facts.md) | 已确认事实、UNKNOWN 和候选资源 |
| [项目路线](roadmap.md) | 后续实现顺序和依赖关系 |
| [项目闭环验证](verification/2026-09-01-project-validation.md) | 最新 ROS 2、仿真和 STM32 汇总结果 |
| [H2 板级验证](verification/2026-08-31-h2-board-bringup.md) | STM32 连接、构建、烧录、寄存器和 UART 证据 |
| [UART 测试脚本](../scripts/test_h2_uart.ps1) | 可重复的板级通信回归 |
| [ROS 构建脚本](../scripts/build_ros.sh) | rosdep、colcon build、test 和结果汇总 |

## 当前限制与安全边界

- **H0/H1：BLOCKED。** 当前缺少万用表独立测量、公共地连续性、电机启动/堵转电流、DRV8871 限流和温升、保险丝与限流电源证据。
- **H2：PASS。** 只证明板级下载、运行、寄存器状态和原始 UART 双向通信。
- **H3/H4：未开始。** 电机、编码器、双轮 PID、正式二进制协议、ROS 2 真机驱动和真机 `/odom` 未完成。
- 在 H0/H1 解锁前，禁止电机通电和任何非零运动命令。
- 视觉、任务管理、受限 LLM 和语音是后续路线，不计入当前完成成果。
- 仿真和真机不得同时发布 `odom → base_footprint`；SLAM 与 AMCL 不得同时发布 `map → odom`。

下一步是补齐电气安全装备和测量证据，冻结 DRV8871、编码器和定时器引脚，再进入架空、限流、低速的 H3 验收。
