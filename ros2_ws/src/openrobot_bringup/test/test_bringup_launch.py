"""Validate the unified first-week Bringup contract."""

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
        "mode",
        "slam",
        "use_sim_time",
        "use_rviz",
        "world",
        "params_file",
        "robot_config_file",
    ):
        assert f'"{argument}"' in launch_source

    assert "IncludeLaunchDescription" in launch_source
    assert "openrobot_gazebo" in launch_source
    assert "openrobot_driver" in launch_source
    assert "openrobot_navigation" in launch_source
    assert "LaunchConfigurationEquals" in launch_source
    assert "office_test.world" in launch_source
    assert "robot.yaml" in launch_source
    assert "Node(" not in launch_source


def test_robot_yaml_preserves_known_values_and_unknown_encoder_count():
    """Check the unified parameter source without inventing hardware facts."""
    package_dir = Path(__file__).resolve().parents[1]
    robot_config = package_dir / "config" / "robot.yaml"
    source = robot_config.read_text(encoding="utf-8")

    assert "wheel_radius_m: 0.0325" in source
    assert "wheel_separation_m: 0.20" in source
    assert "encoder_counts_per_wheel_rev: 0" in source
    assert "command_timeout_ms: 500" in source
    assert "allow_nonzero_motion: true" in source
    assert "publish_tf: false" in source
    assert "odometry_topic: /bench/odom_estimate" in source


def test_hardware_mode_preserves_bench_topic_and_tf_boundary():
    """Keep hardware feedback distinct from simulation odometry and TF."""
    source_root = Path(__file__).resolve().parents[2]
    hardware_launch = (
        source_root
        / "openrobot_driver"
        / "launch"
        / "hardware.launch.py"
    )
    source = hardware_launch.read_text(encoding="utf-8")

    assert '"bench_odom_topic": "/bench/odom_estimate"' in source
    assert '"cmd_vel_topic": "/cmd_vel"' in source
    assert '"joint_states_topic": "/joint_states"' in source
    assert '"bench_odom_frame": "bench_odom"' in source
    assert '"base_frame": "base_footprint"' in source
    assert "TransformBroadcaster" not in source
    assert "publish_odom_tf" not in source
    assert '"/odom"' not in source
