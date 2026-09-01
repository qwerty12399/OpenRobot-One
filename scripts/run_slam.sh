#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

if [[ "${1:-}" == "--help" ]]; then
  echo "Usage: ./scripts/run_slam.sh [ROS launch arguments...]"
  echo "Start SLAM Toolbox only for diagnostics."
  echo "Normal first-week startup uses ./scripts/run_sim.sh."
  exit 0
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

exec ros2 launch openrobot_navigation slam.launch.py \
  use_sim_time:=true \
  "$@"
