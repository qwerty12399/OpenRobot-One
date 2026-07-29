"""Validate the OpenRobot-One Xacro model."""

from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET


REQUIRED_LINKS = {
    "base_footprint",
    "base_link",
    "left_wheel_link",
    "right_wheel_link",
    "caster_link",
    "front_caster_link",
    "laser_link",
}
REQUIRED_CONTINUOUS_JOINTS = {
    "left_wheel_joint",
    "right_wheel_joint",
}
PHYSICAL_LINKS = REQUIRED_LINKS - {"base_footprint"}


def _render_robot() -> ET.Element:
    xacro_file = (
        Path(__file__).resolve().parents[1]
        / "urdf"
        / "openrobot.urdf.xacro"
    )
    assert xacro_file.is_file(), f"missing robot model: {xacro_file}"
    result = subprocess.run(
        ["xacro", str(xacro_file)],
        check=True,
        capture_output=True,
        text=True,
    )

    with tempfile.NamedTemporaryFile(suffix=".urdf") as urdf_file:
        urdf_file.write(result.stdout.encode())
        urdf_file.flush()
        subprocess.run(
            ["check_urdf", urdf_file.name],
            check=True,
            capture_output=True,
            text=True,
        )

    return ET.fromstring(result.stdout)


def test_xacro_is_valid_and_contains_required_model_structure():
    """Check conversion, URDF validity, names, joints, and physical masses."""
    robot = _render_robot()
    links = robot.findall("link")
    joints = robot.findall("joint")
    link_names = [link.attrib["name"] for link in links]
    joint_names = [joint.attrib["name"] for joint in joints]

    assert REQUIRED_LINKS <= set(link_names)
    assert len(link_names) == len(set(link_names))
    assert len(joint_names) == len(set(joint_names))

    joint_types = {
        joint.attrib["name"]: joint.attrib["type"]
        for joint in joints
    }
    for joint_name in REQUIRED_CONTINUOUS_JOINTS:
        assert joint_types[joint_name] == "continuous"

    links_by_name = {link.attrib["name"]: link for link in links}
    for link_name in PHYSICAL_LINKS:
        mass = links_by_name[link_name].find("inertial/mass")
        assert mass is not None, f"missing inertial mass: {link_name}"
        assert float(mass.attrib["value"]) > 0.0

    caster_positions = []
    for joint in joints:
        child = joint.find("child")
        if child is not None and "caster" in child.attrib["link"]:
            origin = joint.find("origin")
            caster_positions.append(float(origin.attrib["xyz"].split()[0]))

    assert min(caster_positions) < 0.0 < max(caster_positions)
