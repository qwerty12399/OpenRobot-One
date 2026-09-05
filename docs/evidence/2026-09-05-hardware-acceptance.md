# 2026-09-05 双电机架空台架验收

## 结论

OpenRobot-One 的 ROS 2 到 STM32 双电机架空控制闭环已完成一键回归，用户
实际执行结果为 7/7 PASS。

本验收只覆盖无轮、架空双电机台架，不等价于落地底盘、真实里程计精度、
真机SLAM或Nav2。

## 实测链路

```text
/cmd_vel
→ openrobot_driver（ROS 2 Humble / C++17）
→ 差速运动学与左右目标RPM
→ UART 115200 8N1
→ STM32F407 100 Hz FF+PI
→ 双BTS7960 / 双编码器电机
→ ASCII遥测
→ /joint_states + /bench/odom_estimate
```

## 7项回归结果

```text
PASS  idle
PASS  forward
PASS  stop_after_forward
PASS  backward
PASS  left_turn
PASS  right_turn
PASS  cmd_timeout

Result: 7/7 tests passed
HARDWARE SMOKE TEST: PASS
```

本次用户提供的关键输出：

| 场景 | 左轮实测 | 右轮实测 | 理论目标 |
| --- | ---: | ---: | ---: |
| 前进 `+0.34 m/s` | `+10.16 rad/s` | `+10.16 rad/s` | `+10.46 / +10.46 rad/s` |
| 后退 `-0.34 m/s` | `-9.84 rad/s` | `-10.16 rad/s` | `-10.46 / -10.46 rad/s` |
| 左转 `+2 rad/s` | `-4.35 rad/s` | `+4.35 rad/s` | `-5.02 / +5.02 rad/s` |
| 右转 `-2 rad/s` | `+4.35 rad/s` | `-4.50 rad/s` | `+5.02 / -5.02 rad/s` |
| 命令超时停车 | `0.00 rad/s` | `0.00 rad/s` | `0.00 / 0.00 rad/s` |

低速旋转数值低于名义运动学目标，但在当前验收脚本25%相对容差内通过。
该容差用于架空功能回归，不代表已完成落地控制精度标定。

## 安全保护

- ROS 2驱动在 `/cmd_vel` 超过300 ms未更新后发送零速目标；
- STM32在有效串口命令超过500 ms未更新后关闭电机输出；
- 验收脚本退出路径再次发送零速命令；
- 非零测试前必须确认双电机安全架空并输入 `YES`。

## TF与里程计边界

- 真机台架发布 `/bench/odom_estimate`，数据来自编码器速度和名义几何参数积分；
- 不发布标准 `/odom`；
- 不广播 `odom → base_footprint` 或其他替代底盘TF；
- 仿真中的标准 `/odom` 与TF仍由Gazebo差速插件唯一发布。

## 可复现命令

```bash
cd /mnt/d/OpenRobot-One
bash scripts/hardware_acceptance.sh
```

预期末尾：

```text
Result: 7/7 tests passed
HARDWARE SMOKE TEST: PASS
[INFO] Stopping hardware bringup...
```

## 未验证与残余风险

- 本轮收尾没有重新给电机上电，7/7结果来自用户已完成的真机运行输出；
- 尚无示波器或逻辑分析仪测得的精确300/500 ms停车延迟；
- 当前ASCII协议没有序号、CRC、累计编码器计数和结构化故障码；
- 轮径、轮距、地面负载、打滑与直线偏差尚未做落地标定；
- 视频是成果展示，不替代原始日志和自动化测试结果。
