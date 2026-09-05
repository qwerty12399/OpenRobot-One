#!/usr/bin/env bash

set -e
set -o pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
WS="${PROJECT_ROOT}/ros2_ws"
LOG_FILE="/tmp/openrobot_bringup.log"

# ROS 2 setup scripts may reference unset variables.
# Disable nounset while sourcing them.
set +u
source /opt/ros/humble/setup.bash
source "${WS}/install/setup.bash"
set -u

echo "======================================"
echo " OpenRobot-One Hardware Acceptance"
echo "======================================"
echo

if [ ! -e /dev/ttyUSB0 ]; then
    echo "[FAIL] /dev/ttyUSB0 does not exist."
    echo "Attach CH340 to WSL with usbipd first."
    exit 1
fi

echo "[PASS] /dev/ttyUSB0 detected"
echo

read -r -p "Confirm motors are safely suspended. Type YES: " ANSWER

if [ "${ANSWER}" != "YES" ]; then
    echo "Cancelled."
    exit 2
fi

echo
echo "[INFO] Starting hardware bringup..."

ros2 launch \
    openrobot_bringup \
    hardware_v1.launch.py \
    >"${LOG_FILE}" 2>&1 &

BRINGUP_PID=$!

cleanup() {
    echo
    echo "[INFO] Stopping hardware bringup..."

    if kill -0 "${BRINGUP_PID}" 2>/dev/null; then
        kill -INT "${BRINGUP_PID}" 2>/dev/null || true
        wait "${BRINGUP_PID}" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

echo "[INFO] Waiting for openrobot_driver..."

READY=0

for _ in $(seq 1 40); do
    if ros2 node list 2>/dev/null | grep -qx "/openrobot_driver"; then
        READY=1
        break
    fi

    sleep 0.25
done

if [ "${READY}" -ne 1 ]; then
    echo "[FAIL] openrobot_driver did not start."
    echo
    echo "===== BRINGUP LOG ====="
    cat "${LOG_FILE}"
    exit 1
fi

echo "[PASS] openrobot_driver running"
echo
echo "[INFO] Starting hardware smoke test..."
echo

python3 \
    "${PROJECT_ROOT}/scripts/hardware_smoke_test.py" \
    --yes
