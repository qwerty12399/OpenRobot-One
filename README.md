# OpenRobot-One | ROS 2 × STM32 双电机闭环台架

[![ROS 2 build and test](https://github.com/qwerty12399/OpenRobot-One/actions/workflows/ros2_build.yml/badge.svg?branch=main)](https://github.com/qwerty12399/OpenRobot-One/actions/workflows/ros2_build.yml)
![ROS 2 Humble](https://img.shields.io/badge/ROS_2-Humble-22314E?logo=ros)
![STM32F407](https://img.shields.io/badge/MCU-STM32F407-03234B?logo=stmicroelectronics)

OpenRobot-One 是一个分层验证的差速移动机器人 MVP。仿真轨验证 Gazebo、
LaserScan、TF 与 SLAM；真机轨在无轮、架空双电机台架上验证
`/cmd_vel → ROS 2 C++ 串口驱动 → STM32F407 → 双 BTS7960 → 双编码器电机`
闭环和失效停车。

> 当前边界：真机是架空台架，不代表落地运动、里程计精度、真机 SLAM 或
> Nav2。台架只发布 `/bench/odom_estimate` 数据，不广播底盘 TF。

[成果与证据](docs/project_showcase.md) ·
[架构](docs/architecture.md) ·
[验收](docs/acceptance.md) ·
[协议](docs/protocol.md) ·
[真机演示](docs/assets/demo/README.md)

## 最新成果

| 范围 | 状态 | 当前证据 |
| --- | --- | --- |
| ROS 2 Humble 工程 | 实际通过 | Docker 中 11 个包、106 项测试，0 失败/错误、3 项跳过 |
| Gazebo + LaserScan + SLAM | 实际通过 | `/scan`、`/odom`、目标 TF 与 `/map` 运行记录 |
| STM32F407 与 UART | 实际通过 | 构建/下载/verify、10000/10000 PING/PONG、20/20 重连 |
| 双电机 FF+PI 闭环 | 实际通过 | 架空台架前进、后退和双向差速旋转 |
| ROS 2 真机链路 | 实际通过 | `/cmd_vel`、`/joint_states`、`/bench/odom_estimate` |
| 安全停车 | 实际通过 | ROS 2 端 300 ms 命令超时；STM32 端 500 ms 通信看门狗 |
| 一键硬件回归 | 实际通过 | 2026-09-05 实测 7/7 PASS |

最新真机结果：

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

详细数值和证据边界见
[硬件验收记录](docs/evidence/2026-09-05-hardware-acceptance.md)。

## 系统链路

```text
/cmd_vel
   ↓
openrobot_driver (ROS 2 Humble / C++17)
   ├─ 差速运动学 → 左右轮目标 RPM
   ├─ 300 ms /cmd_vel 超时停车
   └─ UART 115200 8N1
            ↓
       STM32F407
   ├─ 100 Hz FF+PI 速度闭环
   ├─ 20 kHz PWM
   ├─ 双编码器测速
   └─ 500 ms 通信看门狗
            ↓
     双 BTS7960 / 双电机
            ↓
      编码器遥测反馈
   ├─ /joint_states
   └─ /bench/odom_estimate（架空数学估算）
```

TF 所有权保持唯一：仿真模式由 Gazebo 发布 `odom → base_footprint`；真机
架空台架不发布该变换，也不发布替代底盘 TF。

## 快速复现

### 仿真与测试

```bash
docker build -f docker/Dockerfile -t openrobot-one:humble .
docker compose -f docker/compose.yaml run --rm dev
./scripts/build_ros.sh
./scripts/run_sim.sh --rviz
```

另一个容器终端：

```bash
source /workspace/install/setup.bash
./scripts/check_topics.sh
./scripts/check_slam.sh
```

### 真机台架

前提：双电机安全架空、电源与公共地已检查、固件已烧录、CH340 已映射为
`/dev/ttyUSB0`。

```bash
cd /mnt/d/OpenRobot-One
bash scripts/hardware_acceptance.sh
```

演示拍摄入口：

```bash
bash scripts/video_demo.sh
```

两个脚本都会要求明确输入 `YES` 后才发送非零命令，并在退出时请求停车。

## 工程结构

```text
OpenRobot-One/
├── .github/workflows/              # ROS 2 CI
├── docker/                         # Ubuntu 22.04 + ROS 2 Humble
├── docs/
│   ├── evidence/                   # 可追溯实测记录
│   └── verification/               # 分阶段验证报告
├── firmware/openrobot_firmware/    # STM32CubeIDE 工程与控制固件
├── ros2_ws/src/
│   ├── openrobot_driver/           # C++ 串口驱动与台架反馈
│   ├── openrobot_bringup/          # 仿真/真机启动入口
│   ├── openrobot_description/      # Xacro 与机器人模型
│   ├── openrobot_gazebo/           # Gazebo Classic 仿真
│   └── openrobot_navigation/       # SLAM 配置
└── scripts/                        # 构建、验收与演示脚本
```

卖家资料、开发板驱动安装包、原始硬件照片和拍摄原片只保存在本地，不作为
源代码跟踪；仓库中只保留项目复现所需的代码、配置、测试、文档和精选演示。

## 尚未完成

- 轮径、轮距和打滑的落地标定；
- 真实底盘 `/odom` 与 TF 发布；
- 真机 SLAM/Nav2；
- 带序号、CRC、累计编码器计数和故障码的正式二进制协议；
- 精确通信延迟、丢包率和停车时延仪器测量。

## License

Apache License 2.0，详见 [LICENSE](LICENSE)。
