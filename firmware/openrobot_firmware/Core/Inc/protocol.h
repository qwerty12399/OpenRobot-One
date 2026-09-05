#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "main.h"

void Protocol_Init(
    UART_HandleTypeDef *huart);

void Protocol_Poll(void);

void Protocol_CheckWatchdog(void);

void Protocol_SendTelemetry(void);

void Protocol_RxCallback(
    UART_HandleTypeDef *huart);

void Protocol_ErrorCallback(
    UART_HandleTypeDef *huart);

#endif