"""Validate the repository's minimal ROS 2 package skeletons."""

from pathlib import Path
import xml.etree.ElementTree as ET


EXPECTED_PACKAGES = {
    "openrobot_description",
    "openrobot_bringup",
    "openrobot_gazebo",
    "openrobot_navigation",
    "openrobot_driver",
    "openrobot_msgs",
    "openrobot_tests",
}


def test_expected_package_skeletons_exist():
    """Check that all planned packages export the ament_cmake build type."""
    source_root = Path(__file__).resolve().parents[2]

    for package_name in EXPECTED_PACKAGES:
        package_dir = source_root / package_name
        package_xml = package_dir / "package.xml"
        cmake_file = package_dir / "CMakeLists.txt"

        assert package_dir.is_dir(), f"missing package directory: {package_name}"
        assert package_xml.is_file(), f"missing package.xml: {package_name}"
        assert cmake_file.is_file(), f"missing CMakeLists.txt: {package_name}"

        root = ET.parse(package_xml).getroot()
        assert root.findtext("name") == package_name
        assert root.findtext("export/build_type") == "ament_cmake"
