#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

usage() {
  echo "Usage: ./scripts/run_sim.sh [--rviz] [ROS launch arguments...]"
  echo "Start office_test.world without starting SLAM Toolbox."
}

use_rviz=false
if [[ "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi
if [[ "${1:-}" == "--rviz" ]]; then
  use_rviz=true
  shift
fi

if [[ ! -f /opt/ros/humble/setup.bash ]]; then
  echo "ERROR: ROS 2 Humble was not found." >&2
  echo "Run this script inside the OpenRobot-One development container." >&2
  exit 1
fi

set +u
source /opt/ros/humble/setup.bash
set -u
cd "${repo_root}"
if [[ ! -f install/setup.bash ]]; then
  echo "ERROR: install/setup.bash is missing." >&2
  echo "Run ./scripts/build_ros.sh first." >&2
  exit 1
fi
set +u
source install/setup.bash
set -u

world="${repo_root}/ros2_ws/src/openrobot_gazebo/worlds/office_test.world"
exec ros2 launch openrobot_bringup bringup.launch.py \
  sim:=true \
  use_sim_time:=true \
  use_rviz:="${use_rviz}" \
  world:="${world}" \
  "$@"
