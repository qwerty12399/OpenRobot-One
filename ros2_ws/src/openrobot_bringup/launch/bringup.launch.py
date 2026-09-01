"""Launch OpenRobot-One in simulation or reserved hardware mode."""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition, LaunchConfigurationEquals
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression


def generate_launch_description():
    """Compose child Launch files without duplicating their node logic."""
    bringup_share = Path(get_package_share_directory("openrobot_bringup"))
    driver_share = Path(get_package_share_directory("openrobot_driver"))
    gazebo_share = Path(get_package_share_directory("openrobot_gazebo"))
    navigation_share = Path(
        get_package_share_directory("openrobot_navigation")
    )

    mode = LaunchConfiguration("mode")
    slam = LaunchConfiguration("slam")
    use_sim_time = LaunchConfiguration("use_sim_time")
    use_rviz = LaunchConfiguration("use_rviz")
    world = LaunchConfiguration("world")
    params_file = LaunchConfiguration("params_file")
    robot_config_file = LaunchConfiguration("robot_config_file")

    simulation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            str(gazebo_share / "launch" / "sim.launch.py")
        ),
        condition=LaunchConfigurationEquals("mode", "sim"),
        launch_arguments={
            "use_sim_time": use_sim_time,
            "use_rviz": use_rviz,
            "world": world,
            "params_file": params_file,
            "robot_config_file": robot_config_file,
        }.items(),
    )
    hardware = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            str(driver_share / "launch" / "hardware.launch.py")
        ),
        condition=LaunchConfigurationEquals("mode", "hardware"),
    )
    mapping = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            str(navigation_share / "launch" / "slam.launch.py")
        ),
        condition=IfCondition(
            PythonExpression(
                [
                    "'",
                    mode,
                    "' == 'sim' and '",
                    slam,
                    "'.lower() == 'true'",
                ]
            )
        ),
        launch_arguments={
            "use_sim_time": use_sim_time,
        }.items(),
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "mode",
                default_value="sim",
                choices=["sim", "hardware"],
            ),
            DeclareLaunchArgument("slam", default_value="false"),
            DeclareLaunchArgument("use_sim_time", default_value="true"),
            DeclareLaunchArgument("use_rviz", default_value="true"),
            DeclareLaunchArgument(
                "world",
                default_value=str(
                    gazebo_share / "worlds" / "office_test.world"
                ),
            ),
            DeclareLaunchArgument(
                "params_file",
                default_value=str(
                    bringup_share / "config" / "bringup.yaml"
                ),
            ),
            DeclareLaunchArgument(
                "robot_config_file",
                default_value=str(
                    bringup_share / "config" / "robot.yaml"
                ),
            ),
            simulation,
            hardware,
            mapping,
        ]
    )
