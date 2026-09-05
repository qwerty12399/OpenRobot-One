// Copyright 2026 OpenRobot-One contributors

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "openrobot_driver/driver_core.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class OpenRobotDriverNode : public rclcpp::Node
{
public:
  OpenRobotDriverNode()
  : Node("openrobot_driver")
  {
    serial_port_ = declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
    baud_rate_ = declare_parameter<int>("baud_rate", 115200);
    wheel_radius_ = declare_parameter<double>("wheel_radius", 0.0325);
    wheel_separation_ = declare_parameter<double>("wheel_separation", 0.163);
    max_wheel_rpm_ = declare_parameter<double>("max_wheel_rpm", 150.0);
    command_rate_hz_ = declare_parameter<double>("command_rate_hz", 10.0);
    cmd_timeout_s_ = declare_parameter<double>("cmd_timeout_s", 0.3);
    cmd_vel_topic_ = declare_parameter<std::string>("cmd_vel_topic", "/cmd_vel");
    joint_states_topic_ =
      declare_parameter<std::string>("joint_states_topic", "/joint_states");
    left_joint_name_ =
      declare_parameter<std::string>("left_joint_name", "left_wheel_joint");
    right_joint_name_ =
      declare_parameter<std::string>("right_joint_name", "right_wheel_joint");
    bench_odom_topic_ =
      declare_parameter<std::string>("bench_odom_topic", "/bench/odom_estimate");
    bench_odom_frame_ =
      declare_parameter<std::string>("bench_odom_frame", "bench_odom");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_footprint");
    validate_parameters();
    open_serial();

    cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
      cmd_vel_topic_, 10,
      std::bind(&OpenRobotDriverNode::cmd_vel_callback, this, std::placeholders::_1));
    joint_state_pub_ =
      create_publisher<sensor_msgs::msg::JointState>(joint_states_topic_, 10);
    bench_odom_pub_ =
      create_publisher<nav_msgs::msg::Odometry>(bench_odom_topic_, 10);
    const auto period = std::chrono::duration<double>(1.0 / command_rate_hz_);
    timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::milliseconds>(period),
      std::bind(&OpenRobotDriverNode::control_timer_callback, this));

    last_cmd_time_ = std::chrono::steady_clock::now();
    RCLCPP_INFO(
      get_logger(),
      "serial=%s baud=%d wheel_radius=%.4f wheel_separation=%.4f bench_odom=%s",
      serial_port_.c_str(), baud_rate_, wheel_radius_, wheel_separation_,
      bench_odom_topic_.c_str());
  }

  ~OpenRobotDriverNode() override
  {
    if (serial_fd_ >= 0) {
      send_motor_command(0, 0);
      tcdrain(serial_fd_);
      close(serial_fd_);
    }
  }

private:
  void validate_parameters() const
  {
    if (wheel_radius_ <= 0.0 || wheel_separation_ <= 0.0 || max_wheel_rpm_ <= 0.0 ||
      command_rate_hz_ <= 0.0 || cmd_timeout_s_ <= 0.0)
    {
      throw std::runtime_error("All numeric driver parameters must be greater than zero");
    }
    if (baud_rate_ != 115200) {
      throw std::runtime_error("Current firmware protocol supports baud_rate=115200 only");
    }
    if (bench_odom_topic_ != "/bench/odom_estimate") {
      throw std::runtime_error("Bench firmware must publish only on /bench/odom_estimate");
    }
    if (cmd_vel_topic_.empty() || joint_states_topic_.empty() ||
      bench_odom_frame_.empty() || base_frame_.empty())
    {
      throw std::runtime_error("Topic and frame names must not be empty");
    }
  }

  void open_serial()
  {
    serial_fd_ = open(serial_port_.c_str(), O_RDWR | O_NOCTTY);
    if (serial_fd_ < 0) {
      throw std::runtime_error(
              "Failed to open " + serial_port_ + ": " + std::strerror(errno));
    }

    termios tty{};
    if (tcgetattr(serial_fd_, &tty) != 0) {
      const std::string error = std::strerror(errno);
      close(serial_fd_);
      serial_fd_ = -1;
      throw std::runtime_error("tcgetattr failed: " + error);
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
    tty.c_cflag |= CS8 | CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP |
      INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
      const std::string error = std::strerror(errno);
      close(serial_fd_);
      serial_fd_ = -1;
      throw std::runtime_error("tcsetattr failed: " + error);
    }
    tcflush(serial_fd_, TCIOFLUSH);
  }

  void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    const double left_linear = msg->linear.x - msg->angular.z * wheel_separation_ / 2.0;
    const double right_linear = msg->linear.x + msg->angular.z * wheel_separation_ / 2.0;
    const double rpm_factor = 60.0 / (2.0 * openrobot_driver::kPi * wheel_radius_);
    target_left_rpm_ = static_cast<int>(std::lround(
        std::clamp(left_linear * rpm_factor, -max_wheel_rpm_, max_wheel_rpm_)));
    target_right_rpm_ = static_cast<int>(std::lround(
        std::clamp(right_linear * rpm_factor, -max_wheel_rpm_, max_wheel_rpm_)));
    last_cmd_time_ = std::chrono::steady_clock::now();
    received_cmd_ = true;
  }

  void control_timer_callback()
  {
    int left = received_cmd_ ? target_left_rpm_ : 0;
    int right = received_cmd_ ? target_right_rpm_ : 0;
    const double age = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - last_cmd_time_).count();
    if (received_cmd_ && age > cmd_timeout_s_) {
      left = 0;
      right = 0;
      target_left_rpm_ = 0;
      target_right_rpm_ = 0;
      if (!timeout_reported_) {
        RCLCPP_WARN(get_logger(), "/cmd_vel timeout; commanding STOP");
        timeout_reported_ = true;
      }
    } else if (received_cmd_) {
      timeout_reported_ = false;
    }
    send_motor_command(left, right);
    read_serial();
  }

  void send_motor_command(int left_rpm, int right_rpm)
  {
    if (serial_fd_ < 0) {
      return;
    }
    const std::string command =
      "V," + std::to_string(left_rpm) + "," + std::to_string(right_rpm) + "\n";
    std::size_t sent = 0;
    while (sent < command.size()) {
      const ssize_t result = write(serial_fd_, command.data() + sent, command.size() - sent);
      if (result > 0) {
        sent += static_cast<std::size_t>(result);
      } else if (result < 0 && errno == EINTR) {
        continue;
      } else {
        RCLCPP_ERROR(get_logger(), "Serial write failed: %s", std::strerror(errno));
        return;
      }
    }
  }

  void read_serial()
  {
    char buffer[256];
    while (serial_fd_ >= 0) {
      const ssize_t count = read(serial_fd_, buffer, sizeof(buffer));
      if (count > 0) {
        rx_buffer_.append(buffer, static_cast<std::size_t>(count));
        if (!rx_synchronized_) {
          const auto newline = rx_buffer_.find('\n');
          if (newline == std::string::npos) {
            continue;
          }
          rx_buffer_.erase(0, newline + 1);
          rx_synchronized_ = true;
        }
        process_rx_lines();
      } else if (count == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else if (errno != EINTR) {
        RCLCPP_ERROR(get_logger(), "Serial read failed: %s", std::strerror(errno));
        break;
      }
    }
  }

  void publish_feedback(const openrobot_driver::Telemetry & telemetry)
  {
    const auto stamp = now();
    const auto steady_now = std::chrono::steady_clock::now();
    const double left_velocity =
      telemetry.left_measured_rpm * 2.0 * openrobot_driver::kPi / 60.0;
    const double right_velocity =
      telemetry.right_measured_rpm * 2.0 * openrobot_driver::kPi / 60.0;
    double dt = 0.0;
    if (have_feedback_timestamp_) {
      dt = std::chrono::duration<double>(steady_now - last_feedback_time_).count();
      if (dt > 0.0 && dt < 0.5) {
        left_wheel_position_rad_ += left_velocity * dt;
        right_wheel_position_rad_ += right_velocity * dt;
      }
    }
    last_feedback_time_ = steady_now;
    have_feedback_timestamp_ = true;

    openrobot_driver::update_bench_estimate(
      bench_estimate_, telemetry.left_measured_rpm, telemetry.right_measured_rpm,
      wheel_radius_, wheel_separation_, dt);

    sensor_msgs::msg::JointState joints;
    joints.header.stamp = stamp;
    joints.name = {left_joint_name_, right_joint_name_};
    joints.position = {left_wheel_position_rad_, right_wheel_position_rad_};
    joints.velocity = {left_velocity, right_velocity};
    joint_state_pub_->publish(joints);

    nav_msgs::msg::Odometry odom;
    odom.header.stamp = stamp;
    odom.header.frame_id = bench_odom_frame_;
    odom.child_frame_id = base_frame_;
    odom.pose.pose.position.x = bench_estimate_.x;
    odom.pose.pose.position.y = bench_estimate_.y;
    odom.pose.pose.orientation.z = std::sin(bench_estimate_.yaw / 2.0);
    odom.pose.pose.orientation.w = std::cos(bench_estimate_.yaw / 2.0);
    odom.twist.twist.linear.x = bench_estimate_.linear_velocity;
    odom.twist.twist.angular.z = bench_estimate_.angular_velocity;
    bench_odom_pub_->publish(odom);
  }

  void process_rx_lines()
  {
    while (true) {
      const auto newline = rx_buffer_.find('\n');
      if (newline == std::string::npos) {
        break;
      }
      std::string line = rx_buffer_.substr(0, newline);
      rx_buffer_.erase(0, newline + 1);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      openrobot_driver::Telemetry telemetry;
      if (openrobot_driver::parse_telemetry(line, telemetry)) {
        publish_feedback(telemetry);
      } else if (line.rfind("S,", 0) == 0) {
        RCLCPP_WARN(get_logger(), "Invalid STM32 telemetry: %s", line.c_str());
      }
    }
    if (rx_buffer_.size() > 1024) {
      rx_buffer_.clear();
      RCLCPP_WARN(get_logger(), "Serial RX buffer cleared");
    }
  }

  std::string serial_port_;
  int baud_rate_{115200};
  double wheel_radius_{0.0325};
  double wheel_separation_{0.163};
  double max_wheel_rpm_{150.0};
  double command_rate_hz_{10.0};
  double cmd_timeout_s_{0.3};
  std::string cmd_vel_topic_;
  std::string joint_states_topic_;
  std::string left_joint_name_;
  std::string right_joint_name_;
  std::string bench_odom_topic_;
  std::string bench_odom_frame_;
  std::string base_frame_;
  int serial_fd_{-1};
  int target_left_rpm_{0};
  int target_right_rpm_{0};
  bool received_cmd_{false};
  bool timeout_reported_{false};
  std::chrono::steady_clock::time_point last_cmd_time_;
  std::string rx_buffer_;
  bool rx_synchronized_{false};
  double left_wheel_position_rad_{0.0};
  double right_wheel_position_rad_{0.0};
  bool have_feedback_timestamp_{false};
  std::chrono::steady_clock::time_point last_feedback_time_;
  openrobot_driver::BenchEstimate bench_estimate_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr bench_odom_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<OpenRobotDriverNode>());
  } catch (const std::exception & error) {
    RCLCPP_FATAL(rclcpp::get_logger("openrobot_driver"), "%s", error.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
