cmake_minimum_required(VERSION 3.8)
project(openrobot_bringup)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
  ament_add_pytest_test(
    bringup_launch
    test/test_bringup_launch.py
  )
endif()

ament_package()
