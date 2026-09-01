"""Validate the first-week helper script contracts."""

from pathlib import Path


REQUIRED_SCRIPTS = {
    "build_ros.sh",
    "check_slam.sh",
    "check_topics.sh",
    "run_sim.sh",
    "run_slam.sh",
}


def test_first_week_scripts_are_portable_and_have_help():
    """Check shell safety, root discovery, help, and forbidden commands."""
    repository_root = Path(__file__).resolve().parents[4]
    scripts_dir = repository_root / "scripts"

    for script_name in REQUIRED_SCRIPTS:
        script_path = scripts_dir / script_name
        assert script_path.is_file(), f"missing script: {script_path}"
        source = script_path.read_text(encoding="utf-8")
        assert "set -euo pipefail" in source
        assert 'BASH_SOURCE[0]' in source
        assert "--help" in source
        assert "sudo " not in source
        assert "/mnt/d/" not in source


def test_runtime_scripts_preserve_launch_ownership():
    """Ensure the one-click path has one simulation and one SLAM owner."""
    repository_root = Path(__file__).resolve().parents[4]
    scripts_dir = repository_root / "scripts"
    run_sim = (scripts_dir / "run_sim.sh").read_text(encoding="utf-8")
    run_slam = (scripts_dir / "run_slam.sh").read_text(encoding="utf-8")

    assert "office_test.world" in run_sim
    assert "openrobot_bringup" in run_sim
    assert "mode:=sim" in run_sim
    assert 'slam:="${use_slam}"' in run_sim
    assert "--no-slam" in run_sim
    assert "openrobot_navigation" in run_slam
    assert "openrobot_bringup" not in run_slam
