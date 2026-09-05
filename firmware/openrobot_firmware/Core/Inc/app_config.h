#ifndef APP_CONFIG_H
#define APP_CONFIG_H
/* =========================================================
 * OpenRobot-One current verified configuration
 * ========================================================= */

/* Motor */
#define MOTOR_NOLOAD_RPM            620.0f

/* Encoder:
 * 11 PPR × 9.6 gearbox × quadrature x4
 */
#define ENCODER_CPR                 422.4f

/* Existing TIM6 configuration = 100 Hz */
#define CONTROL_DT_S                0.01f

/* 5 × 10 ms = 50 ms RPM estimation window */
#define SPEED_WINDOW_SAMPLES        5U

/* Unified software motor command */
#define MOTOR_CMD_FULL_SCALE        1000.0f

/* Initial bench safety limit = 40% */
#define MOTOR_CMD_LIMIT             400.0f

/* Current PI development range */
#define TARGET_RPM_LIMIT            150.0f

/* =========================================================
 * Encoder directions
 * ========================================================= */

/*
 * Confirmed by:
 * MOTOR-DONE SIDE=LEFT DIR=FWD
 * START=0 END=65494 DELTA=-42
 */
#define LEFT_ENCODER_SIGN           (-1)

/*
 * Set this according to your already completed:
 * MOTOR-DONE SIDE=RIGHT DIR=FWD
 *
 * DELTA > 0 -> +1
 * DELTA < 0 -> -1
 */
#define RIGHT_ENCODER_SIGN          (-1)

/* =========================================================
 * Initial FF + PI
 * ========================================================= */

#define LEFT_KP                     1.5f
#define LEFT_KI                     2.0f

#define RIGHT_KP                    1.5f
#define RIGHT_KI                    2.0f

/* Integral contribution limit */
#define I_TERM_LIMIT                150.0f

/* =========================================================
 * UART / Safety
 * ========================================================= */

#define UART_WATCHDOG_MS            500U
#define TELEMETRY_PERIOD_MS         100U

/*
 * 0 = formal UART/ROS mode
 * 1 = Left +100 RPM
 * 2 = Left -100 RPM
 * 3 = Dual motor automatic test
 */
#define APP_TEST_MODE               0

#endif