#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

if [[ "${1:-}" == "--help" ]]; then
  echo "Usage: ./scripts/check_topics.sh"
  echo "Check first-week ROS topics and TF segments."
  exit 0
fi
if [[ $# -ne 0 ]]; then
  echo "ERROR: unknown argument: $1" >&2
  exit 2
fi

if [[ ! -f /opt/ros/humble/setup.bash ]]; then
  echo "FAIL: ROS 2 Humble was not found." >&2
  exit 1
fi
set +u
source /opt/ros/humble/setup.bash
set -u
cd "${repo_root}"
if [[ ! -f install/setup.bash ]]; then
  echo "FAIL: install/setup.bash is missing; run ./scripts/build_ros.sh." >&2
  exit 1
fi
set +u
source install/setup.bash
set -u

if ! command -v timeout >/dev/null 2>&1; then
  echo "FAIL: timeout command is unavailable." >&2
  exit 1
fi

failures=0
warnings=0

check_topic() {
  local topic="$1"
  local required="$2"
  if ros2 topic list 2>/dev/null | grep -Fxq "${topic}"; then
    echo "PASS: topic ${topic} exists"
  elif [[ "${required}" == "true" ]]; then
    echo "FAIL: topic ${topic} is missing"
    failures=$((failures + 1))
  else
    echo "WARN: topic ${topic} is missing (start SLAM if mapping)"
    warnings=$((warnings + 1))
  fi
}

check_tf() {
  local parent="$1"
  local child="$2"
  local required="$3"
  local output
  output="$(
    timeout 4s ros2 run tf2_ros tf2_echo \
      "${parent}" "${child}" 2>&1 || true
  )"
  if grep -q "At time" <<<"${output}"; then
    echo "PASS: TF ${parent} -> ${child} is available"
  elif [[ "${required}" == "true" ]]; then
    echo "FAIL: TF ${parent} -> ${child} is unavailable"
    failures=$((failures + 1))
  else
    echo "WARN: TF ${parent} -> ${child} is unavailable (start SLAM)"
    warnings=$((warnings + 1))
  fi
}

check_topic /scan true
check_topic /odom true
check_topic /joint_states true
check_topic /map false
check_tf odom base_footprint true
check_tf base_footprint base_link true
check_tf base_link laser_link true
check_tf map odom false

if ((failures > 0)); then
  echo "FAIL: ${failures} required check(s) failed; ${warnings} warning(s)."
  exit 1
fi
echo "PASS: all required checks passed; ${warnings} warning(s)."
