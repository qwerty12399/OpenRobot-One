"""Launch the OpenRobot-One model in Gazebo Classic."""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro
import yaml


def _positive(config, section, key):
    """Return one required positive numeric configuration value."""
    value = config[section][key]
    if not isinstance(value, (int, float)) or value <= 0:
        raise ValueError(f"{section}.{key} must be greater than zero")
    return value


def _launch_setup(context):
    """Build simulation actions after resolving the robot configuration."""
    description_share = Path(
        get_package_share_directory("openrobot_description")
    )
    gazebo_ros_share = Path(get_package_share_directory("gazebo_ros"))

    rviz_config = str(
        description_share / "rviz" / "openrobot.rviz"
    )
    robot_config_file = Path(
        LaunchConfiguration("robot_config_file").perform(context)
    )
    with robot_config_file.open(encoding="utf-8") as stream:
        robot_config = yaml.safe_load(stream)

    geometry_keys = (
        "base_length_m",
        "base_width_m",
        "base_height_m",
        "wheel_radius_m",
        "wheel_width_m",
        "wheel_separation_m",
    )
    xacro_mappings = {
        key.removesuffix("_m"): str(_positive(
            robot_config, "geometry", key
        ))
        for key in geometry_keys
    }
    xacro_mappings.update(
        {
            "update_rate": str(_positive(
                robot_config, "simulation", "update_rate_hz"
            )),
            "max_wheel_torque": str(_positive(
                robot_config, "simulation", "max_wheel_torque_nm"
            )),
            "max_wheel_acceleration": str(_positive(
                robot_config,
                "simulation",
                "max_wheel_acceleration_rad_s2",
            )),
            "scan_topic": str(
                robot_config["simulation"]["scan_topic"]
            ),
        }
    )
    robot_description = xacro.process_file(
        str(description_share / "urdf" / "openrobot.urdf.xacro"),
        mappings=xacro_mappings,
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

    return [
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


def generate_launch_description():
    """Build the complete first-week simulation launch description."""
    gazebo_share = Path(get_package_share_directory("openrobot_gazebo"))

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "world",
                default_value=str(
                    gazebo_share / "worlds" / "office_test.world"
                ),
            ),
            DeclareLaunchArgument("use_sim_time", default_value="true"),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            DeclareLaunchArgument(
                "params_file",
                default_value=str(
                    gazebo_share / "config" / "sim.yaml"
                ),
            ),
            DeclareLaunchArgument(
                "robot_config_file",
                description="Unified robot geometry and runtime parameters.",
            ),
            OpaqueFunction(function=_launch_setup),
        ]
    )
