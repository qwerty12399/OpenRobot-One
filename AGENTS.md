# OpenRobot-One 仓库开发约束

本文件适用于仓库根目录及其所有子目录，是后续 Codex 任务的长期约束。除非用户在当前任务中明确提出更高优先级的要求，否则必须遵守本文件。

## 1. 项目基线

固定开发与运行环境：

- Ubuntu 22.04
- ROS 2 Humble
- Gazebo Classic 11
- RViz2
- C++17
- Python 3
- colcon
- ament_cmake / ament_python
- Docker
- GitHub Actions

OpenRobot-One 是 ROS 2 差速移动机器人双轨 MVP：

1. 仿真轨：Gazebo → LaserScan → SLAM Toolbox → AMCL → Nav2。
2. 真机轨：`/cmd_vel` → ROS 2 串口驱动 → STM32F407 → 双电机 PID → 编码器反馈 → `/odom` 与 TF。
3. 仿真和真机共用标准接口：`/cmd_vel`、`/odom`、`/tf`、`/joint_states`、`/scan`。
4. MVP 阶段不使用 micro-ROS 和 `ros2_control`。

## 2. 开发前检查

执行任何功能开发前，必须先检查当前实现与 Git 工作树状态，至少完整检查：

- 仓库结构和相关 README、文档；
- 所有相关 `package.xml`；
- 所有相关 `CMakeLists.txt`、`setup.py` 和 `setup.cfg`；
- Launch 文件；
- URDF/Xacro 文件；
- Dockerfile、Compose、entrypoint、`.dockerignore`；
- GitHub Actions；
- 现有单元测试、lint 测试、Launch/集成测试和验收脚本。

先明确当前任务的假设、范围和可验证成功标准。若需求存在会改变实现方向的歧义，必须列出可选方案及优缺点并立即向用户澄清，禁止猜测式编码。

## 3. 修改原则

- 每次只实现当前任务，不提前实现后续功能。
- 修改前先检查现有实现，不重复创建同名文件或同名节点。
- 不覆盖用户已经完成且能工作的 Day 1–3 内容。
- 不大规模重构无关文件。
- 不删除现有测试。
- 不修改公开接口，除非任务明确要求。
- 不使用绝对路径。
- 不在源码中硬编码机器人尺寸、Topic 名和串口配置。
- 所有可调参数进入 Xacro、YAML 或 ROS 参数。
- 优先使用 ROS 2 Humble 已提供的稳定接口。
- 不引入不必要的新依赖。
- 不使用 `sudo`。
- 不自动提交或推送 Git；只有用户在当前任务中明确要求时才可执行。
- 不执行破坏性命令。
- 不生成无法解释的模板代码。
- 仅修改与当前任务直接相关的文件，遵循现有代码风格，只清理由本次修改产生的冗余。

## 4. 代码规范

- C++ 使用 C++17。
- ROS 2 C++ 使用 `rclcpp`。
- Python Launch 文件兼容 ROS 2 Humble。
- C++ 开启合理编译警告：`-Wall -Wextra -Wpedantic`。
- 所有节点、参数、Topic、Frame 命名保持统一。
- 使用 ament lint。
- 关键算法必须可测试。
- 异常必须提供明确日志。
- 避免使用 `catch (...)` 后静默忽略错误。
- README 中不得宣称尚未完成的功能。
- 用最少代码完成明确需求，不做超前设计、冗余封装或无关优化。

Topic 名必须通过 ROS 参数、Launch 参数或 remapping 配置；机器人尺寸必须进入 Xacro；串口设备、波特率、超时和协议相关可调项必须进入 YAML 或 ROS 参数。

## 5. TF 所有权

目标 TF 树：

```text
map
└── odom
    └── base_footprint
        └── base_link
            ├── left_wheel_link
            ├── right_wheel_link
            └── laser_link
```

发布职责：

- `map → odom`：由 SLAM Toolbox 或 AMCL 发布。
- `odom → base_footprint`：由 Gazebo 差速插件或真机驱动发布，二者不可同时发布。
- `base_footprint → base_link` 以及底盘其他固定或关节关系：由 `robot_state_publisher` 发布。
- 不允许两个节点发布同一条 TF。

任何涉及 TF 的修改都必须先列出每条变换的唯一发布者，并验证仿真模式与真机模式不会同时占用同一 TF。

## 6. 验证与任务交付

每次任务结束必须：

1. 列出所有修改文件。
2. 解释每个修改的作用。
3. 实际执行以下适用命令；若当前环境无法执行，则提供完整命令、说明未执行原因，并将结果标记为“未验证”，不得声称通过：

   ```bash
   source /opt/ros/humble/setup.bash
   rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y
   colcon build --base-paths ros2_ws/src --event-handlers console_direct+
   source install/setup.bash
   colcon test --base-paths ros2_ws/src --event-handlers console_direct+
   colcon test-result --verbose
   ```

4. 给出与当前任务对应的运行命令。
5. 给出可复现的验收命令和预期现象。
6. 报告所有尚未验证的部分、环境限制和残余风险。
7. 明确区分“实际运行并通过”“仅静态检查”“未运行”，不声称未实际运行的测试已经通过。

提交前必须检查 `git diff` 和 `git status`，确认没有修改任务范围外的功能、没有覆盖用户现有改动、没有删除测试，也没有意外暂存无关文件。
