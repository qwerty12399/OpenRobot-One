#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>

/*
 * Must be included before MotorControl_t,
 * because SPEED_WINDOW_SAMPLES is defined here.
 */
#include "app_config.h"


typedef struct
{
    /*
     * Target wheel speed.
     * Unit: RPM
     */
    float target_rpm;

    /*
     * Estimated actual wheel speed.
     * Unit: RPM
     */
    float measured_rpm;

    /*
     * PI parameters.
     */
    float kp;
    float ki;

    /*
     * Integral contribution and final
     * normalized motor command.
     */
    float i_term;
    float output;

    /*
     * Encoder direction correction:
     *
     * +1 = raw encoder direction already matches
     *      software forward direction
     *
     * -1 = reverse raw encoder sign
     */
    int8_t encoder_sign;

    /*
     * Previous raw encoder counter value.
     */
    uint16_t encoder_last;

    /*
     * Encoder deltas from recent control cycles.
     *
     * SPEED_WINDOW_SAMPLES is defined
     * in app_config.h.
     *
     * Current design:
     *
     * TIM6 = 100 Hz
     * control period = 10 ms
     *
     * 5 samples × 10 ms = 50 ms
     * speed estimation window.
     */
    int16_t delta_history[SPEED_WINDOW_SAMPLES];

    /*
     * Sum of current delta window.
     */
    int32_t delta_sum;

    /*
     * Circular-buffer index.
     */
    uint8_t delta_index;

    /*
     * Number of valid samples currently
     * accumulated after startup.
     */
    uint8_t delta_count;

    /*
     * Used to detect target direction reversal
     * and clear the PI integral.
     */
    float previous_target;

} MotorControl_t;


/*
 * Global motor controller instances.
 *
 * Definitions are in motor_control.c.
 */
extern MotorControl_t g_motor_left;
extern MotorControl_t g_motor_right;


/*
 * Initialize PWM, encoders and controller state.
 */
void MotorControl_Init(void);


/*
 * Execute one 100 Hz control iteration.
 */
void MotorControl_Update10ms(void);


/*
 * Set left/right target wheel RPM.
 */
void MotorControl_SetTargets(
    float left_rpm,
    float right_rpm
);


/*
 * Immediately set both targets/output to zero.
 */
void MotorControl_Stop(void);


#endif /* MOTOR_CONTROL_H */