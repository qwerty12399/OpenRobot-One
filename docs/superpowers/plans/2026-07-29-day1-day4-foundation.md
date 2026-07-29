# OpenRobot-One Day 1–4 Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use $superpower-subagents (recommended) or $superpower-executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking via update_plan.

**Goal:** Build and verify the Day 1–2 ROS 2 foundation, then add the minimum tested Day 3 robot model required by the Day 4 Gazebo and unified Bringup flow.

**Architecture:** Seven focused `ament_cmake` packages share one Docker-based ROS 2 Humble environment. Day 1–2 is a hard build/test gate; Day 3 owns robot description and fixed/joint transforms; Day 4 composes the standard Gazebo plugins and Launch files without custom driver code.

**Tech Stack:** Ubuntu 22.04, ROS 2 Humble, Gazebo Classic 11, RViz2, Xacro, Python Launch, ament_cmake, pytest, colcon, Docker, GitHub Actions.

---

## File map

- Root policy and documentation: `.gitignore`, `.dockerignore`, `README.md`, `LICENSE`, `docs/architecture.md`.
- Reproducible environment: `docker/Dockerfile`, `docker/compose.yaml`, `docker/entrypoint.sh`.
- One authoritative build entry: `scripts/build_ros.sh`.
- CI: `.github/workflows/ros2_build.yml`.
- Package skeletons: `ros2_ws/src/openrobot_*/package.xml` and `CMakeLists.txt`.
- Foundation audit: `ros2_ws/src/openrobot_tests/test/test_package_skeleton.py`.
- Model source: `ros2_ws/src/openrobot_description/urdf/*.xacro`.
- Model runtime: `ros2_ws/src/openrobot_description/launch/display.launch.py` and `rviz/openrobot.rviz`.
- Model tests: `ros2_ws/src/openrobot_description/test/test_robot_model.py`.
- Simulation: `ros2_ws/src/openrobot_gazebo/{launch,worlds,config,test}`.
- Unified entry: `ros2_ws/src/openrobot_bringup/{launch,config,test}`.
- Hardware-mode placeholder: `ros2_ws/src/openrobot_driver/launch/hardware.launch.py`.

### Task 1: Add the failing Day 1–2 package audit

**Files:**
- Create: `ros2_ws/src/openrobot_tests/package.xml`
- Create: `ros2_ws/src/openrobot_tests/CMakeLists.txt`
- Create: `ros2_ws/src/openrobot_tests/test/test_package_skeleton.py`

- [ ] **Step 1: Register one pytest audit**

The test declares the exact seven package names and checks that each directory contains `package.xml`, `CMakeLists.txt`, a matching package name, and an `ament_cmake` export.

- [ ] **Step 2: Run the test before creating the other packages**

Run in the ROS container:

```bash
colcon build --base-paths ros2_ws/src --packages-select openrobot_tests
source install/setup.bash
colcon test --base-paths ros2_ws/src --packages-select openrobot_tests
colcon test-result --verbose
```

Expected: failure listing the six missing package directories.

- [ ] **Step 3: Preserve the failing output for the final audit**

Record the command and expected failure reason; do not add duplicate audit scripts.

### Task 2: Implement the seven minimal package skeletons

**Files:**
- Create: `ros2_ws/src/openrobot_description/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_bringup/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_gazebo/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_navigation/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_driver/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_msgs/{package.xml,CMakeLists.txt}`

- [ ] **Step 1: Add only `ament_cmake` package metadata**

Each package uses:

```xml
<buildtool_depend>ament_cmake</buildtool_depend>
<export><build_type>ament_cmake</build_type></export>
```

No node, message, Xacro, Gazebo, Nav2, or STM32 business file is added in this task.

- [ ] **Step 2: Add the minimum CMake export**

Each skeleton initially contains:

```cmake
cmake_minimum_required(VERSION 3.8)
project(PACKAGE_NAME)
ament_package()
```

- [ ] **Step 3: Re-run the package audit**

Use the Task 1 commands. Expected: `openrobot_tests` passes and `colcon list` reports exactly seven `openrobot_*` packages.

### Task 3: Add the reproducible Day 1–2 environment and repository policy

**Files:**
- Modify: `.gitignore`
- Create: `.dockerignore`
- Create: `docker/Dockerfile`
- Create: `docker/compose.yaml`
- Create: `docker/entrypoint.sh`
- Create: `scripts/build_ros.sh`
- Create: `.github/workflows/ros2_build.yml`
- Create: `README.md`
- Create: `docs/architecture.md`
- Create: `LICENSE`

- [ ] **Step 1: Exclude generated and local-only artifacts**

Ignore root/workspace `build`, `install`, and `log` directories plus `.worktrees`, `.superpowers`, Docker installer executables, Docker migration backups, local render/temp directories, and local transcript logs. Mirror these exclusions in `.dockerignore` while keeping source `.docx` files outside the Docker context.

- [ ] **Step 2: Build one ROS 2 Humble image**

Use `osrf/ros:humble-desktop-full` and install only the stable packages needed through Day 4:

```text
ros-humble-gazebo-ros-pkgs
ros-humble-navigation2
ros-humble-nav2-bringup
ros-humble-slam-toolbox
ros-humble-xacro
ros-humble-joint-state-publisher-gui
ros-humble-tf2-tools
ros-humble-teleop-twist-keyboard
python3-colcon-common-extensions
python3-rosdep
```

The entrypoint sources `/opt/ros/humble/setup.bash` and an existing `/workspace/install/setup.bash`, then executes the requested command.

- [ ] **Step 3: Add the authoritative build script**

`scripts/build_ros.sh` uses `set -euo pipefail`, resolves the repository root, sources Humble, runs the required `rosdep`, `colcon build`, `colcon test`, and `colcon test-result --verbose` commands, and contains no `sudo`.

- [ ] **Step 4: Add Docker-parity CI**

The workflow triggers on pushes to `main`/`dev` and pull requests to `main`, builds `docker/Dockerfile`, then runs `./scripts/build_ros.sh` in the image with the checkout mounted at `/workspace`.

- [ ] **Step 5: Document only completed Day 1–2 behavior**

README and architecture documentation describe the dual-track boundary, seven package responsibilities, Docker commands, interface conventions, TF ownership, and procurement-derived calibration risks without claiming Day 3–4 completion.

### Task 4: Verify the Day 1–2 hard gate

**Files:**
- No source changes unless a gate command exposes a defect.

- [ ] **Step 1: Build the image**

```bash
docker build -f docker/Dockerfile -t openrobot-one:humble .
```

- [ ] **Step 2: Run the complete build/test gate**

```bash
docker run --rm -v "$PWD:/workspace" openrobot-one:humble bash -lc \
  './scripts/build_ros.sh'
```

- [ ] **Step 3: Confirm package count**

```bash
docker run --rm -v "$PWD:/workspace" openrobot-one:humble bash -lc \
  'colcon list --base-paths ros2_ws/src'
```

Expected: seven packages, zero build/test failures. Do not begin Task 5 if this gate fails.

### Task 5: Add failing Day 3 model tests

**Files:**
- Modify: `ros2_ws/src/openrobot_description/package.xml`
- Modify: `ros2_ws/src/openrobot_description/CMakeLists.txt`
- Create: `ros2_ws/src/openrobot_description/test/test_robot_model.py`

- [ ] **Step 1: Define model acceptance in pytest**

The test invokes `xacro` and `check_urdf`, parses the generated XML, and asserts:

```python
required_links = {
    "base_footprint", "base_link", "left_wheel_link",
    "right_wheel_link", "caster_link", "laser_link",
}
required_continuous_joints = {"left_wheel_joint", "right_wheel_joint"}
```

It also checks duplicate Link/Joint names and positive inertial masses for all physical links.

- [ ] **Step 2: Verify the test fails for the missing model**

Run:

```bash
colcon build --base-paths ros2_ws/src --packages-select openrobot_description
source install/setup.bash
colcon test --base-paths ros2_ws/src --packages-select openrobot_description
colcon test-result --verbose
```

Expected: failure because `urdf/openrobot.urdf.xacro` is absent.

### Task 6: Implement the minimum parameterized Day 3 model

**Files:**
- Create: `ros2_ws/src/openrobot_description/urdf/openrobot.urdf.xacro`
- Create: `ros2_ws/src/openrobot_description/urdf/common_properties.xacro`
- Create: `ros2_ws/src/openrobot_description/urdf/inertial_macros.xacro`
- Create: `ros2_ws/src/openrobot_description/urdf/base.xacro`
- Create: `ros2_ws/src/openrobot_description/urdf/wheel.xacro`
- Create: `ros2_ws/src/openrobot_description/urdf/sensor.xacro`
- Create: `ros2_ws/src/openrobot_description/urdf/gazebo.xacro`
- Create: `ros2_ws/src/openrobot_description/launch/display.launch.py`
- Create: `ros2_ws/src/openrobot_description/rviz/openrobot.rviz`
- Modify: `ros2_ws/src/openrobot_description/{package.xml,CMakeLists.txt}`

- [ ] **Step 1: Implement focused Xacro macros**

Use the approved values for base size, wheel size/separation, caster, and laser pose. `base_footprint` is virtual and inertial-free; all physical links use explicit positive mass and calculated box/cylinder/sphere inertia.

- [ ] **Step 2: Add the display Launch**

Resolve package paths through `ament_index_python`, process Xacro, and launch `robot_state_publisher`, `joint_state_publisher_gui`, and optional RViz with `use_sim_time=false`.

- [ ] **Step 3: Install model resources**

Install `urdf`, `launch`, and `rviz` into the package share directory and declare only the runtime/test dependencies used.

- [ ] **Step 4: Run the Day 3 test green**

Repeat Task 5 verification. Expected: Xacro conversion, `check_urdf`, structure, uniqueness, and inertial checks pass.

### Task 7: Add failing Day 4 simulation composition tests

**Files:**
- Modify: `ros2_ws/src/openrobot_gazebo/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_gazebo/test/test_simulation_assets.py`
- Modify: `ros2_ws/src/openrobot_bringup/{package.xml,CMakeLists.txt}`
- Create: `ros2_ws/src/openrobot_bringup/test/test_bringup_launch.py`

- [ ] **Step 1: Test the required local assets and plugin configuration**

Assert the empty world, simulation Launch, Gazebo diff-drive plugin, joint-state plugin, frame names, wheel parameters, and no scan plugin are present.

- [ ] **Step 2: Test unified Launch arguments and includes**

Parse/import the Bringup Launch and assert the five approved arguments exist and simulation/hardware paths are included rather than duplicated.

- [ ] **Step 3: Verify both tests fail**

Run package-select builds/tests for `openrobot_gazebo` and `openrobot_bringup`. Expected: missing simulation and Bringup assets.

### Task 8: Implement Day 4 Gazebo and unified Bringup

**Files:**
- Modify: `ros2_ws/src/openrobot_description/urdf/gazebo.xacro`
- Create: `ros2_ws/src/openrobot_gazebo/worlds/empty.world`
- Create: `ros2_ws/src/openrobot_gazebo/config/sim.yaml`
- Create: `ros2_ws/src/openrobot_gazebo/launch/sim.launch.py`
- Create: `ros2_ws/src/openrobot_bringup/config/bringup.yaml`
- Create: `ros2_ws/src/openrobot_bringup/launch/bringup.launch.py`
- Create: `ros2_ws/src/openrobot_driver/launch/hardware.launch.py`
- Modify: corresponding `package.xml` and `CMakeLists.txt` files
- Modify: `README.md`
- Modify: `docs/architecture.md`

- [ ] **Step 1: Configure standard Gazebo plugins**

`gazebo_ros_diff_drive` owns `/odom` and `odom -> base_footprint`; `gazebo_ros_joint_state_publisher` owns `/joint_states`; both wheel TF publication options remain disabled so `robot_state_publisher` remains the sole model TF publisher.

- [ ] **Step 2: Add an offline empty world and complete simulation Launch**

Include Gazebo's standard Launch, start `robot_state_publisher`, spawn from `robot_description`, and conditionally launch RViz. Accept `world`, `use_sim_time`, `use_rviz`, and `params_file`.

- [ ] **Step 3: Add unified Bringup**

Declare `sim`, `use_sim_time`, `use_rviz`, `world`, and `params_file`. Include the simulation Launch when `sim=true`; include the explicit no-hardware placeholder Launch when `sim=false`.

- [ ] **Step 4: Install resources and update accurate documentation**

Document keyboard control, `/cmd_vel`, `/odom`, `/joint_states`, TF validation, wheel-direction diagnosis, estimated dimensions, missing wheel/caster purchase details, and TB6612 stall-current risk.

- [ ] **Step 5: Run the Day 4 tests green**

Repeat Task 7 verification. Expected: both packages pass.

### Task 9: Run final build, headless runtime acceptance, and scope audit

**Files:**
- Modify only files implicated by fresh failures.

- [ ] **Step 1: Run the full required build/test sequence in Docker**

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths ros2_ws/src --ignore-src --rosdistro humble -y
colcon build --base-paths ros2_ws/src --event-handlers console_direct+
source install/setup.bash
colcon test --base-paths ros2_ws/src --event-handlers console_direct+
colcon test-result --verbose
```

- [ ] **Step 2: Run headless Gazebo acceptance**

Launch with `use_rviz:=false`, wait for `/spawn_entity` completion, publish bounded `/cmd_vel`, and verify `/odom`, `/joint_states`, and `odom -> base_footprint` with ROS CLI timeouts. Terminate the Launch process cleanly after evidence is collected.

- [ ] **Step 3: Audit scope and generated files**

Use:

```bash
git diff --check
git diff --stat
git status --short
find . -type d \( -name build -o -name install -o -name log \) -prune -print
```

Confirm no Nav2 configuration, scan plugin, STM32 code, custom serial driver, micro-ROS, or `ros2_control` implementation was added.

- [ ] **Step 4: Integrate the final audit**

Report modified files once, grouped by responsibility; distinguish actual Docker/build/test/runtime evidence from static checks and GUI items not run. Do not commit or push.

## Verification summary

- Day 1–2 cannot be marked complete until the Docker image and full colcon gate pass.
- Day 3 requires both the intentional red failure and green model validation.
- Day 4 requires static Launch/assets tests plus headless Gazebo Topic/TF evidence.
- RViz and keyboard GUI behavior remain explicitly unverified if the environment cannot display them.
- Final status includes residual procurement risks and contains no claims beyond fresh command output.

**Next skill:** `$superpower-executing-plans`
