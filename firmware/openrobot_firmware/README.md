# OpenRobot-One STM32 board baseline

## Current Step 20 firmware (2026-09-05)

The current sources have replaced the H2/H3 applications described below with
the FF+PI motor controller and ASCII telemetry. Those sections are historical
bring-up instructions, not commands supported by this build.

`APP_TEST_MODE=1` currently starts a left +100 RPM test at approximately one
second after boot and stops it at six seconds. In this mode the UART watchdog
is disabled, and automatic targets can overwrite `STOP` on the next control
tick. Do not treat serial `STOP` as a latched emergency stop in automatic mode.
No flashing or motor operation was performed for the build repair.

Open the project in `STM32CubeIDE`, refresh/reopen `OpenRobotFirmware`, then
use **Project > Build Project** with the **Debug** configuration. The project
links `motor_control.c` and `protocol.c` under `Application/User/Core`.
Successful acceptance is compilation of both files, no undefined references,
and generation of `Debug/OpenRobotFirmware.elf`.

USART1 NVIC configuration and its HAL interrupt handler are connected for the
existing receive callbacks. Actual UART reception, motor response, encoder
calibration and fault stopping still require hardware verification. The
nominal encoder CPR and initial PI gains are not measured acceptance results.

This directory contains the minimum STM32CubeIDE configuration that is
confirmed by the LXBF407ZG-P1 V2.0 board files and supplied HAL examples.

## Confirmed configuration

- MCU: STM32F407ZGT6, LQFP144
- External high-speed crystal: 8 MHz
- System clock: 168 MHz
- Debug: SWD on PA13/PA14
- Status LED: PC13, active low, initialized off
- Host UART: USART1 on PA9/PA10, 115200 8N1, transmit and receive

## Current peripheral baseline

The generated H2 baseline configures the two IBT-2/BTS7960 control interfaces,
TIM3 four-channel PWM, TIM2/TIM1 encoder interfaces, and TIM6 100 Hz timing.
Motor PID, the motion command protocol, and the independent 500 ms motion
watchdog are not implemented yet. PWM compare values start at zero and all
four enable outputs start low; it must not be used to energize either motor
until the electrical checks and control firmware are complete.

The frozen pin mapping is PC6/PC7/PB0/PB1 for the four PWM signals, PC0-PC3
for the four enable outputs, PA0/PA1 for the left encoder, and PE9/PE11 for
the right encoder. PB6/PB7 are reserved by the board's 32.768 kHz RTC crystal.
USART1 remains on PA9/PA10.

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

## H3 guarded motor pulse diagnostic

The H3 diagnostic keeps every motor output disabled at boot and permits only a
single 20% PWM pulse after a one-use serial authorization. `TIM6` independently
ends the pulse after 10 control ticks (nominally 100 ms), clears all four PWM
compare registers, and drives all four enable outputs low.

Supported ASCII commands are:

```text
MOTOR-ARM
MOTOR-PULSE LEFT FWD
MOTOR-PULSE LEFT REV
MOTOR-PULSE RIGHT FWD
MOTOR-PULSE RIGHT REV
MOTOR-STOP
MOTOR-STATUS
```

Authorization expires after five seconds and is consumed by the first accepted
pulse. A pulse without authorization returns `MOTOR-DENIED`. Only one motor can
be selected per pulse; this firmware does not support continuous PWM, dual-motor
motion, RPM control, PID, or the final ROS 2 protocol.

`FWD` and `REV` are temporary electrical direction labels because the supplied
IBT-2 documentation contradicts itself about RPWM/LPWM direction. The completion
line reports the selected side and direction plus the encoder start count, end
count, and signed wrap-safe delta so the physical mapping can be frozen from
evidence.

On Windows, run `scripts/test_h2_uart.ps1` with the CH340 port and the local
STM32CubeProgrammer CLI path to repeat the UART and reset/reconnect checks.

Code generation requires the STM32CubeF4 firmware package selected by
STM32CubeIDE. Do not replace the MCU with STM32F407VET6.
