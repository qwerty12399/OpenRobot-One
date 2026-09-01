# OpenRobot-One STM32 board baseline

This directory contains the minimum STM32CubeIDE configuration that is
confirmed by the LXBF407ZG-P1 V2.0 board files and supplied HAL examples.

## Confirmed configuration

- MCU: STM32F407ZGT6, LQFP144
- External high-speed crystal: 8 MHz
- System clock: 168 MHz
- Debug: SWD on PA13/PA14
- Status LED: PC13, active low, initialized off
- Host UART: USART1 on PA9/PA10, 115200 8N1, transmit and receive

## Deliberately not configured yet

The current H2 firmware does not configure the two IBT-2/BTS7960 modules,
TIM4 PWM outputs, TIM2/TIM3 encoder interfaces, motor PID, or the independent
500 ms motion-command watchdog. It must not be used to energize either motor.

The reviewed pin candidates are PB6/PB7 for the left RPWM/LPWM, PB8/PB9 for
the right RPWM/LPWM, PC0-PC3 for four independently controlled enable inputs,
PA0/PA1 for the left encoder, and PA6/PA7 for the right encoder. USART1 remains
on PA9/PA10. These candidates still require CubeMX generation and no-power
electrical validation before they are frozen.

Each enable input requires an external pull-down so both bridges remain
disabled while the MCU is resetting or its GPIOs are high impedance. Do not
tie R_EN or L_EN permanently to 5 V.

The vendor-wide `F407ZG.ioc` also enables RTC, SDIO, SPI3, and USB CDC. Those
peripherals are board demonstrations and are intentionally omitted from this
robot bring-up baseline.

## First hardware acceptance test

1. Power the board from its USB connector.
2. Connect ST-Link using SWDIO, SWCLK, and GND only.
3. Open `OpenRobotFirmware.ioc` in STM32CubeIDE and generate code.
4. Build and flash without connecting the motor driver or encoders.
5. Confirm that PC13 starts high (LED off) and that USART1 is configured for
   115200 8N1 on PA9/PA10.

The temporary H2 acceptance application sends `H2-READY\r\n` after startup,
responds to `H2-PING\r\n` with `H2-PONG\r\n`, and returns `H2-ERR\r\n` for
unknown or overlong input. This ASCII exchange is only a board-level test; it
is not the final ROS 2-to-STM32 binary protocol.

Incomplete input is silently discarded after 500 ms of UART inactivity.
Parity, frame, noise, and overrun flags reset the parser before subsequent
input is accepted. These protections apply only to the H2 UART parser; they do
not implement a motor command watchdog.

On Windows, run `scripts/test_h2_uart.ps1` with the CH340 port and the local
STM32CubeProgrammer CLI path to repeat the UART and reset/reconnect checks.

Code generation requires the STM32CubeF4 firmware package selected by
STM32CubeIDE. Do not replace the MCU with STM32F407VET6.
