"""Reserved hardware launch for the not-yet-implemented STM32 driver."""

from launch import LaunchDescription
from launch.actions import LogInfo


def generate_launch_description():
    """Report the deliberate hardware-mode boundary."""
    return LaunchDescription(
        [
            LogInfo(
                msg=(
                    "mode=hardware selected, but the OpenRobot-One STM32 "
                    "serial driver is not implemented yet; no motor command "
                    "will be published."
                )
            )
        ]
    )
