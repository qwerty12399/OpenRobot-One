# 2026-09-01 BTS7960双电机方案转向验证

## 结论

- 当前真机目标已改为两块IBT-2/BTS7960、两台JGA25-370和双编码器的架空闭环台架。
- 仿真轨继续保留，并与台架共享 `/cmd_vel`、差速参数和左右轮命令/反馈语义。
- ROS 2 Docker构建与测试实际通过：11 packages，88 tests，0 errors，0 failures，0 skipped。
- 真机H0/H1仍为 `BLOCKED`，本轮没有电机通电、PWM输出或编码器实测。

## 实际执行

```powershell
docker version --format '{{.Server.Os}} {{.Server.Version}}'
docker compose -f docker/compose.yaml run --rm dev bash ./scripts/build_ros.sh
```

环境：Docker Linux engine 29.5.3，镜像 `openrobot-one:humble`。

结果摘要：

```text
Summary: 11 packages finished
Summary: 88 tests, 0 errors, 0 failures, 0 skipped
```

新增的安全回归检查确认：

- `allow_nonzero_motion: false`；
- `publish_tf: false`；
- 台架估算Topic为 `/bench/odom_estimate`；
- hardware Launch明确不发布电机命令、里程计或TF。

## Windows静态测试

明确测试目录运行结果为 `8 passed, 1 failed`。唯一失败是Windows环境缺少 `xacro` 可执行文件；相同Xacro测试已在上述ROS 2 Docker全量测试中通过。

仓库根目录直接运行pytest还会被 `log/latest_*` Windows符号链接权限拦截，因此Windows结果只作为辅助，不替代Docker验收。

## 资产与文档

- 新增三张BTS7960卖家截图和SHA-256索引；
- 删除当前方案不再使用的DRV8871/TB6612图片和旧接线设计；
- 旧H2/UART/ROS/Gazebo报告保留为历史证据并增加时间点说明；
- 两份旧Word手册从Git索引移除；本地文件因外部进程占用暂未物理删除，并由精确 `.gitignore` 规则隔离。

## 未验证与禁止外推

- 两块BTS7960实物芯片、逻辑阈值、连续能力和温升未测；
- 两台电机电流、编码器电平和计数未测；
- 四路PWM、四EN、双编码器、双PID和500ms运动看门狗未实现；
- ROS 2真机串口驱动和 `/bench/odom_estimate` 未实现。

因此不能宣称真机双电机闭环、架空里程计或超时停车已完成。
