# 2026-08-31 H2 板级验证记录

> 历史时间点说明：本文记录当日板级与UART实测，旧TB6612/DRV8871规划已被两块IBT-2/BTS7960双电机架空台架方案取代。保留原结果作为证据，不将旧接线用于当前实验。

## 安全边界

本轮只执行不接电机的板级检查。TB6612、电机和编码器没有接入或通电证据；H0/H1 仍为 `BLOCKED`，禁止非零电机输出。

## 当前结论

H2 功能状态：`PASS`。H0/H1 电气安全门仍为 `BLOCKED`，禁止进入电机测试。

- 2026-09-01 使用 ST-Link V2、STM32CubeProgrammer 2.23.0 重新连接目标，读取 Device ID `0x413`、1 MiB Flash、Cortex-M4，目标参考电压为 3.28–3.29 V。
- 已完整备份下载前的 1 MiB Flash；备份 SHA-256 为 `06CD3FF0D791392E263AD06D1A2AAF23C93A436776642B84D76632EF1AC02E0F`。
- 新增临时 H2 ASCII 验收逻辑并重新构建；固件 SHA-256 为 `7B5E2C7071FB0576A9E494B313DDF42E1E9FF4DF16A0D9A174FFC1530B600C26`。
- STM32CubeProgrammer 下载、逐字节 verify 和复位均实际通过。
- 热连接寄存器读数确认 PC13 为输出且 ODR13=1；USART1 BRR=`0x02D9`、CR1=`0x200C`，对应 115200 与 UART/TX/RX 使能。
- CH340 枚举为 COM4；启动收到 `H2-READY`，连续 100 次 `H2-PING`/`H2-PONG` 全部通过，未知命令和超长输入均返回 `H2-ERR`，无额外字节。
- 10 次 ST-Link 复位、串口关闭/重开和单次 PING/PONG 全部通过。
- 以上结果证明板级下载、运行和原始 UART 双向通信；不证明正式二进制协议、看门狗、电机或编码器链路。

- `.ioc` 的 MCU、SWD、状态 LED、USART1 和系统时钟静态配置一致，未发现引脚重复分配。
- STM32CubeIDE 2.2.0、STM32CubeMX 6.18.1 和 STM32CubeProgrammer 2.23.0 已安装。
- 首次枚举结果为 `No ST-Link detected!` 和 `0 serial ports`；用户接入后 ST-Link V2 已成功枚举，固件为 V2J35S7。
- ST-Link 报告目标参考电压 3.28 V，但常速和 400 kHz 两次 SWD 连接均返回 `Unable to get core ID` / `No STM32 target found`。
- 用户要求重试后，ST-Link 再次正常枚举，但 400 kHz SWD 连接仍返回相同错误；已停止继续重试，未执行擦除或烧写。
- 改用 USB-A 转 USB-C 后开发板电源灯正常，确认原 C-to-C 供电方式不适用于当前板卡。
- 增接 `ST-Link RST -> 开发板 RST` 后执行 400 kHz connect-under-reset，仍无法取得 Core ID；供电与复位方式已不足以解释故障，下一步必须核对实际针脚和接触。
- 用户重新调整接线后再次执行 connect-under-reset，成功读取 Device ID `0x413`、STM32F405/407/415/417 系列、1 MiB Flash、Cortex-M4；SWD 板级连接已通过。
- 用户确认现场没有万用表，无法独立测量板卡 5 V、3.3 V、公共地连续性或短路；该条件同时阻塞 H0 和 H2 后续实物操作。
- Windows 也没有枚举到 CH340、STM32 Virtual COM 或其他串口。
- CubeMX 在临时目录的静默代码生成尝试停在旧 `.ioc` 迁移阶段；未生成工程，也未改写仓库中的 `.ioc`。
- CubeMX 可见界面载入旧工程时要求下载约 54 MB 的 CubeMX 6.7.0 兼容数据库包；下载已在安装前取消，等待用户明确授权。
- 因此，代码生成、编译、下载、LED 初态和 UART 双向通信均不能标记为通过。

原始日志保存在 `log/h2-20260831/` 和 `log/h2-cubemx-generate-20260831.txt`。

## 已确认的板级资源

| 功能 | 配置 | 状态 |
| --- | --- | --- |
| MCU | STM32F407ZGT6，LQFP144 | 静态确认 |
| HSE | PH0/PH1，8 MHz | 静态确认；未板测起振 |
| 系统时钟 | SYSCLK/AHB 168 MHz，APB1 42 MHz，APB2 84 MHz | 静态确认；未核对生成代码 |
| SWDIO | PA13 | 保留，无冲突 |
| SWCLK | PA14 | 保留，无冲突 |
| USART1 TX | PA9 | 保留，无冲突 |
| USART1 RX | PA10 | 保留，无冲突 |
| 状态 LED | PC13，低有效，初始高电平 | `.ioc` 已锁定；未板测 |
| USART1 | 异步，115200 | 波特率静态确认；8N1/TX_RX 待生成代码确认 |

`.ioc` 当前只有 8 个引脚/虚拟资源，没有 PWM、TB6612 方向/STBY 或编码器输入。不存在电机引脚冲突，但这也意味着电机相关引脚尚未冻结。

## H2 检查表

| 检查项 | 当前状态 | 通过证据 |
| --- | --- | --- |
| MCU 为 STM32F407ZGT6 | PASS / 静态 | `.ioc` 的 `Mcu.CPN` 与 `Mcu.UserName` |
| PA13/PA14 保留给 SWD | PASS / 静态 | `.ioc` 的 `SYS_JTMS-SWDIO`、`SYS_JTCK-SWCLK` |
| PA9/PA10 保留给 USART1 | PASS / 静态 | `.ioc` 的 `USART1_TX/RX` |
| PC13 为低有效状态 LED，初始关闭 | PASS / 静态 | `PinState=GPIO_PIN_SET`；仍需实物确认 |
| HSE 8 MHz、SYSCLK 168 MHz | PASS / 静态 | `.ioc` 时钟参数；仍需生成代码和板测 |
| CubeMX 兼容加载与生成工程 | PASS / 已有生成工程 | 现有 CubeIDE 工程可使用 GNU Tools for STM32 14.3 构建；未重新执行 CubeMX 迁移 |
| CubeIDE 构建 | PASS / 实测 | `make all -j4` 成功；存在链接器 RWX LOAD segment 警告，未影响下载 |
| ST-Link 探针枚举 | PASS / 实测 | ST-Link V2，V2J35S7，序列号已写入原始日志 |
| MCU SWD 连接 | PASS / 实测 | 最终 connect-under-reset 成功，Device ID `0x413`，1 MiB Flash，Cortex-M4 |
| 下载和 verify | PASS / 实测 | STM32CubeProgrammer 下载、verify、复位成功 |
| 板卡 5 V / 3.3 V 供电测量 | BLOCKED | 现场无万用表；不得用电源灯或 ST-Link 电压读数代替 |
| PC13 上电初态 | PASS / 寄存器实测 | GPIOC MODER=`0x04000000`、ODR=`0x00002000`；未用肉眼替代寄存器证据 |
| CH340 为 3.3 V 逻辑 | PARTIAL | COM4 实际通信通过；仍无万用表独立测量 TX 空闲电平 |
| USART1 115200 8N1 双向收发 | PASS / 实测 | 100/100 往返、错误输入、超长输入、10/10 复位重连通过 |
| PWM、方向、STBY、编码器引脚冻结 | NOT STARTED | H0/H1 电气事实仍不完整 |

## 接入硬件后的最小步骤

1. 保持 TB6612、电机和编码器全部断开。
2. 仅连接开发板供电与 ST-Link；按具体 ST-Link 型号确认 SWDIO、SWCLK、GND 和 VTref/供电方式。
3. 在 CubeMX/CubeIDE 中打开 `.ioc`，明确选择迁移副本，不直接覆盖原文件；确认 STM32CubeF4 固件包来源和版本。
4. 生成工程后核对：
   - `SystemClock_Config()` 使用 HSE 和 168 MHz SYSCLK；
   - `MX_GPIO_Init()` 将 PC13 初始化为高电平；
   - `MX_USART1_UART_Init()` 为 115200、8 data bits、1 stop bit、no parity、TX_RX；
   - 工程中不存在 TIM PWM 或电机输出初始化。
5. 完整构建并保存控制台日志。
6. 使用 CubeProgrammer 执行 ST-Link 下载和 verify，保存原始输出。
7. CH340 切换到 3.3 V，先用万用表测 TX 空闲电平；确认后连接 CH340 TX → PA10、CH340 RX ← PA9、GND ↔ GND，不连接 CH340 5 V。
8. H2 临时使用 ASCII `H2-PING\n` / `H2-PONG\n` 验证原始 UART，不把它声明为正式二进制协议。
9. 连续往返 100 次，并执行 10 次板卡复位、10 次串口重连；保存每次时间戳、TX/RX 十六进制、超时和额外字节统计。

## 正式协议边界

当前只能验证原始 UART。以下内容仍未冻结：SOF、ADDRESS 语义、命令 ID、最大载荷、payload 单位/比例、CRC 覆盖范围和线上字节序、协议版本、序号、握手状态机及 CRC 连续失败阈值。CRC-16/CCITT-FALSE 仍只是候选，不能写成已经实现或互通。

## 下一安全门的最小解锁动作

用户需要先完成两件事：

1. 准备具备直流电压和通断档的数字万用表，先验证开发板 5 V、3.3 V 和公共地。
2. 使用万用表独立测量 CH340 TX 空闲电平、编码器供电和 A/B 输出电平。
3. 核对限流电源、保险丝、总开关和 TB6612 VM 去耦；这些条件完成前不得进入 H3。

重复 H2 UART 验收：

```powershell
.\scripts\test_h2_uart.ps1 `
  -Port COM4 `
  -ProgrammerCli 'C:\path\to\STM32_Programmer_CLI.exe'
```

原始摘要保存在 `log/h2-20260901/uart-validation.txt`；下载前 Flash 备份保存在同目录。H2 已通过，但 H0/H1 未通过，结论不得外推到电机链路。
