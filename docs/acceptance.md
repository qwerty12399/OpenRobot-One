# 功能演示步骤与验收标准

## 1. 工程验收

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y
colcon build --base-paths ros2_ws/src --event-handlers console_direct+
source install/setup.bash
colcon test --base-paths ros2_ws/src --event-handlers console_direct+
colcon test-result --verbose
```

预期：所有活动包构建成功，测试无失败，ament lint 不报告由本项目引入的问题。

## 2. 仿真与 SLAM 验收

终端 1：

```bash
./scripts/run_sim.sh
```

终端 2：

```bash
source install/setup.bash
./scripts/check_topics.sh
./scripts/check_slam.sh
```

预期：

- `/scan`、`/odom`、`/joint_states` 持续发布。
- `odom → base_footprint`、`base_footprint → base_link`、`base_link → laser_link` 可查询。
- SLAM 启用时 `/map` 和 `map → odom` 存在。
- 同一条 TF 没有两个发布者。

## 3. STM32 板级验收

前置：不接电机驱动和编码器。

1. 使用 ST-Link 下载由 `.ioc` 生成的工程。
2. PC13 状态灯初始保持关闭。
3. USART1 使用 PA9/PA10、115200 8N1 完成双向收发。
4. 复位或断开串口不会产生 PWM 输出。

当前仓库只提供 `.ioc` 基线；未生成的应用代码不得标记为已通过。

## 4. 电机与编码器验收

前置：轮子架空、限流电源、保险丝、总开关、万用表、人在旁值守。

1. 左右编码器分别手转标定 3 次，每输出轴一圈计数离散度不超过 1%。
2. 每台电机分别完成低占空比正反转，方向与编码器符号一致。
3. 双轮速度闭环在多个目标点无持续振荡，稳态误差不高于 10%。
4. 命令超时、拔串口、ROS 进程退出和 MCU 复位均在 500 ms 级别进入停车。
5. 落地直行与原地转向无失控，速度限制符合参数。

任一安全项失败即停止后续测试。

## 5. 视觉验收

1. 摄像头连续运行 10 分钟，无崩溃或静默卡帧。
2. 目标不存在时不发布伪目标。
3. 红色目标在预定光照和距离范围内的检测率达到测试记录要求。
4. 目标类别、置信度、中心位置和时间戳语义正确。
5. 拔掉摄像头或视频结束时健康状态失败，任务状态机输出零速。

## 6. AI 与任务闭环验收

演示口令：`寻找红色物体`、`寻找红色杯子`、`停止` 和一个未知指令。

预期：

- 合法寻找指令进入搜索、对准、接近和停止状态。
- `停止` 在一个控制周期内发布零速。
- 未知指令、非法 LLM JSON、API 超时、目标丢失和视觉故障均不会继续运动。
- 云端关闭时，本地规则仍能完成稳定演示。
- 五次完整目标闭环至少四次成功；失败必须可由日志定位。

## 7. 交付证据

每次里程碑记录：Git 提交号（由用户自行提交）、配置文件、测试命令及原始结果、接线照片、供电/限流设置、标定表、故障视频和未验证项。截图和视频链接只有实际生成后才能写入 README。
