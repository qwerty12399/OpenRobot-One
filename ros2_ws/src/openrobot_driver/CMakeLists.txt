cmake_minimum_required(VERSION 3.8)
project(openrobot_driver)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
add_executable(
  openrobot_driver_node
  src/openrobot_driver_node.cpp
)

ament_target_dependencies(
  openrobot_driver_node
  rclcpp
  geometry_msgs
  sensor_msgs
  nav_msgs
)

target_include_directories(openrobot_driver_node PRIVATE include)

install(
  TARGETS
    openrobot_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
  PATTERN "*.bak" EXCLUDE
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_driver_core test/test_driver_core.cpp)
  target_include_directories(test_driver_core PRIVATE include)
endif()

ament_package()
