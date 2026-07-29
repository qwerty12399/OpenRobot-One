cmake_minimum_required(VERSION 3.8)
project(openrobot_tests)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
  ament_add_pytest_test(package_skeleton test/test_package_skeleton.py)
  ament_add_pytest_test(week_one_scripts test/test_week_one_scripts.py)
endif()

ament_package()
