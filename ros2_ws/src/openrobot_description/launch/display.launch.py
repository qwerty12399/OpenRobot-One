"""Display the OpenRobot-One model with robot_state_publisher and RViz."""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro


def generate_launch_description():
    """Build the model display launch description."""
    package_share = Path(
        get_package_share_directory("openrobot_description")
    )
    robot_description = xacro.process_file(
        str(package_share / "urdf" / "openrobot.urdf.xacro")
    ).toxml()

    use_rviz = LaunchConfiguration("use_rviz")
    rviz_config = str(package_share / "rviz" / "openrobot.rviz")

    return LaunchDescription(
        [
            DeclareLaunchArgument("use_rviz", default_value="true"),
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                parameters=[
                    {
                        "robot_description": robot_description,
                        "use_sim_time": False,
                    }
                ],
                output="screen",
            ),
            Node(
                package="joint_state_publisher_gui",
                executable="joint_state_publisher_gui",
                output="screen",
            ),
            Node(
                package="rviz2",
                executable="rviz2",
                arguments=["-d", rviz_config],
                condition=IfCondition(use_rviz),
                output="screen",
            ),
        ]
    )
