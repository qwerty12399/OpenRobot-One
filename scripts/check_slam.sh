#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

if [[ "${1:-}" == "--help" ]]; then
  echo "Usage: ./scripts/check_slam.sh"
  echo "Check SLAM Toolbox prerequisites and mapping outputs."
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

pass() {
  echo "PASS: $1"
}

warn() {
  echo "WARN: $1"
  warnings=$((warnings + 1))
}

fail() {
  echo "FAIL: $1"
  failures=$((failures + 1))
}

echo "PASS: ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0 (default)}"

topics="$(ros2 topic list 2>/dev/null || true)"
for topic in /scan /odom; do
  if grep -Fxq "${topic}" <<<"${topics}"; then
    pass "topic ${topic} exists"
  else
    fail "topic ${topic} is missing"
  fi
done

scan_rate="$(
  timeout 6s ros2 topic hz /scan 2>&1 || true
)"
if grep -q "average rate:" <<<"${scan_rate}"; then
  pass "/scan is publishing ($(
    grep "average rate:" <<<"${scan_rate}" | tail -n 1
  ))"
else
  fail "/scan frequency could not be measured"
fi

for edge in \
  "odom base_footprint" \
  "base_footprint base_link" \
  "base_link laser_link"; do
  read -r parent child <<<"${edge}"
  tf_output="$(
    timeout 4s ros2 run tf2_ros tf2_echo \
      "${parent}" "${child}" 2>&1 || true
  )"
  if grep -q "At time" <<<"${tf_output}"; then
    pass "TF ${parent} -> ${child} is available"
  else
    fail "TF ${parent} -> ${child} is unavailable"
  fi
done

nodes="$(ros2 node list 2>/dev/null || true)"
if grep -Eq '(^|/)slam_toolbox$' <<<"${nodes}"; then
  pass "slam_toolbox node is running"
else
  fail "slam_toolbox node is not running"
fi

sim_time="$(ros2 param get /slam_toolbox use_sim_time 2>/dev/null || true)"
if grep -Eqi 'boolean value is: true|^true$' <<<"${sim_time}"; then
  pass "slam_toolbox use_sim_time is enabled"
elif [[ -n "${sim_time}" ]]; then
  fail "slam_toolbox use_sim_time is not enabled"
else
  warn "could not read slam_toolbox use_sim_time"
fi

if grep -Fxq "/map" <<<"${topics}"; then
  pass "topic /map exists"
else
  fail "topic /map is missing"
fi

tf_info="$(ros2 topic info /tf -v 2>/dev/null || true)"
if grep -Eq "/(gazebo|openrobot_diff_drive)" <<<"${tf_info}" \
    && grep -q "/openrobot_serial_driver" <<<"${tf_info}"; then
  fail "Gazebo and hardware driver both publish /tf; stop one mode"
elif [[ -n "${tf_info}" ]]; then
  pass "no simultaneous Gazebo/hardware odom publisher was detected"
else
  warn "could not inspect /tf publishers for ownership conflicts"
fi

if ((failures > 0)); then
  echo "FAIL: ${failures} check(s) failed; ${warnings} warning(s)."
  echo "Check simulation, /scan timestamps, TF ownership, and sim time."
  exit 1
fi
echo "PASS: SLAM prerequisites are ready; ${warnings} warning(s)."
