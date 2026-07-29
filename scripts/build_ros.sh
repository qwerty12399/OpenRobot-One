#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

if [[ "${1:-}" == "--help" ]]; then
  echo "Usage: ./scripts/build_ros.sh"
  echo "Install ROS dependencies, build all packages, and run all tests."
  exit 0
fi

if [[ $# -ne 0 ]]; then
  echo "ERROR: unknown argument: $1" >&2
  echo "Run ./scripts/build_ros.sh --help for usage." >&2
  exit 2
fi

if [[ ! -f /opt/ros/humble/setup.bash ]]; then
  echo "ERROR: ROS 2 Humble was not found at /opt/ros/humble." >&2
  echo "Run this script inside the OpenRobot-One development container." >&2
  exit 1
fi

set +u
source /opt/ros/humble/setup.bash
set -u
cd "${repo_root}"

rosdep install \
  --from-paths ros2_ws/src \
  --ignore-src \
  --rosdistro humble \
  -y

colcon build \
  --base-paths ros2_ws/src \
  --event-handlers console_direct+

set +u
source install/setup.bash
set -u

colcon test \
  --base-paths ros2_ws/src \
  --event-handlers console_direct+

colcon test-result --verbose
