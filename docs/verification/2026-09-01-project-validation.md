# 2026-09-01 项目闭环验证记录

## 结论

- STM32 H2 板级功能验收：`PASS`。
- ROS 2 容器构建与测试：`PASS`。
- Gazebo + SLAM 运行验收：`PASS`。
- H0/H1 电气安全门：`BLOCKED`，禁止电机通电和非零运动命令。

## STM32 与 UART

- ST-Link：V2J48S7，Device ID `0x413`，1 MiB Flash，Cortex-M4。
- 目标参考电压：3.28–3.29 V；该读数不是万用表独立测量。
- 固件构建：GNU Tools for STM32 14.3，`text=9084`、`data=20`、`bss=16532`。
- 固件下载与逐字节 verify：通过。
- UART：USART1 PA9/PA10，115200 8N1。
- 启动响应：`H2-READY`。
- PING/PONG：10000/10000 通过，无额外字节。
- 未知命令和超长输入：均返回 `H2-ERR`。
- ST-Link 复位 + 串口重连：20/20 通过。
- 压力回归：连续 10000/10000 次 PING/PONG、20/20 次复位重连通过。
- 错误恢复：100/100 个未知命令、100/100 个超长输入均返回 `H2-ERR`，随后正常 PING/PONG 通过。
- 串口关闭 1 秒后不复位 MCU，重新打开 COM4 并发送 PING，收到 `H2-PONG`。
- 连续输入 1000 个孤立回车后，正常 PING/PONG 仍通过。

## ELF 段权限

GNU ld 14.3 最初报告 Flash `LOAD` segment 具有 `RWE` 权限。链接脚本已增加显式程序头，将 Flash 固定为 `R E`、RAM 固定为 `RW`。重新构建后警告消失，`readelf -lW` 实测为：

```text
LOAD 0x08000000 ... R E
LOAD 0x20000000 ... RW
LOAD 0x2000000c ... RW
```

当前 ELF SHA-256 为 `6E6620065852C3AEA67003FAC2DAC75A7827B9B15E2DF857419ED039D8B5BBFB`；重新下载、verify 和完整 UART 回归均通过。

## 已解决的通信风险

- 未完成命令超过 500 ms 后静默清空，不产生额外串口响应。
- 发送 `H2-`、停顿 750 ms、再发送 `PING\r\n`，实测返回 `H2-ERR`；随后完整 PING 返回 `H2-PONG`。
- 使用 9600 波特率发送数据，等待 750 ms 后切回 115200，第一条完整 PING 实测直接返回 `H2-PONG`。
- USART 奇偶校验、帧、噪声或溢出错误标志会在接收前清除，同时重置解析状态。

重复命令：

```powershell
.\scripts\test_h2_uart.ps1 `
  -Port COM4 `
  -ProgrammerCli 'C:\path\to\STM32_Programmer_CLI.exe'
```

下载前 1 MiB Flash 已备份到 `log/h2-20260901/stm32-flash-before-download.bin`，SHA-256 为 `06CD3FF0D791392E263AD06D1A2AAF23C93A436776642B84D76632EF1AC02E0F`。

## ROS 2 构建与测试

执行：

```powershell
docker compose -f docker/compose.yaml build dev
docker compose -f docker/compose.yaml run --rm -T dev ./scripts/build_ros.sh
```

实际结果：

```text
Summary: 11 packages finished
Summary: 87 tests, 0 errors, 0 failures, 0 skipped
```

## Gazebo 与 SLAM

执行 `./scripts/run_sim.sh` 后，分别运行 `./scripts/check_topics.sh` 和 `./scripts/check_slam.sh`。

实际结果：

- `/scan`、`/odom`、`/joint_states`、`/map` 全部存在。
- `/scan` 实测平均频率为 9.990 Hz。
- `odom -> base_footprint`、`base_footprint -> base_link`、`base_link -> laser_link`、`map -> odom` 全部可查询。
- `slam_toolbox` 正常运行且 `use_sim_time=true`。
- 未检测到 Gazebo 与真机驱动同时发布 odom TF。
- `check_topics.sh`：0 个失败、0 个警告。
- `check_slam.sh`：0 个失败、0 个警告。

Gazebo 在无音频设备的容器中输出 ALSA/OpenAL 警告，但不影响传感器、TF、里程计和 SLAM 验收。

## 未通过与禁止外推

- 未使用万用表独立测量开发板电源、CH340 TX 空闲电平、编码器供电或 A/B 输出电平。
- 当前驱动方案为两块 DRV8871；用户已提供版型图和芯片实物近照，端子与芯片型号已确认，但 ILIM 电阻精确值和持续电流仍无测量证据。
- 未确认电机启动/堵转电流、DRV8871 温升、VM 去耦、保险丝和限流电源。
- 未冻结两块 DRV8871 控制输入、故障输入（若有）和编码器定时器引脚。
- 未实现正式二进制协议、CRC、500 ms 看门狗、ROS 2 真机串口驱动或里程计。
- 因此不得宣称电机、编码器、PID、真机 `/odom` 或超时停车已经完成。

## 下一步安全门

只有完成万用表、电源保护和编码器电平实测后，才能冻结引脚并进入 H3。若这些条件暂时无法满足，项目应继续走安全降级路径：正式协议测试向量、错误帧、通信超时和 ROS 2 串口驱动的仿真/回环测试。
