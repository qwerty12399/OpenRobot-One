# STM32F407 双电机引脚配置设计

## 目标

在现有 `firmware/openrobot_firmware/OpenRobotFirmware.ioc` 基础上使用本机 STM32CubeMX 6.7.0 配置双 BTS7960、双编码器和 100 Hz 控制时基，重新生成 HAL 代码，同时保留已经实测通过的 H2 USART1 逻辑和默认零输出边界。

本任务完成可供后续闭环测试直接使用的引脚、外设初始化与无动力运行基线：启动双编码器、零占空比PWM和100 Hz控制时基并取得板上证据。不实现PID、运动协议或非零电机命令，也不执行电机动力测试；因此交付状态是“闭环前置配置实际通过”，不是“H4真机闭环通过”。

## 硬件基线

- MCU：STM32F407ZGT6，LQFP144。
- HSE：8 MHz；SYSCLK：168 MHz。
- APB1：42 MHz；APB1定时器时钟：84 MHz。
- CH340：USART1，PA9/PA10，115200 8N1。
- SWD：PA13/PA14。
- 状态 LED：PC13，初始高电平。
- 电机：GA25-370，12 V/620 RPM。
- 编码器：AB相，3.3 V供电基线，图片标称11 PPR/22 CPR。
- 驱动：两块IBT-2/BTS7960，逻辑VCC为5 V，PWM上限图片标称25 kHz。

## 最终资源分配

| 功能 | 引脚 | CubeMX配置 |
| --- | --- | --- |
| 左 RPWM | PC6 | TIM3_CH1 / PWM Generation CH1 |
| 左 LPWM | PC7 | TIM3_CH2 / PWM Generation CH2 |
| 右 RPWM | PB0 | TIM3_CH3 / PWM Generation CH3 |
| 右 LPWM | PB1 | TIM3_CH4 / PWM Generation CH4 |
| 左编码器 A | PA0 | TIM2_CH1 / Encoder Interface |
| 左编码器 B | PA1 | TIM2_CH2 / Encoder Interface |
| 右编码器 A | PE9 | TIM1_CH1 / Encoder Interface |
| 右编码器 B | PE11 | TIM1_CH2 / Encoder Interface |
| 左 R_EN | PC0 | GPIO Output，初始LOW |
| 左 L_EN | PC1 | GPIO Output，初始LOW |
| 右 R_EN | PC2 | GPIO Output，初始LOW |
| 右 L_EN | PC3 | GPIO Output，初始LOW |
| 控制周期 | 无外部引脚 | TIM6 Update，100 Hz |

GPIO标签固定为 `LEFT_R_EN`、`LEFT_L_EN`、`RIGHT_R_EN`、`RIGHT_L_EN`。四个EN不在初始化结束后自动拉高。

四个EN在BTS输入端各需要约10 kΩ外部下拉至GND。软件初始LOW只覆盖`MX_GPIO_Init()`之后的状态；没有外部下拉时，本任务只能在两块BTS逻辑/动力均断开的条件下验证STM32配置，不能证明复位窗口驱动禁用。

## 定时器参数

### TIM3 PWM

- Prescaler：0。
- Counter Period：4199。
- PWM频率：`84 MHz / 4200 = 20 kHz`。
- Counter Mode：Up。
- Clock Division：DIV1。
- Auto-reload preload：Enable。
- CH1–CH4：PWM mode 1，Pulse=0，Polarity=High，Fast Mode=Disable。

### TIM2左编码器

- Encoder Mode：TI1 and TI2。
- Prescaler：0。
- Counter Period：65535。
- IC1/IC2：Rising、Direct TI、DIV1、Filter=4。
- 不启用TIM2中断，由TIM6周期读取CNT。

### TIM1右编码器

- 参数与TIM2一致。
- 不启用TIM1中断，由TIM6周期读取CNT。

PB6/PB7 未使用：核心板原理图和实物图将其连接到 32.768 kHz RTC 晶振，避免与 TIM4 编码器复用。

### TIM6控制时基

- Internal Clock。
- Prescaler：8399。
- Counter Period：99。
- Update频率：`84 MHz / 8400 / 100 = 100 Hz`。
- 启用TIM6_DAC全局中断，抢占优先级0，子优先级0（CubeMX默认值）。

USART1是否改为中断接收不属于本任务；保留当前H2轮询收发，避免扩大实现范围。TIM6中断不能发送阻塞UART数据。

## 代码生成要求

使用STM32CubeMX 6.7.0打开现有 `.ioc`，保持STM32Cube FW_F4 V1.27.1和STM32CubeIDE工具链，启用保留USER CODE。由CubeMX生成而不是手工拼接 `.ioc`。

预期生成或更新：

- `OpenRobotFirmware.ioc`；
- `Core/Inc/main.h`；
- `Core/Inc/stm32f4xx_it.h`；
- `Core/Src/main.c`；
- `Core/Src/stm32f4xx_hal_msp.c`；
- `Core/Src/stm32f4xx_it.c`；
- `Core/Inc/stm32f4xx_hal_conf.h`；
- 必要的STM32CubeIDE工程元数据。

生成前记录这些文件的Git状态。`STM32CubeIDE/.settings/language.settings.xml`已有用户环境哈希改动，不将其混入本任务补丁。

## 启动与故障状态

1. GPIO端口时钟使能后，先把PC0–PC3写为LOW。
2. TIM3四个CCR初始为0。
3. 启动PWM不会自动使能BTS输出。
4. 启动TIM3四路PWM时CCR保持0，启动TIM2/TIM1编码器和TIM6中断；四个EN全程保持LOW。
5. TIM6回调只递增一个可由调试器观察的100 Hz计数器，不运行PID，也不改变PWM或EN。
6. 在`Error_Handler`进入死循环前再次把PC0–PC3写LOW并把TIM3 CCR1–CCR4清零；Cortex Fault入口的完整安全停车属于后续固件安全任务。
7. 在完整Fault安全路径完成前，真机非零运动保持禁止。

这一边界避免“仅配置引脚”意外变成可输出非零PWM的运动固件。

## 验证标准

### 静态验证

- CubeMX无红色引脚冲突。
- `.ioc`中存在TIM1、TIM2、TIM3、TIM6和PC0–PC3。
- USART1仍为PA9/PA10；SWD仍为PA13/PA14；PC13保持不变。
- TIM3的PSC/ARR对应20 kHz，四路Pulse均为0。
- TIM6的PSC/ARR对应100 Hz且NVIC已启用。
- HAL TIM模块启用；生成代码包含`htim1/2/3/6`和`TIM6_DAC_IRQHandler`。
- H2 UART USER CODE未被覆盖。
- `main()`启动双编码器、四路CCR=0的PWM和TIM6中断，四个EN仍为LOW。
- TIM6回调计数在调试器中按100 Hz增长。

### 构建验证

- STM32CubeIDE clean build成功。
- ELF无RWX段回归。
- Docker/ROS测试保持通过，证明固件配置没有影响仿真轨。

### 已连接板卡无动力验证

- 保持两块BTS的12 V动力输入断开。
- 下载、逐字节verify和复位通过。
- H2 UART启动、PING/PONG与异常恢复回归通过。
- 读取GPIO/定时器寄存器确认PC0–PC3为输出低、TIM3 CCR1–CCR4为0、TIM2/3/4/6配置值正确。
- 手转左右电机轴时分别观察TIM2/TIM1 CNT变化；只验证计数链，不在本任务冻结每圈计数。

只有上述结果均有实时证据，才能标记“引脚和外设配置实际通过”。本任务不支持标记电机、编码器、PID或真机闭环通过。

## 与现行文档同步

实施完成后把所有当前有效的引脚说明统一为本设计的TIM3 PWM、TIM1/TIM2编码器方案。对`docs/acceptance.md`只修改现有引脚行，保留用户其他未提交内容；不触碰`硬件参数图片及数据/bts7960/README.md`。

CubeMX生成前，将已修改的`STM32CubeIDE/.settings/language.settings.xml`精确复制到工作区外临时文件并记录SHA-256。生成后无论CubeMX是否改写，都恢复用户版本并复核哈希一致。禁止使用`git add -A`，后续只精确暂存本任务文件。
