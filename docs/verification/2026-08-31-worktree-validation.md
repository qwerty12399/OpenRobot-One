# 2026-08-31 工作树分类与验证记录

> 历史时间点说明：本文中的TB6612、旧Word手册和当日工作树仅为历史证据。当前硬件方案见 `docs/hardware_facts.md` 与BTS7960双电机设计。

## 结论

- 当前分支为 `main`，基线提交为 `7fbff41`。本次没有暂存、提交或推送。
- Docker 镜像、ROS 依赖、11 个包的构建与 87 项测试已实际通过。
- Gazebo、LaserScan、SLAM、关键 Topic 和四条目标 TF 已实际运行复验通过。
- H0、H1 均为 `BLOCKED`：仓库资料不能证明现场安全装备、接线和关键电气参数已确认，禁止电机通电。
- 原始日志保存在 `log/verification-20260830-2315/`；该目录受 `.gitignore` 的 `log/` 规则保护，不会意外加入提交。`SHA256SUMS.txt` 可用于完整性复核。

## 当前工作树快照

- 暂存区：空。
- tracked：18 项，包含 15 个修改、3 个删除；`git diff --stat` 为 421 行新增、418 行删除。
- untracked：6961 个文件。其中 `开发板内容/` 6927 个、约 743 MiB，是厂商 SDK、例程、资料和构建产物集合；不得整体作为当前源码成果提交。
- `git diff --check`：通过。
- 注意：Git 持续提示部分文本文件未来可能发生 LF 到 CRLF 转换，应避免无意义的全文件换行改动。

原始快照：

- `30-git-status.txt`
- `31-tracked-changes.patch`
- `32-untracked-files.txt`
- `33-git-diff-check.txt`

## 变更分类

### A. 已完成且本轮已验证的成果

| 范围 | 文件 | 结论 |
| --- | --- | --- |
| 仿真组合启动 | `ros2_ws/src/openrobot_bringup/launch/bringup.launch.py`、`scripts/run_sim.sh`、`scripts/run_slam.sh` | `mode=sim/hardware` 互斥、默认联合启动 SLAM、`--no-slam` 入口已构建和运行验证 |
| Gazebo 参数化 | `ros2_ws/src/openrobot_gazebo/launch/sim.launch.py`、`ros2_ws/src/openrobot_bringup/config/robot.yaml` | YAML 到 Xacro 参数注入已构建并在 Gazebo 中运行 |
| 包依赖与静态测试 | `openrobot_bringup/package.xml`、`openrobot_gazebo/package.xml`、相关三个测试文件及 `openrobot_tests` 两个测试文件 | 参与 87 项测试，0 失败 |
| 工程骨架 | `openrobot_ai/`、`openrobot_control/`、`openrobot_task/`、`openrobot_vision/` | 包骨架已完成并参与 11 包构建；不表示业务节点已实现 |
| 仿真与 SLAM 文档 | `README.md`、`docs/architecture.md`、`docs/acceptance.md`、`docs/environment.md`、`docs/protocol.md`、`docs/roadmap.md`、`docs/stage1_tasks.md` | 与当前仿真入口和安全边界一致；其中真机部分仍是计划或约束 |

### B. 已保存但未验证为可用功能

| 范围 | 文件 | 未验证点 |
| --- | --- | --- |
| 真机启动 | `ros2_ws/src/openrobot_driver/launch/hardware.launch.py`、`openrobot_driver/package.xml` | 仍是安全占位，只打印未实现信息；没有串口节点、里程计或 TF |
| STM32 基线 | `firmware/openrobot_firmware/OpenRobotFirmware.ioc`、`firmware/openrobot_firmware/README.md` | 未生成、编译、烧录或板测；PWM、编码器、方向和 STBY 引脚未冻结 |
| 硬件资料 | `docs/hardware_bom.md`、`docs/hardware_facts.md`、`硬件参数图片/` | 仅为照片和资料核对，不代替实测、电气安全或接线验收 |
| Word 手册 | `docs/OpenRobot-One_真机硬件开发保姆级手册.docx` | 未做人工内容审阅和可追踪源文件核对 |
| 执行设计 | `docs/superpowers/specs/2026-08-30-72-hour-job-loop-design.md` | 是工作计划，不是执行结果 |

### C. 必须单独处理或排除

| 路径 | 处理意见 |
| --- | --- |
| `开发板内容/` | 外部厂商资料，6927 个文件、约 743 MiB，含 SDK 副本、压缩包和编译产物；不要整体加入 Git |
| `.workbuddy-ai/` | 工具私有状态，不属于项目成果，应排除 |
| `OpenRobot-One 双轨 MVP 项目执行方案.docx` | tracked 删除，Git 无 rename 证据；提交前必须确认删除意图 |
| `采购清单.docx` | tracked 删除，Git 无 rename 证据；提交前必须确认删除意图 |
| `module1-stage1.ps1` | tracked 删除；该脚本会变更主机环境，提交前必须确认删除意图 |

### D. 接口与残余风险

- 顶层 Launch 参数由 `sim:=true` 改为 `mode:=sim`，属于公开接口变化；旧命令会失效。
- 直接运行 `openrobot_gazebo/sim.launch.py` 时必须显式传入 `robot_config_file`；经 Bringup 启动时已正确传入。
- `run_sim.sh` 默认启动 SLAM；若再单独运行 `run_slam.sh`，可能产生两个 `map -> odom` 发布者。
- 运行日志包含无声卡环境的 ALSA 警告，不影响无界面 Gazebo 验收。
- SLAM Toolbox 报告其内部激光范围阈值被裁剪到传感器能力。实际 `/scan` 为 0.12–8.0 m，建图已运行，但参数告警尚未消除。
- 本次构建使用现有挂载的 `build/install/log`，属于增量构建，不是独立输出目录的 clean build。

## 实际验证结果

| 项目 | 命令或证据 | 结果 |
| --- | --- | --- |
| Docker 镜像 | `docker compose -f docker/compose.yaml build` | 通过，退出 0 |
| ROS 依赖 | `rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y` | 通过，退出 0 |
| 构建 | `colcon build --base-paths ros2_ws/src --event-handlers console_direct+` | 11 包通过，退出 0 |
| 测试 | `colcon test --base-paths ros2_ws/src --event-handlers console_direct+` | 退出 0 |
| 汇总 | `colcon test-result --verbose` | 87 tests，0 errors，0 failures，0 skipped |
| Topic | `scripts/check_topics.sh` | 全部必需项通过，0 warning |
| SLAM | `scripts/check_slam.sh` | 通过，0 warning；`/scan` 平均约 9.99 Hz |
| LaserScan | `/scan --once` | `frame_id=laser_link`，范围 0.12–8.0 m，完整 ranges 数据 |
| 里程计 | `/odom --once` | `odom -> base_footprint` |
| 地图 | `/map --once` | `frame_id=map` |
| TF 边 | 15 秒观测 | `map -> odom`、`odom -> base_footprint`、`base_footprint -> base_link`、`base_link -> laser_link` 均存在 |

TF 所有权交叉证据：

| 变换 | 目标唯一发布者 | 本轮证据 |
| --- | --- | --- |
| `map -> odom` | `slam_toolbox` | 节点列表只有一个 `/slam_toolbox`；`/tf` 端点包含该节点；15 秒内观测 300 条该边 |
| `odom -> base_footprint` | Gazebo `openrobot_diff_drive` | `/tf` 端点包含该节点；Gazebo 原始日志明确声明该变换；15 秒内观测 751 条该边；无真机驱动节点 |
| `base_footprint -> base_link` | `robot_state_publisher` | `/tf_static` 只有一个发布端点，节点为 `robot_state_publisher`；观测到一次 transient-local 静态变换 |
| `base_link -> laser_link` | `robot_state_publisher` | 同上 |

ROS 2 Humble 的 Python 订阅 API不提供本脚本所需的每条消息 publisher GID 回调，因此动态 TF 的“边到 GID”不能直接绑定。本轮采用节点唯一性、Topic 端点、Launch 配置、Gazebo声明和边观测交叉确认；原失败探针保存在 `runtime-attempt1/`。

## H0：首次上电前安全检查表

判定：`BLOCKED`。没有现场照片、仪表读数和接线记录，以下任一项未确认都禁止电机通电。

| 检查项 | 状态 | 最小解锁证据 |
| --- | --- | --- |
| 0–15 V、至少 3 A 的可调限流电源 | UNKNOWN | 铭牌照片、空载 12 V 和限流设置记录 |
| 数字万用表可用 | UNKNOWN | 实物照片及电压/通断档自检记录 |
| VM 回路保险丝和保险丝座 | FAIL / 缺失证据 | 串联接线照片；规格须在电流数据后冻结 |
| 可立即触及的总电源开关或急停 | FAIL / 缺失证据 | 断电位置照片和通断测试 |
| PC/CH340、STM32、TB6612、电源公共地 | UNKNOWN | 完全断电后的低阻连续性读数和接线照片 |
| TB6612 VM 端 1000 µF/25 V 与 0.1 µF 去耦 | UNKNOWN | 实物、极性、安装位置和连通证据 |
| 两个驱动轮稳定架空 | UNKNOWN | 工位照片 |
| 电机回路导线、端子、绝缘和固定合格 | UNKNOWN | 20–22 AWG 或等效线材及无裸铜接线照片 |
| 通电时有人值守且可立即断电 | 未验证 | 试验记录中的操作者、时间和断电负责人 |

## H1：硬件参数确认表

判定：`BLOCKED`。已确认 `STM32F407ZGT6`、电机标签 `JGA25-370 / DC 12V / 62 RPM`、驱动模块 `TB6612FNG`、USART1 `PA9/PA10 / 115200`；这些事实不足以允许通电。

| 参数 | 状态 | 最小确认动作 |
| --- | --- | --- |
| 电机允许供电范围 | UNKNOWN | 厂家或订单规格 |
| 左右电机空载、启动、持续、堵转电流 | UNKNOWN | H0 通过后分别限流测量；堵转优先采用厂家数据 |
| TB6612 电流裕量、散热、温升和实装去耦 | UNKNOWN | 电流数据、数据手册核对和受控温升记录 |
| 编码器供电电压、A/B 高电平、电平转换需求、A/B 相序 | UNKNOWN | 查资料并在确认供电下手转测量 |
| 编码器原始 PPR/CPR、倍频、减速比、输出轴每圈计数 | UNKNOWN | 三次手转标定，离散度目标不超过 1% |
| 电机电源额定电压、持续电流 | UNKNOWN | 铭牌和限流供电记录 |
| 保险丝规格 | UNKNOWN | 基于启动/持续电流和线径选型 |
| 有效轮径、轮宽、轮距 | UNKNOWN | 装配后实测；`robot.yaml` 当前值仅为仿真值 |
| 左右电机与编码器的软件符号 | UNKNOWN | 架空低占空比正反转后记录 |
| PWM、方向、STBY、双编码器最终 GPIO/定时器 | UNKNOWN | CubeMX、原理图和实物三方复核后冻结 |
| 串口设备名、UART 最大载荷、CRC 参数和测试向量 | UNKNOWN / 未实现 | 协议测试和真机驱动实现阶段确认 |

`robot.yaml` 中 `encoder_counts_per_wheel_rev: 0` 是显式无效值；真机软件必须据此禁止非零运动。另有文档冲突：`tools/build_beginner_handbook.py` 仍指导创建 `STM32F407VET6` 工程，而实物与 `.ioc` 基线为 `STM32F407ZGT6`，后续应修正文档生成源。

## 复现命令

```bash
docker compose -f docker/compose.yaml build
docker compose -f docker/compose.yaml run --rm dev
source /opt/ros/humble/setup.bash
rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y
colcon build --base-paths ros2_ws/src --event-handlers console_direct+
source install/setup.bash
colcon test --base-paths ros2_ws/src --event-handlers console_direct+
colcon test-result --verbose
./scripts/run_sim.sh
```

另一个容器终端：

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
./scripts/check_topics.sh
./scripts/check_slam.sh
```
