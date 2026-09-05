from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="openrobot_driver",
                executable="openrobot_driver_node",
                name="openrobot_driver",
                output="screen",
                parameters=[
                    {
                        "serial_port": "/dev/ttyUSB0",
                        "baud_rate": 115200,
                        "wheel_radius": 0.0325,
                        "wheel_separation": 0.163,
                        "max_wheel_rpm": 150.0,
                        "command_rate_hz": 10.0,
                        "cmd_timeout_s": 0.3,
                        "cmd_vel_topic": "/cmd_vel",
                        "joint_states_topic": "/joint_states",
                        "left_joint_name": "left_wheel_joint",
                        "right_joint_name": "right_wheel_joint",
                        "bench_odom_topic": "/bench/odom_estimate",
                        "bench_odom_frame": "bench_odom",
                        "base_frame": "base_footprint",
                    }
                ],
            )
        ]
    )
