// Copyright 2026 OpenRobot-One contributors

#ifndef OPENROBOT_DRIVER__DRIVER_CORE_HPP_
#define OPENROBOT_DRIVER__DRIVER_CORE_HPP_

#include <array>
#include <charconv>
#include <cmath>
#include <string_view>

namespace openrobot_driver
{
constexpr double kPi = 3.14159265358979323846;

struct Telemetry
{
  int left_target_rpm{};
  int left_measured_rpm{};
  int left_command{};
  int right_target_rpm{};
  int right_measured_rpm{};
  int right_command{};
};

inline bool parse_telemetry(std::string_view line, Telemetry & output)
{
  if (line.size() < 3 || line.substr(0, 2) != "S,") {
    return false;
  }
  std::array<int *, 6> fields{
    &output.left_target_rpm, &output.left_measured_rpm, &output.left_command,
    &output.right_target_rpm, &output.right_measured_rpm, &output.right_command};
  std::size_t begin = 2;
  for (std::size_t index = 0; index < fields.size(); ++index) {
    const bool last = index + 1 == fields.size();
    const std::size_t end = last ? line.size() : line.find(',', begin);
    if (end == std::string_view::npos || end == begin) {
      return false;
    }
    const char * first = line.data() + begin;
    const char * final = line.data() + end;
    const auto result = std::from_chars(first, final, *fields[index]);
    if (result.ec != std::errc{} || result.ptr != final) {
      return false;
    }
    begin = end + 1;
  }
  return true;
}

struct BenchEstimate
{
  double x{};
  double y{};
  double yaw{};
  double linear_velocity{};
  double angular_velocity{};
};

inline void update_bench_estimate(
  BenchEstimate & estimate, double left_rpm, double right_rpm,
  double wheel_radius, double wheel_separation, double dt)
{
  const double left_linear = left_rpm * 2.0 * kPi / 60.0 * wheel_radius;
  const double right_linear = right_rpm * 2.0 * kPi / 60.0 * wheel_radius;
  estimate.linear_velocity = (left_linear + right_linear) / 2.0;
  estimate.angular_velocity = (right_linear - left_linear) / wheel_separation;
  if (dt <= 0.0 || dt >= 0.5) {
    return;
  }
  const double mid_yaw = estimate.yaw + estimate.angular_velocity * dt / 2.0;
  estimate.x += estimate.linear_velocity * std::cos(mid_yaw) * dt;
  estimate.y += estimate.linear_velocity * std::sin(mid_yaw) * dt;
  estimate.yaw += estimate.angular_velocity * dt;
  estimate.yaw = std::atan2(std::sin(estimate.yaw), std::cos(estimate.yaw));
}
}  // namespace openrobot_driver

#endif  // OPENROBOT_DRIVER__DRIVER_CORE_HPP_
