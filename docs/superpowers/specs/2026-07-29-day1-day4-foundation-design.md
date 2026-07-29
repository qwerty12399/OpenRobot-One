# OpenRobot-One Day 1–4 工程基座与仿真设计

日期：2026-07-29

## 1. 目标

在现有空仓库中分阶段建立可复现的 ROS 2 Humble 工程：

1. 先完成并验证 Day 1–2 工程基座。
2. Day 1–2 验证通过后检查 Day 3；由于当前没有机器人描述实现，检查结论为“未完成”，随后补齐 Day 4 所必需的最小 Day 3 模型。
3. 最后实现 Day 4 的 Gazebo Classic 差速仿真和统一 Bringup。

Day 1–2 阶段不包含 URDF、Gazebo 启动、Nav2 配置或 STM32 业务功能。Day 3–4 也不实现激光扫描插件、SLAM、Nav2 导航、串口驱动或 STM32 固件。

## 2. 已确认假设

- 第一形态为两轮差速移动底盘。
- 固定环境为 Ubuntu 22.04、ROS 2 Humble、Gazebo Classic 11、RViz2、C++17、Python 3、colcon、Docker 和 GitHub Actions。
- Windows 主机的 Docker CLI 不在 `PATH`，但 WSL2 Ubuntu 22.04 内可以访问 Docker Desktop 29.5.3；权威构建与测试在 Docker 内执行。
- 7 个 ROS 2 包均先使用最小 `ament_cmake` 骨架；`openrobot_msgs` 在没有实际自定义接口前不创建空消息。
- Day 4 使用 ROS 2 Humble 已提供的 `gazebo_ros_diff_drive`，不开发自定义 Gazebo 插件。
- 不删除当前未跟踪的 Docker 安装包、迁移备份、日志、临时文档渲染或用户脚本，只通过忽略规则避免提交。
- 不自动 commit 或 push。

## 3. 实现方案

采用同一 Dockerfile 驱动本地开发和 CI 的方案，避免开发环境与 CI 漂移。

### 3.1 Day 1–2 工程基座

创建以下 ROS 2 包：

- `openrobot_description`
- `openrobot_bringup`
- `openrobot_gazebo`
- `openrobot_navigation`
- `openrobot_driver`
- `openrobot_msgs`
- `openrobot_tests`

每个包只包含可构建所需的 `package.xml`、`CMakeLists.txt` 和必要测试配置。包职责在 README 与架构文档中说明，但不提前创建后续业务节点。

Docker 开发环境包含：

- ROS 2 Humble
- Gazebo Classic ROS 包
- RViz2
- Xacro 与 URDF 检查工具
- Nav2 与 SLAM Toolbox 的依赖安装，但不提供配置或启动文件
- rosdep、colcon 和 ament lint/test 工具

仓库提供：

- `docker/Dockerfile`
- `docker/compose.yaml`
- `docker/entrypoint.sh`
- `scripts/build_ros.sh`
- 根 README
- `docs/architecture.md`
- GitHub Actions 构建测试工作流
- `.gitignore` 与 `.dockerignore`

CI 构建同一 Dockerfile，并在镜像中执行项目构建脚本。

### 3.2 Day 3 机器人描述

机器人模型使用拆分 Xacro，至少包含：

- 公共属性和颜色
- 惯性宏
- 底盘与 `base_footprint`
- 左右轮和支撑轮
- 雷达外形及固定关节
- Gazebo 专用标签独立文件

初始参数：

- 底盘：`0.26 × 0.20 × 0.08 m`
- 轮半径：`0.0325 m`
- 轮宽：`0.025 m`
- 左右轮中心距：`0.20 m`
- 雷达：底盘前方 `0.08 m`、高度 `0.12 m`

这些值是仿真工程估计值，不代表采购实物测量结果。

Day 3 提供：

- `display.launch.py`
- 最小 RViz 配置
- Xacro 转换和 URDF 结构测试
- `check_urdf`、Xacro 和 TF 检查命令

模型测试验证：

- Xacro 能转换为 URDF。
- URDF 能通过 `check_urdf`。
- 必需的 Link 和左右连续轮关节存在。
- Link/Joint 名称不重复。
- 除无惯性虚拟 `base_footprint` 外，所有实体 Link 的质量大于 0。

### 3.3 Day 4 Gazebo 与统一 Bringup

`openrobot_gazebo` 提供：

- 本地 `empty.world`，不依赖在线模型。
- Gazebo 仿真 Launch。
- 从 `robot_description` 自动生成实体。
- `gazebo_ros_diff_drive` 配置。
- `/joint_states` 发布配置。

差速插件参数从 Xacro 参数传递：

- 左右轮关节名
- 轮距
- 轮径
- 更新频率
- 最大轮扭矩
- 最大轮加速度
- `odom_frame`
- `base_frame`

统一 `openrobot_bringup/launch/bringup.launch.py` 参数：

- `sim`
- `use_sim_time`
- `use_rviz`
- `world`
- `params_file`

`sim=true` 通过 IncludeLaunchDescription 启动完整仿真，不复制子包 Launch 内容。`sim=false` 调用 `openrobot_driver` 中只输出清晰“真机驱动尚未实现”日志的预留 Launch，不创建串口节点或伪硬件逻辑。

## 4. TF 与 Topic 所有权

Day 4 的 TF 树：

```text
odom
└── base_footprint
    └── base_link
        ├── left_wheel_link
        ├── right_wheel_link
        ├── caster_link
        └── laser_link
```

唯一发布者：

- `odom → base_footprint`：仅 `gazebo_ros_diff_drive`。
- `base_footprint → base_link`：仅 `robot_state_publisher`。
- `base_link → left_wheel_link/right_wheel_link/caster_link/laser_link`：仅 `robot_state_publisher`，轮关节角度来自 `/joint_states`。
- Day 4 不发布 `map → odom`。

核心 Topic：

- `/cmd_vel`：Gazebo 差速插件订阅。
- `/odom`：Gazebo 差速插件发布。
- `/joint_states`：Gazebo 关节状态插件发布。
- `/tf`：按上述唯一职责发布。

Day 4 不发布 `/scan`。

## 5. 采购清单关联

- JGA25-370 霍尔编码器电机将影响后续编码器 CPR、减速比、轴安装和轮速标定；当前清单没有足够数据，禁止猜测具体数值。
- 当前 `wheel_radius=0.0325 m` 和 `wheel_separation=0.20 m` 仅为仿真初值，后续必须按实际驱动轮和安装中心距校准。
- 采购清单没有明确驱动轮及支撑轮规格，记录为 Day 8 前硬件待确认项。
- TB6612FNG 与具体 JGA25-370 电机堵转电流的兼容性需要在接线和上电前核验。
- 摄像头不参与 Day 1–4，不引入视觉依赖。
- STM32F407VET6、ST-Link V2、CH340、逻辑分析仪、电源与接线材料只在文档中建立后续任务映射，本阶段不实现固件或串口功能。

## 6. 错误处理边界

- 构建脚本使用 `set -euo pipefail`，缺少 ROS 环境或依赖时输出明确错误并失败。
- entrypoint 只负责加载 ROS 2 Humble 和已存在的工作区 overlay。
- Launch 文件使用 ROS 2 Launch 的条件和包索引解析路径，不使用绝对路径。
- `sim=false` 不静默成功，明确说明真机驱动尚未实现。
- 不添加超出当前阶段的恢复框架、通用封装或虚构硬件行为。

## 7. 可验证成功标准

### 7.1 Day 1–2 门槛

以下步骤必须在 Docker 内实际成功，才允许继续 Day 3–4：

1. Docker 镜像构建成功。
2. `rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y` 成功。
3. `colcon list` 准确列出 7 个包。
4. `colcon build --base-paths ros2_ws/src --event-handlers console_direct+` 成功。
5. `colcon test --base-paths ros2_ws/src --event-handlers console_direct+` 成功。
6. `colcon test-result --verbose` 报告零失败。

若任一步失败，先修复 Day 1–2，不继续 Day 3–4。

### 7.2 Day 3

1. Xacro 转换成功。
2. `check_urdf` 成功。
3. 模型结构测试通过。
4. `colcon build` 与 `colcon test` 保持零失败。
5. RViz/TF 图形界面验证在环境允许时执行；无法执行则明确标记未验证。

### 7.3 Day 4

1. 统一 Bringup 能启动 Gazebo empty world 并生成机器人。
2. 发布 `/cmd_vel` 后机器人能前进、后退、左转和右转。
3. `/odom` 连续发布，消息 Frame 为 `odom`/`base_footprint`。
4. TF 连通且没有重复的 `odom → base_footprint` 发布者。
5. `/joint_states` 发布左右轮关节。
6. `colcon build` 与 `colcon test` 保持零失败。
7. 若 Docker 环境不能提供 GUI，则使用无界面 Gazebo 服务完成运行检查，并把 RViz/键盘人工验收标记为未验证。

## 8. 非目标

- 激光雷达 Gazebo 插件与 `/scan`
- 自定义 office world
- SLAM Toolbox 配置
- AMCL 与 Nav2 配置
- STM32 工程、PWM、编码器或 PID
- ROS 2 串口节点
- micro-ROS
- `ros2_control`
- 真机尺寸、编码器 CPR 或电机电流参数的猜测
