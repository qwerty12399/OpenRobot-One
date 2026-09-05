#include "motor_control.h"
#include "main.h"

#include <math.h>

/* Timer handles are defined in main.c in this CubeMX project. */
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

MotorControl_t g_motor_left;
MotorControl_t g_motor_right;


/* =========================================================
 * Utility
 * ========================================================= */

static float clamp_float(
    float value,
    float min_value,
    float max_value)
{
    if (value > max_value)
    {
        return max_value;
    }

    if (value < min_value)
    {
        return min_value;
    }

    return value;
}


/* =========================================================
 * TIM3 BTS7960 PWM layer
 *
 * Current verified mapping:
 *
 * CH1 = LEFT FWD
 * CH2 = LEFT REV
 * CH3 = RIGHT FWD
 * CH4 = RIGHT REV
 * ========================================================= */

static uint32_t command_to_compare(float command)
{
    float magnitude = fabsf(command);

    if (magnitude > MOTOR_CMD_FULL_SCALE)
    {
        magnitude = MOTOR_CMD_FULL_SCALE;
    }

    /*
     * TIM3 ARR currently = 4199.
     * Period count = ARR + 1 = 4200.
     */
    uint32_t period =
        __HAL_TIM_GET_AUTORELOAD(&htim3) + 1U;

    return (uint32_t)(
        magnitude
        * (float)period
        / MOTOR_CMD_FULL_SCALE
    );
}


static void MotorDriver_Left(float command)
{
    uint32_t compare =
        command_to_compare(command);

    if (command > 0.0f)
    {
        /*
         * LEFT FWD
         *
         * Verified mapping:
         * TIM3 CH1 = FWD
         * TIM3 CH2 = REV
         */

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_2,
            0U);

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_1,
            compare);

        /*
         * Enable left BTS7960.
         */
        HAL_GPIO_WritePin(
            GPIOC,
            LEFT_R_EN_Pin |
            LEFT_L_EN_Pin,
            GPIO_PIN_SET);
    }
    else if (command < 0.0f)
    {
        /*
         * LEFT REV
         */

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_1,
            0U);

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_2,
            compare);

        HAL_GPIO_WritePin(
            GPIOC,
            LEFT_R_EN_Pin |
            LEFT_L_EN_Pin,
            GPIO_PIN_SET);
    }
    else
    {
        /*
         * STOP:
         * PWM first to zero,
         * then disable BTS7960.
         */

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_1,
            0U);

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_2,
            0U);

        HAL_GPIO_WritePin(
            GPIOC,
            LEFT_R_EN_Pin |
            LEFT_L_EN_Pin,
            GPIO_PIN_RESET);
    }
}


static void MotorDriver_Right(float command)
{
    uint32_t compare =
        command_to_compare(command);

    if (command > 0.0f)
    {
        /*
         * RIGHT FWD
         *
         * Verified mapping:
         * TIM3 CH3 = FWD
         * TIM3 CH4 = REV
         */

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_4,
            0U);

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_3,
            compare);

        HAL_GPIO_WritePin(
            GPIOC,
            RIGHT_R_EN_Pin |
            RIGHT_L_EN_Pin,
            GPIO_PIN_SET);
    }
    else if (command < 0.0f)
    {
        /*
         * RIGHT REV
         */

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_3,
            0U);

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_4,
            compare);

        HAL_GPIO_WritePin(
            GPIOC,
            RIGHT_R_EN_Pin |
            RIGHT_L_EN_Pin,
            GPIO_PIN_SET);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_3,
            0U);

        __HAL_TIM_SET_COMPARE(
            &htim3,
            TIM_CHANNEL_4,
            0U);

        HAL_GPIO_WritePin(
            GPIOC,
            RIGHT_R_EN_Pin |
            RIGHT_L_EN_Pin,
            GPIO_PIN_RESET);
    }
}


/* =========================================================
 * Encoder speed estimator
 * ========================================================= */

static void Encoder_Update(
    MotorControl_t *motor,
    TIM_HandleTypeDef *htim)
{
    uint16_t current =
        (uint16_t)__HAL_TIM_GET_COUNTER(htim);

    /*
     * Signed subtraction naturally handles
     * 65535 -> 0 / 0 -> 65535 wraparound
     * for the small per-cycle deltas here.
     */
    int16_t delta =
        (int16_t)(
            current - motor->encoder_last
        );

    motor->encoder_last =
        current;

    /*
     * Sliding 5-sample / 50ms window.
     */
    motor->delta_sum -=
        motor->delta_history[
            motor->delta_index
        ];

    motor->delta_history[
        motor->delta_index
    ] = delta;

    motor->delta_sum += delta;

    motor->delta_index++;

    if (motor->delta_index >=
        SPEED_WINDOW_SAMPLES)
    {
        motor->delta_index = 0U;
    }

    if (motor->delta_count <
        SPEED_WINDOW_SAMPLES)
    {
        motor->delta_count++;
    }

    /*
     * During first few samples use actual window duration.
     */
    float window_time =
        (float)motor->delta_count
        * CONTROL_DT_S;

    if (window_time <= 0.0f)
    {
        motor->measured_rpm = 0.0f;
        return;
    }

    motor->measured_rpm =
        (float)motor->encoder_sign
        *
        ((float)motor->delta_sum * 60.0f)
        /
        (ENCODER_CPR * window_time);
}


/* =========================================================
 * Feed-forward + PI
 * ========================================================= */

static float PI_Update(
    MotorControl_t *motor)
{
    float target =
        motor->target_rpm;

    if (fabsf(target) < 0.5f)
    {
        motor->i_term = 0.0f;
        motor->output = 0.0f;
        motor->previous_target = 0.0f;

        return 0.0f;
    }

    /*
     * Direction change:
     * clear historical integral.
     */
    if ((motor->previous_target > 0.0f &&
         target < 0.0f) ||
        (motor->previous_target < 0.0f &&
         target > 0.0f))
    {
        motor->i_term = 0.0f;
    }

    float error =
        target - motor->measured_rpm;

    /*
     * Feed-forward using seller nominal:
     *
     * 620 RPM -> command 1000
     *
     * 100 RPM -> ~161
     */
    float feedforward =
        target
        / MOTOR_NOLOAD_RPM
        * MOTOR_CMD_FULL_SCALE;

    /*
     * Candidate integral contribution.
     */
    float new_i_term =
        motor->i_term
        +
        motor->ki
        * error
        * CONTROL_DT_S;

    new_i_term =
        clamp_float(
            new_i_term,
            -I_TERM_LIMIT,
            I_TERM_LIMIT
        );

    float p_term =
        motor->kp * error;

    float raw_output =
        feedforward
        + p_term
        + new_i_term;

    float limited_output =
        clamp_float(
            raw_output,
            -MOTOR_CMD_LIMIT,
            MOTOR_CMD_LIMIT
        );

    /*
     * Anti-windup:
     * if saturated and error pushes further
     * into saturation, discard new I update.
     */
    if ((raw_output > MOTOR_CMD_LIMIT &&
         error > 0.0f) ||
        (raw_output < -MOTOR_CMD_LIMIT &&
         error < 0.0f))
    {
        new_i_term =
            motor->i_term;

        raw_output =
            feedforward
            + p_term
            + new_i_term;

        limited_output =
            clamp_float(
                raw_output,
                -MOTOR_CMD_LIMIT,
                MOTOR_CMD_LIMIT
            );
    }

    motor->i_term =
        new_i_term;

    motor->output =
        limited_output;

    motor->previous_target =
        target;

    return limited_output;
}


/* =========================================================
 * Automatic bench tests
 * ========================================================= */

static void TestMode_Update(uint32_t ms)
{
#if APP_TEST_MODE == 1

    /*
     * LEFT +100 RPM
     *
     * 0-1s   STOP
     * 1-6s   +100
     * >6s    STOP
     */

    if (ms < 1000U)
    {
        MotorControl_SetTargets(
            0.0f,
            0.0f);
    }
    else if (ms < 6000U)
    {
        MotorControl_SetTargets(
            100.0f,
            0.0f);
    }
    else
    {
        MotorControl_SetTargets(
            0.0f,
            0.0f);
    }


#elif APP_TEST_MODE == 2

    /*
     * LEFT -100 RPM
     */

    if (ms < 1000U)
    {
        MotorControl_SetTargets(
            0.0f,
            0.0f);
    }
    else if (ms < 6000U)
    {
        MotorControl_SetTargets(
            -100.0f,
            0.0f);
    }
    else
    {
        MotorControl_SetTargets(
            0.0f,
            0.0f);
    }


#elif APP_TEST_MODE == 3

    /*
     * Dual motor automatic verification
     */

    if (ms < 1000U)
    {
        MotorControl_SetTargets(
            0.0f, 0.0f);
    }
    else if (ms < 6000U)
    {
        MotorControl_SetTargets(
            100.0f, 100.0f);
    }
    else if (ms < 7000U)
    {
        MotorControl_SetTargets(
            0.0f, 0.0f);
    }
    else if (ms < 12000U)
    {
        MotorControl_SetTargets(
            -100.0f, -100.0f);
    }
    else if (ms < 13000U)
    {
        MotorControl_SetTargets(
            0.0f, 0.0f);
    }
    else if (ms < 18000U)
    {
        MotorControl_SetTargets(
            -100.0f, 100.0f);
    }
    else
    {
        MotorControl_SetTargets(
            0.0f, 0.0f);
    }

#endif
}


/* =========================================================
 * Public
 * ========================================================= */

void MotorControl_Init(void)
{
    /*
     * Clear all runtime states.
     */
    g_motor_left =
        (MotorControl_t){0};

    g_motor_right =
        (MotorControl_t){0};


    /*
     * LEFT motor PI parameters.
     */
    g_motor_left.kp =
        LEFT_KP;

    g_motor_left.ki =
        LEFT_KI;

    /*
     * Confirmed from previous real test:
     *
     * MOTOR-DONE SIDE=LEFT DIR=FWD
     * START=0 END=65494 DELTA=-42
     *
     * Therefore:
     * logical forward -> encoder raw count decreases
     */
    g_motor_left.encoder_sign =
        LEFT_ENCODER_SIGN;


    /*
     * RIGHT motor PI parameters.
     */
    g_motor_right.kp =
        RIGHT_KP;

    g_motor_right.ki =
        RIGHT_KI;

    /*
     * RIGHT_ENCODER_SIGN will use the result
     * from the previous RIGHT FWD test.
     *
     * For the current LEFT +100 RPM test,
     * this value does not affect the result.
     */
    g_motor_right.encoder_sign =
        RIGHT_ENCODER_SIGN;


    /* =====================================================
     * Start TIM3 PWM
     *
     * Verified mapping:
     *
     * CH1 = LEFT FWD
     * CH2 = LEFT REV
     * CH3 = RIGHT FWD
     * CH4 = RIGHT REV
     * ===================================================== */

    if (HAL_TIM_PWM_Start(
            &htim3,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(
            &htim3,
            TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(
            &htim3,
            TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(
            &htim3,
            TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }


    /* =====================================================
     * Start encoder timers
     *
     * Current verified project mapping:
     *
     * TIM2 = LEFT encoder
     * TIM1 = RIGHT encoder
     * ===================================================== */

    if (HAL_TIM_Encoder_Start(
            &htim2,
            TIM_CHANNEL_ALL) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Encoder_Start(
            &htim1,
            TIM_CHANNEL_ALL) != HAL_OK)
    {
        Error_Handler();
    }


    /* =====================================================
     * Reset encoder counters
     * ===================================================== */

    __HAL_TIM_SET_COUNTER(
        &htim2,
        0U);

    __HAL_TIM_SET_COUNTER(
        &htim1,
        0U);

    g_motor_left.encoder_last =
        0U;

    g_motor_right.encoder_last =
        0U;


    /* =====================================================
     * Startup safety
     *
     * PWM = 0
     * BTS7960 EN = LOW
     * ===================================================== */

    MotorDriver_Left(0.0f);

    MotorDriver_Right(0.0f);
}


void MotorControl_SetTargets(
    float left_rpm,
    float right_rpm)
{
    g_motor_left.target_rpm =
        clamp_float(
            left_rpm,
            -TARGET_RPM_LIMIT,
            TARGET_RPM_LIMIT
        );

    g_motor_right.target_rpm =
        clamp_float(
            right_rpm,
            -TARGET_RPM_LIMIT,
            TARGET_RPM_LIMIT
        );
}


void MotorControl_Stop(void)
{
    g_motor_left.target_rpm = 0.0f;
    g_motor_right.target_rpm = 0.0f;

    g_motor_left.i_term = 0.0f;
    g_motor_right.i_term = 0.0f;

    g_motor_left.output = 0.0f;
    g_motor_right.output = 0.0f;

    g_motor_left.previous_target = 0.0f;
    g_motor_right.previous_target = 0.0f;

    MotorDriver_Left(0.0f);
    MotorDriver_Right(0.0f);
}


void MotorControl_Update10ms(void)
{
    /*
     * Correct current project mapping.
     */
    Encoder_Update(
        &g_motor_left,
        &htim2);

    Encoder_Update(
        &g_motor_right,
        &htim1);


#if APP_TEST_MODE != 0

    TestMode_Update(
        HAL_GetTick());

#endif


    float left_output =
        PI_Update(
            &g_motor_left);

    float right_output =
        PI_Update(
            &g_motor_right);


    MotorDriver_Left(
        left_output);

    MotorDriver_Right(
        right_output);
}
