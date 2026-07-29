"""Validate the first-week SLAM Toolbox configuration."""

from pathlib import Path


def test_slam_configuration_uses_the_approved_frames_and_scan():
    """Check the stable mapping parameters without launching ROS."""
    package_dir = Path(__file__).resolve().parents[1]
    params_file = package_dir / "config" / "slam_params.yaml"
    launch_file = package_dir / "launch" / "slam.launch.py"

    assert params_file.is_file(), f"missing SLAM parameters: {params_file}"
    assert launch_file.is_file(), f"missing SLAM launch: {launch_file}"

    params_source = params_file.read_text(encoding="utf-8")
    for required_value in (
        "mode: mapping",
        "map_frame: map",
        "odom_frame: odom",
        "base_frame: base_footprint",
        "scan_topic: /scan",
        "max_laser_range: 8.0",
    ):
        assert required_value in params_source

    launch_source = launch_file.read_text(encoding="utf-8")
    compile(launch_source, str(launch_file), "exec")
    assert "sync_slam_toolbox_node" in launch_source
    assert '"use_sim_time"' in launch_source
    assert '"params_file"' in launch_source
    assert "openrobot_gazebo" not in launch_source
