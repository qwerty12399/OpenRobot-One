import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, LogInfo
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    driver_launch = os.path.join(
        get_package_share_directory("openrobot_driver"),
        "launch",
        "hardware.launch.py",
    )

    return LaunchDescription(
        [
            LogInfo(
                msg="Starting OpenRobot-One hardware bringup V1"
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(driver_launch)
            ),
        ]
    )
