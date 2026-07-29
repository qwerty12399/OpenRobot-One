"""Validate the Gazebo assets completed through Day 5."""

from pathlib import Path
import xml.etree.ElementTree as ET


def test_simulation_assets_define_the_approved_plugins_and_frames():
    """Check local world, Launch contract, and Gazebo plugin ownership."""
    package_dir = Path(__file__).resolve().parents[1]
    source_root = package_dir.parent
    world_file = package_dir / "worlds" / "empty.world"
    office_world_file = package_dir / "worlds" / "office_test.world"
    launch_file = package_dir / "launch" / "sim.launch.py"
    gazebo_xacro = (
        source_root
        / "openrobot_description"
        / "urdf"
        / "gazebo.xacro"
    )

    assert world_file.is_file(), f"missing world: {world_file}"
    assert office_world_file.is_file(), (
        f"missing office world: {office_world_file}"
    )
    assert launch_file.is_file(), f"missing simulation launch: {launch_file}"
    ET.parse(world_file)
    office_world = ET.parse(office_world_file).getroot()

    launch_source = launch_file.read_text(encoding="utf-8")
    compile(launch_source, str(launch_file), "exec")
    for argument in ("world", "use_sim_time", "use_rviz", "params_file"):
        assert f'"{argument}"' in launch_source
    assert "spawn_entity.py" in launch_source
    assert '"gui": use_rviz' in launch_source

    gazebo_source = gazebo_xacro.read_text(encoding="utf-8")
    for required_value in (
        "libgazebo_ros_diff_drive.so",
        "libgazebo_ros_joint_state_publisher.so",
        "libgazebo_ros_ray_sensor.so",
        "left_wheel_joint",
        "right_wheel_joint",
        "odom",
        "base_footprint",
        "publish_odom_tf",
        "laser_link",
        "<samples>360</samples>",
        "<update_rate>10.0</update_rate>",
        "<min>0.12</min>",
        "<max>8.0</max>",
        "~/out:=${scan_topic}",
    ):
        assert required_value in gazebo_source

    models = office_world.findall("./world/model")
    model_names = {model.attrib["name"] for model in models}
    assert {"obstacle_1", "obstacle_2", "obstacle_3"} <= model_names
    assert len(models) >= 10
    for model in models:
        assert model.find(".//collision") is not None
        assert model.find(".//visual") is not None

    office_source = office_world_file.read_text(encoding="utf-8")
    assert "model://" not in office_source
    assert "https://" not in office_source
