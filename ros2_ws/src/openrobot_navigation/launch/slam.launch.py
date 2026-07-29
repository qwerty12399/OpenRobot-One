"""Launch synchronous SLAM Toolbox mapping without starting simulation."""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """Start the sole map-to-odom publisher used during mapping."""
    package_share = Path(
        get_package_share_directory("openrobot_navigation")
    )
    use_sim_time = LaunchConfiguration("use_sim_time")
    params_file = LaunchConfiguration("params_file")

    return LaunchDescription(
        [
            DeclareLaunchArgument("use_sim_time", default_value="true"),
            DeclareLaunchArgument(
                "params_file",
                default_value=str(
                    package_share / "config" / "slam_params.yaml"
                ),
            ),
            Node(
                package="slam_toolbox",
                executable="sync_slam_toolbox_node",
                name="slam_toolbox",
                parameters=[
                    params_file,
                    {"use_sim_time": use_sim_time},
                ],
                output="screen",
            ),
        ]
    )
