"""Validate the unified Day 4 Bringup contract."""

from pathlib import Path


def test_bringup_declares_modes_and_includes_child_launch_files():
    """Check arguments and composition without duplicated child nodes."""
    source_root = Path(__file__).resolve().parents[2]
    launch_file = (
        source_root
        / "openrobot_bringup"
        / "launch"
        / "bringup.launch.py"
    )
    hardware_launch = (
        source_root
        / "openrobot_driver"
        / "launch"
        / "hardware.launch.py"
    )

    assert launch_file.is_file(), f"missing bringup launch: {launch_file}"
    assert hardware_launch.is_file(), (
        f"missing hardware placeholder launch: {hardware_launch}"
    )

    launch_source = launch_file.read_text(encoding="utf-8")
    compile(launch_source, str(launch_file), "exec")
    for argument in (
        "sim",
        "use_sim_time",
        "use_rviz",
        "world",
        "params_file",
    ):
        assert f'"{argument}"' in launch_source

    assert "IncludeLaunchDescription" in launch_source
    assert "openrobot_gazebo" in launch_source
    assert "openrobot_driver" in launch_source
    assert "Node(" not in launch_source
