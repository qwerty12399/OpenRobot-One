#!/usr/bin/env python3

import argparse
import statistics
import sys
import time

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node
from sensor_msgs.msg import JointState


class HardwareSmokeTest(Node):
    def __init__(
        self,
        left_joint,
        right_joint,
        cmd_vel_topic,
        joint_states_topic,
    ):
        super().__init__("openrobot_hardware_smoke_test")

        self.left_joint = left_joint
        self.right_joint = right_joint
        self.cmd_vel_topic = cmd_vel_topic

        self.cmd_pub = self.create_publisher(
            Twist,
            cmd_vel_topic,
            10,
        )

        self.joint_sub = self.create_subscription(
            JointState,
            joint_states_topic,
            self.joint_callback,
            10,
        )

        self.left_velocity = None
        self.right_velocity = None
        self.feedback_seq = 0

    def joint_callback(self, msg):
        try:
            left_index = msg.name.index(self.left_joint)
            right_index = msg.name.index(self.right_joint)
        except ValueError:
            return

        if len(msg.velocity) <= max(left_index, right_index):
            return

        self.left_velocity = float(msg.velocity[left_index])
        self.right_velocity = float(msg.velocity[right_index])
        self.feedback_seq += 1

    def send_cmd(self, linear_x, angular_z):
        msg = Twist()
        msg.linear.x = float(linear_x)
        msg.angular.z = float(angular_z)
        self.cmd_pub.publish(msg)

    def collect(
        self,
        duration,
        command=None,
        warmup=0.0,
    ):
        start = time.monotonic()
        end = start + duration

        next_publish = start
        last_seq = self.feedback_seq

        samples = []

        while time.monotonic() < end:
            now = time.monotonic()

            if command is not None and now >= next_publish:
                self.send_cmd(
                    command[0],
                    command[1],
                )
                next_publish = now + 0.10

            rclpy.spin_once(
                self,
                timeout_sec=0.02,
            )

            if (
                self.feedback_seq != last_seq
                and self.left_velocity is not None
                and self.right_velocity is not None
            ):
                last_seq = self.feedback_seq

                if time.monotonic() - start >= warmup:
                    samples.append(
                        (
                            self.left_velocity,
                            self.right_velocity,
                        )
                    )

        return samples

    def wait_for_feedback(self, timeout=5.0):
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            rclpy.spin_once(
                self,
                timeout_sec=0.1,
            )

            if (
                self.left_velocity is not None
                and self.right_velocity is not None
            ):
                return True

        return False


def median_pair(samples):
    if not samples:
        raise RuntimeError("No joint-state samples received")

    left = statistics.median(
        sample[0] for sample in samples
    )

    right = statistics.median(
        sample[1] for sample in samples
    )

    return left, right


def check_expected(
    name,
    samples,
    expected_left,
    expected_right,
    relative_tolerance=0.25,
    zero_tolerance=0.8,
):
    try:
        measured_left, measured_right = median_pair(samples)
    except RuntimeError as exc:
        print(f"[FAIL] {name}: {exc}")
        return False

    def one_ok(measured, expected):
        if abs(expected) < 1e-6:
            return abs(measured) <= zero_tolerance

        tolerance = max(
            abs(expected) * relative_tolerance,
            0.8,
        )

        return abs(measured - expected) <= tolerance

    left_ok = one_ok(
        measured_left,
        expected_left,
    )

    right_ok = one_ok(
        measured_right,
        expected_right,
    )

    ok = left_ok and right_ok

    status = "PASS" if ok else "FAIL"

    print(
        f"[{status}] {name}: "
        f"L={measured_left:+.2f} rad/s "
        f"(expected {expected_left:+.2f}), "
        f"R={measured_right:+.2f} rad/s "
        f"(expected {expected_right:+.2f})"
    )

    return ok


def main():
    parser = argparse.ArgumentParser(
        description="OpenRobot-One hardware smoke test"
    )

    parser.add_argument(
        "--left-joint",
        default="left_wheel_joint",
    )

    parser.add_argument(
        "--right-joint",
        default="right_wheel_joint",
    )

    parser.add_argument(
        "--yes",
        action="store_true",
        help="skip safety confirmation",
    )

    parser.add_argument(
        "--cmd-vel-topic",
        default="/cmd_vel",
    )

    parser.add_argument(
        "--joint-states-topic",
        default="/joint_states",
    )

    args = parser.parse_args()

    if not args.yes:
        print()
        print("======================================")
        print(" OpenRobot-One Hardware Smoke Test")
        print("======================================")
        print()
        print("Before continuing confirm:")
        print("  - Motors/wheels are safely suspended.")
        print("  - STM32 and 12V motor power are on.")
        print("  - /dev/ttyUSB0 is attached to WSL.")
        print("  - hardware bringup is already running.")
        print()

        answer = input(
            "Type YES to start the motor test: "
        ).strip()

        if answer != "YES":
            print("Cancelled.")
            return 2

    rclpy.init()

    node = HardwareSmokeTest(
        args.left_joint,
        args.right_joint,
        args.cmd_vel_topic,
        args.joint_states_topic,
    )

    results = []

    try:
        print("Waiting for /cmd_vel subscriber ...")

        cmd_deadline = time.monotonic() + 5.0

        while time.monotonic() < cmd_deadline:
            if node.count_subscribers(node.cmd_vel_topic) >= 1:
                break

            rclpy.spin_once(
                node,
                timeout_sec=0.1,
            )
        else:
            print(
                "[FAIL] No /cmd_vel subscriber after 5 s. "
                "Is openrobot_driver running?"
            )
            return 1

        print("[PASS] /cmd_vel subscriber detected")

        print("Waiting for /joint_states ...")

        if not node.wait_for_feedback():
            print(
                "[FAIL] No usable /joint_states received."
            )
            return 1

        print("[PASS] ROS2 hardware feedback detected")

        # Ensure a known stopped state.
        stopped = node.collect(
            duration=1.2,
            command=(0.0, 0.0),
            warmup=0.5,
        )

        results.append(
            (
                "idle",
                check_expected(
                    "idle",
                    stopped,
                    0.0,
                    0.0,
                ),
            )
        )

        # 65 mm wheel:
        # 0.34 m/s ~= 10.46 rad/s ~= 100 RPM.
        forward = node.collect(
            duration=3.0,
            command=(0.34, 0.0),
            warmup=1.2,
        )

        results.append(
            (
                "forward",
                check_expected(
                    "forward +0.34 m/s",
                    forward,
                    +10.46,
                    +10.46,
                ),
            )
        )

        stop1 = node.collect(
            duration=1.2,
            command=(0.0, 0.0),
            warmup=0.5,
        )

        results.append(
            (
                "stop_after_forward",
                check_expected(
                    "stop after forward",
                    stop1,
                    0.0,
                    0.0,
                ),
            )
        )

        backward = node.collect(
            duration=3.0,
            command=(-0.34, 0.0),
            warmup=1.2,
        )

        results.append(
            (
                "backward",
                check_expected(
                    "backward -0.34 m/s",
                    backward,
                    -10.46,
                    -10.46,
                ),
            )
        )

        node.collect(
            duration=1.0,
            command=(0.0, 0.0),
            warmup=0.4,
        )

        # wheel_separation = 0.163 m
        # angular.z = +2 rad/s
        # wheel angular velocity ~= +/-5.02 rad/s.
        left_turn = node.collect(
            duration=3.0,
            command=(0.0, +2.0),
            warmup=1.2,
        )

        results.append(
            (
                "left_turn",
                check_expected(
                    "rotate left +2 rad/s",
                    left_turn,
                    -5.02,
                    +5.02,
                ),
            )
        )

        node.collect(
            duration=1.0,
            command=(0.0, 0.0),
            warmup=0.4,
        )

        right_turn = node.collect(
            duration=3.0,
            command=(0.0, -2.0),
            warmup=1.2,
        )

        results.append(
            (
                "right_turn",
                check_expected(
                    "rotate right -2 rad/s",
                    right_turn,
                    +5.02,
                    -5.02,
                ),
            )
        )

        node.collect(
            duration=1.0,
            command=(0.0, 0.0),
            warmup=0.4,
        )

        print(
            "[INFO] Watchdog test: publishing forward "
            "for 2 s, then intentionally stopping /cmd_vel"
        )

        node.collect(
            duration=2.0,
            command=(0.34, 0.0),
            warmup=1.0,
        )

        # Intentionally publish NOTHING.
        watchdog_samples = node.collect(
            duration=1.0,
            command=None,
            warmup=0.55,
        )

        results.append(
            (
                "cmd_timeout",
                check_expected(
                    "ROS /cmd_vel timeout stop",
                    watchdog_samples,
                    0.0,
                    0.0,
                ),
            )
        )

    finally:
        # Always request a clean stop before exiting.
        try:
            node.collect(
                duration=0.8,
                command=(0.0, 0.0),
                warmup=0.0,
            )
        except Exception:
            pass

        node.destroy_node()
        rclpy.shutdown()

    print()
    print("======================================")
    print(" Hardware Smoke Test Summary")
    print("======================================")

    passed = 0

    for name, ok in results:
        print(
            f"{'PASS' if ok else 'FAIL':4}  {name}"
        )
        if ok:
            passed += 1

    print("--------------------------------------")
    print(
        f"Result: {passed}/{len(results)} tests passed"
    )

    if passed == len(results):
        print("HARDWARE SMOKE TEST: PASS")
        return 0

    print("HARDWARE SMOKE TEST: FAIL")
    return 1


if __name__ == "__main__":
    sys.exit(main())
