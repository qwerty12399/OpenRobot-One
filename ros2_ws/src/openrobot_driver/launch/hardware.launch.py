"""Reserved hardware launch for the not-yet-implemented real driver."""

from launch import LaunchDescription
from launch.actions import LogInfo


def generate_launch_description():
    """Report the deliberate Day 4 hardware-mode boundary."""
    return LaunchDescription(
        [
            LogInfo(
                msg=(
                    "sim=false selected, but the real OpenRobot-One serial "
                    "driver is not implemented in Day 1-4."
                )
            )
        ]
    )
