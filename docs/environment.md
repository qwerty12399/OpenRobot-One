# 环境配置需求

## 1. 固定工程基线

| 类别 | 要求 |
| --- | --- |
| 操作系统 | Ubuntu 22.04；Windows 开发机使用 Windows 11 + WSL2 |
| ROS | ROS 2 Humble Desktop Full |
| 仿真 | Gazebo Classic 11、RViz2 |
| 编译 | GCC/G++、C++17、Python 3、CMake、colcon |
| ROS 构建 | ament_cmake / ament_python、rosdep |
| 容器 | Docker Engine 24+ 或 Docker Desktop，Compose v2 |
| 固件 | STM32CubeIDE 与 STM32CubeF4；不要求 micro-ROS |
| 版本控制 | Git；CI 固定 Ubuntu 22.04 |

## 2. 推荐主机

- 64 位 x86 处理器，4 核以上。
- 16 GB 内存；只做无头构建时 8 GB 可用。
- 20 GB 可用磁盘空间，不把 Docker 安装包、厂商完整 SDK 或构建产物提交进仓库。
- 视觉推理首版可使用 CPU；实时 YOLO 建议 NVIDIA GPU，但不是构建必需条件。
- USB 端口至少 3 个：摄像头、ST-Link、USB-TTL。

## 3. Docker 路径（推荐）

在 WSL2/Ubuntu 中执行：

```bash
docker build -f docker/Dockerfile -t openrobot-one:humble .
docker compose -f docker/compose.yaml run --rm dev
```

容器内：

```bash
./scripts/build_ros.sh
```

脚本会依次运行 `rosdep install`、`colcon build`、`colcon test` 和 `colcon test-result --verbose`。

GUI 使用 WSLg 时通常可直接显示；X11 环境需要正确配置 `DISPLAY`。无 GUI 时仍可完成编译、测试和 Topic/TF 检查。

## 4. 原生 Ubuntu 路径

安装 ROS 2 Humble Desktop Full 后，在仓库根目录执行：

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y
colcon build --base-paths ros2_ws/src --event-handlers console_direct+
source install/setup.bash
colcon test --base-paths ros2_ws/src --event-handlers console_direct+
colcon test-result --verbose
```

## 5. 真机工具

- STM32CubeIDE：打开 `firmware/openrobot_firmware/OpenRobotFirmware.ioc` 并生成工程。
- STM32CubeF4 固件包：版本由 CubeIDE 管理，避免把整套固件包复制进仓库。
- ST-Link V2：只按板卡丝印连接 SWDIO、SWCLK、GND，供电策略见硬件手册。
- CH340G USB-TTL：3.3 V 逻辑，115200 8N1 基线；串口设备必须通过参数配置。
- 万用表、可调限流电源、保险丝和总开关：电机上电的强制前置条件。

## 6. 视觉与 AI 可选依赖

视觉节点实现阶段再按实际代码加入 OpenCV、相机驱动和 YOLO 运行时，不在骨架阶段提前拉取大型模型。模型权重放入本地 `models/`，该目录不提交大文件。

LLM 默认使用本地规则模式。启用云端 API 时，密钥必须来自环境变量或未提交的 `.env`，日志不得输出密钥。语音识别是后续可选能力，文字输入必须一直作为稳定回退路径。

## 7. 环境验收

```bash
lsb_release -ds
ros2 --help
gazebo --version
colcon --help
docker compose version
```

预期系统为 Ubuntu 22.04，ROS 发行版为 Humble，Gazebo 主版本为 11。缺少真机或摄像头不会阻止 ROS 工作区的静态构建测试。
