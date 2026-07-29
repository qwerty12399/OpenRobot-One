"""Launch the OpenRobot-One model in Gazebo Classic."""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro


def generate_launch_description():
    """Build the complete Day 4 simulation launch description."""
    description_share = Path(
        get_package_share_directory("openrobot_description")
    )
    gazebo_share = Path(get_package_share_directory("openrobot_gazebo"))
    gazebo_ros_share = Path(get_package_share_directory("gazebo_ros"))

    default_world = str(gazebo_share / "worlds" / "empty.world")
    default_params = str(gazebo_share / "config" / "sim.yaml")
    rviz_config = str(
        description_share / "rviz" / "openrobot.rviz"
    )
    robot_description = xacro.process_file(
        str(description_share / "urdf" / "openrobot.urdf.xacro")
    ).toxml()

    world = LaunchConfiguration("world")
    use_sim_time = LaunchConfiguration("use_sim_time")
    use_rviz = LaunchConfiguration("use_rviz")
    params_file = LaunchConfiguration("params_file")

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            str(gazebo_ros_share / "launch" / "gazebo.launch.py")
        ),
        launch_arguments={
            "world": world,
            "gui": use_rviz,
        }.items(),
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument("world", default_value=default_world),
            DeclareLaunchArgument("use_sim_time", default_value="true"),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            DeclareLaunchArgument(
                "params_file",
                default_value=default_params,
            ),
            gazebo,
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                parameters=[
                    params_file,
                    {
                        "robot_description": robot_description,
                        "use_sim_time": use_sim_time,
                    },
                ],
                output="screen",
            ),
            Node(
                package="gazebo_ros",
                executable="spawn_entity.py",
                arguments=[
                    "-entity",
                    "openrobot_one",
                    "-topic",
                    "robot_description",
                    "-z",
                    "0.01",
                ],
                output="screen",
            ),
            Node(
                package="rviz2",
                executable="rviz2",
                arguments=["-d", rviz_config],
                condition=IfCondition(use_rviz),
                parameters=[{"use_sim_time": use_sim_time}],
                output="screen",
            ),
        ]
    )
