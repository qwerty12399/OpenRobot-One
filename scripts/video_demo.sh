#!/usr/bin/env bash

set -e
set -o pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
WS="${PROJECT_ROOT}/ros2_ws"
LOG_FILE="/tmp/openrobot_video_bringup.log"

# ROS 2 setup scripts may access unset variables.
set +u
source /opt/ros/humble/setup.bash
source "${WS}/install/setup.bash"
set -u

clear

echo
echo "============================================================"
echo "                    OpenRobot-One V1"
echo "          ROS2 + STM32 Differential Drive Robot"
echo "============================================================"
echo
echo "  ROS2 Humble / C++17 / STM32F407"
echo "  Encoder Feedback / FF+PI / UART / Hardware Safety"
echo
echo "============================================================"
echo

sleep 2

if [ ! -e /dev/ttyUSB0 ]; then
    echo "[FAIL] CH340 serial device /dev/ttyUSB0 not found."
    exit 1
fi

echo "[PASS] CH340 /dev/ttyUSB0 detected"
echo "[INFO] Hardware platform ready"
echo

read -r -p "Motors suspended and safe? Type YES to start demo: " ANSWER

if [ "${ANSWER}" != "YES" ]; then
    echo "Demo cancelled."
    exit 2
fi

echo
echo "============================================================"
echo " STEP 1/3  Starting ROS2 Hardware Bringup"
echo "============================================================"
echo

ros2 launch \
    openrobot_bringup \
    hardware_v1.launch.py \
    >"${LOG_FILE}" 2>&1 &

BRINGUP_PID=$!

cleanup() {
    if kill -0 "${BRINGUP_PID}" 2>/dev/null; then
        kill -INT "${BRINGUP_PID}" 2>/dev/null || true
        wait "${BRINGUP_PID}" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

echo "[INFO] Waiting for /openrobot_driver ..."

READY=0

for _ in $(seq 1 40); do
    if ros2 node list 2>/dev/null | grep -qx "/openrobot_driver"; then
        READY=1
        break
    fi

    sleep 0.25
done

if [ "${READY}" -ne 1 ]; then
    echo
    echo "[FAIL] openrobot_driver failed to start."
    echo
    echo "Bringup log:"
    cat "${LOG_FILE}"
    exit 1
fi

echo "[PASS] /openrobot_driver running"
echo

sleep 1

echo "============================================================"
echo " STEP 2/3  Automated Real-Hardware Motion Test"
echo "============================================================"
echo
echo "Test sequence:"
echo
echo "  1. Idle safety"
echo "  2. Forward"
echo "  3. Stop"
echo "  4. Backward"
echo "  5. Rotate left"
echo "  6. Rotate right"
echo "  7. ROS /cmd_vel timeout stop"
echo
echo "Starting..."
echo

sleep 2

set +e

python3 \
    "${PROJECT_ROOT}/scripts/hardware_smoke_test.py" \
    --yes

TEST_RESULT=$?

set -e

echo

if [ "${TEST_RESULT}" -eq 0 ]; then

    echo "============================================================"
    echo " STEP 3/3  HARDWARE VERIFICATION COMPLETE"
    echo "============================================================"
    echo
    echo "                 7 / 7 TESTS PASSED"
    echo
    echo "            HARDWARE SMOKE TEST: PASS"
    echo
    echo "------------------------------------------------------------"
    echo " ROS2 Driver                  PASS"
    echo " STM32 Motor Control          PASS"
    echo " Encoder Feedback             PASS"
    echo " Forward / Backward           PASS"
    echo " Differential Rotation        PASS"
    echo " /cmd_vel Timeout Safety      PASS"
    echo " One-click Hardware Bringup   PASS"
    echo "------------------------------------------------------------"
    echo
    echo " OpenRobot-One Hardware Base V1"
    echo
    echo "============================================================"

    # Leave final PASS screen visible for filming.
    sleep 6

else

    echo "============================================================"
    echo "                 HARDWARE TEST FAILED"
    echo "============================================================"
    echo
    echo "Do not use this take for the demo video."
    echo

    sleep 3
    exit "${TEST_RESULT}"
fi

echo
echo "[INFO] Demo finished. Stopping ROS2 hardware bringup..."
