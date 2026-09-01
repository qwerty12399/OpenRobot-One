"""Safe placeholder for the not-yet-implemented dual-motor bench driver."""

from launch import LaunchDescription
from launch.actions import LogInfo


def generate_launch_description():
    """Report the deliberate hardware-mode boundary."""
    return LaunchDescription(
        [
            LogInfo(
                msg=(
                    "mode=hardware selected, but the BTS7960 dual-motor bench "
                    "driver and H0-H4 safety gates are not implemented yet; "
                    "no motor command, odometry, or TF will be published."
                )
            )
        ]
    )
