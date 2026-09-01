#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

usage() {
  echo "Usage: ./scripts/run_sim.sh [--rviz] [--no-slam] [ROS launch arguments...]"
  echo "Start office_test.world and SLAM Toolbox with one Bringup launch."
}

use_rviz=false
use_slam=true
if [[ "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi
while [[ $# -gt 0 ]]; do
  case "$1" in
    --rviz)
      use_rviz=true
      shift
      ;;
    --no-slam)
      use_slam=false
      shift
      ;;
    *)
      break
      ;;
  esac
done

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
  mode:=sim \
  slam:="${use_slam}" \
  use_sim_time:=true \
  use_rviz:="${use_rviz}" \
  world:="${world}" \
  "$@"
