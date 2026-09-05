// Copyright 2026 OpenRobot-One contributors

#include "gtest/gtest.h"
#include "openrobot_driver/driver_core.hpp"

TEST(DriverCore, ParsesOnlyCompleteTelemetryFrames)
{
  openrobot_driver::Telemetry telemetry;
  EXPECT_TRUE(openrobot_driver::parse_telemetry("S,100,99,207,-100,-102,-203", telemetry));
  EXPECT_EQ(telemetry.left_measured_rpm, 99);
  EXPECT_EQ(telemetry.right_command, -203);
  EXPECT_FALSE(openrobot_driver::parse_telemetry("S,1,2,3,4,5", telemetry));
  EXPECT_FALSE(openrobot_driver::parse_telemetry("S,1,2,3,4,5,6,7", telemetry));
  EXPECT_FALSE(openrobot_driver::parse_telemetry("S,1,2,3,4,5,6junk", telemetry));
}

TEST(DriverCore, IntegratesStraightBenchEstimate)
{
  openrobot_driver::BenchEstimate estimate;
  openrobot_driver::update_bench_estimate(estimate, 100.0, 100.0, 0.0325, 0.163, 0.1);
  EXPECT_NEAR(estimate.x, 0.0340339, 1e-6);
  EXPECT_NEAR(estimate.y, 0.0, 1e-9);
  EXPECT_NEAR(estimate.yaw, 0.0, 1e-9);
  EXPECT_NEAR(estimate.angular_velocity, 0.0, 1e-9);
}

TEST(DriverCore, IntegratesInPlaceRotationWithCorrectSign)
{
  openrobot_driver::BenchEstimate estimate;
  openrobot_driver::update_bench_estimate(estimate, -100.0, 100.0, 0.0325, 0.163, 0.1);
  EXPECT_NEAR(estimate.x, 0.0, 1e-9);
  EXPECT_NEAR(estimate.y, 0.0, 1e-9);
  EXPECT_GT(estimate.yaw, 0.0);
  EXPECT_GT(estimate.angular_velocity, 0.0);
}

TEST(DriverCore, RejectsLargeIntegrationGapButKeepsCurrentVelocity)
{
  openrobot_driver::BenchEstimate estimate;
  openrobot_driver::update_bench_estimate(estimate, 100.0, 100.0, 0.0325, 0.163, 0.5);
  EXPECT_DOUBLE_EQ(estimate.x, 0.0);
  EXPECT_GT(estimate.linear_velocity, 0.0);
}
