#include "protocol.h"

#include "app_config.h"
#include "motor_control.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#define RX_BUFFER_SIZE 64U


static UART_HandleTypeDef *s_uart;

static uint8_t s_rx_byte;

static char s_rx_buffer[
    RX_BUFFER_SIZE];

static volatile uint16_t s_rx_index;

static volatile bool s_line_ready;

static uint32_t s_last_command_ms;

static uint32_t s_last_telemetry_ms;

static bool s_received_command;


static int round_int(float value)
{
    if (value >= 0.0f)
    {
        return (int)(value + 0.5f);
    }

    return (int)(value - 0.5f);
}


void Protocol_Init(
    UART_HandleTypeDef *huart)
{
    s_uart = huart;

    s_rx_index = 0U;
    s_line_ready = false;

    s_last_command_ms =
        HAL_GetTick();

    s_last_telemetry_ms =
        HAL_GetTick();

    s_received_command = false;

    HAL_UART_Receive_IT(
        s_uart,
        &s_rx_byte,
        1U);
}


void Protocol_RxCallback(
    UART_HandleTypeDef *huart)
{
    if (huart != s_uart)
    {
        return;
    }

    char c =
        (char)s_rx_byte;

    if (c == '\r')
    {
        /* ignore */
    }
    else if (c == '\n')
    {
        if (s_rx_index > 0U)
        {
            s_rx_buffer[
                s_rx_index] = '\0';

            s_line_ready = true;

            s_rx_index = 0U;
        }
    }
    else if (!s_line_ready)
    {
        if (s_rx_index <
            RX_BUFFER_SIZE - 1U)
        {
            s_rx_buffer[
                s_rx_index++] = c;
        }
        else
        {
            s_rx_index = 0U;
        }
    }

    HAL_UART_Receive_IT(
        s_uart,
        &s_rx_byte,
        1U);
}


void Protocol_ErrorCallback(
    UART_HandleTypeDef *huart)
{
    if (huart != s_uart)
    {
        return;
    }

    s_rx_index = 0U;

    HAL_UART_Receive_IT(
        s_uart,
        &s_rx_byte,
        1U);
}


void Protocol_Poll(void)
{
    if (!s_line_ready)
    {
        return;
    }

    char line[
        RX_BUFFER_SIZE];

    __disable_irq();

    strncpy(
        line,
        s_rx_buffer,
        sizeof(line));

    line[
        sizeof(line) - 1U] = '\0';

    s_line_ready = false;

    __enable_irq();


    int left = 0;
    int right = 0;


    /*
     * V,100,100
     */
    if (sscanf(
            line,
            "V,%d,%d",
            &left,
            &right) == 2)
    {
        MotorControl_SetTargets(
            (float)left,
            (float)right);

        s_last_command_ms =
            HAL_GetTick();

        s_received_command =
            true;

        return;
    }


    if (strcmp(
            line,
            "STOP") == 0)
    {
        MotorControl_Stop();

        s_last_command_ms =
            HAL_GetTick();

        s_received_command =
            true;
    }
}


void Protocol_CheckWatchdog(void)
{
#if APP_TEST_MODE == 0

    if (!s_received_command)
    {
        return;
    }

    if ((uint32_t)(
            HAL_GetTick()
            - s_last_command_ms)
        > UART_WATCHDOG_MS)
    {
        MotorControl_Stop();

        s_received_command =
            false;
    }

#endif
}


void Protocol_SendTelemetry(void)
{
    if ((uint32_t)(
            HAL_GetTick()
            - s_last_telemetry_ms)
        < TELEMETRY_PERIOD_MS)
    {
        return;
    }

    s_last_telemetry_ms =
        HAL_GetTick();


    char tx[96];

    int length =
        snprintf(
            tx,
            sizeof(tx),

            "S,%d,%d,%d,%d,%d,%d\r\n",

            round_int(
                g_motor_left.target_rpm),

            round_int(
                g_motor_left.measured_rpm),

            round_int(
                g_motor_left.output),

            round_int(
                g_motor_right.target_rpm),

            round_int(
                g_motor_right.measured_rpm),

            round_int(
                g_motor_right.output)
        );


    if (length > 0)
    {
        HAL_UART_Transmit(
            s_uart,
            (uint8_t *)tx,
            (uint16_t)length,
            20U);
    }
}