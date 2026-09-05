/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : OpenRobot-One motor closed-loop main program
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "motor_control.h"
#include "protocol.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/*
 * TIM6 = 100 Hz.
 * ISR only sets this flag.
 * The real control calculation runs in main().
 */
volatile uint8_t g_control_10ms_flag = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM6_Init(void);

/* USER CODE BEGIN PFP */

static void HardwareMotorSafeOff(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Immediately place both BTS7960 drivers into safe OFF state.
 *
 * This function does not depend on the PI controller state.
 * It is used for startup/error safety.
 */
static void HardwareMotorSafeOff(void)
{
    const uint32_t enable_pins =
        LEFT_R_EN_Pin  |
        LEFT_L_EN_Pin  |
        RIGHT_R_EN_Pin |
        RIGHT_L_EN_Pin;

    /*
     * If TIM3 clock is already enabled,
     * force all four PWM channels to zero.
     */
    if ((RCC->APB1ENR & RCC_APB1ENR_TIM3EN) != 0U)
    {
        TIM3->CCR1 = 0U;
        TIM3->CCR2 = 0U;
        TIM3->CCR3 = 0U;
        TIM3->CCR4 = 0U;
    }

    /*
     * Disable both BTS7960 boards.
     */
    if ((RCC->AHB1ENR & RCC_AHB1ENR_GPIOCEN) != 0U)
    {
        GPIOC->BSRR =
            ((uint32_t)enable_pins << 16U);
    }
}

void Motor_EmergencyStop(void)
{
    HardwareMotorSafeOff();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM6_Init();

    /* USER CODE BEGIN 2 */

    /*
     * Always enter safe state before enabling
     * PWM/encoder/control peripherals.
     */
    HardwareMotorSafeOff();

    /*
     * MotorControl_Init() starts:
     *
     * TIM2 -> LEFT encoder
     * TIM1 -> RIGHT encoder
     * TIM3 -> PWM CH1~CH4
     */
    MotorControl_Init();

    /*
     * Start USART1 interrupt reception.
     */
    Protocol_Init(&huart1);

    /*
     * Start existing TIM6 100 Hz control scheduler.
     *
     * Do this exactly ONCE.
     */
    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /*
         * Parse complete UART commands.
         */
        Protocol_Poll();

        /*
         * Formal mode:
         * 500 ms without a valid speed command -> STOP.
         */
        Protocol_CheckWatchdog();

        /*
         * Run control loop once for each TIM6 10 ms tick.
         */
        if (g_control_10ms_flag != 0U)
        {
            g_control_10ms_flag = 0U;

            MotorControl_Update10ms();
        }

        /*
         * Send telemetry every 100 ms.
         */
        Protocol_SendTelemetry();
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSE;

    RCC_OscInitStruct.HSEState =
        RCC_HSE_ON;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP =
        RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(
            &RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV4;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{
    TIM_Encoder_InitTypeDef sConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode =
        TIM_COUNTERMODE_UP;
    htim1.Init.Period = 65535;
    htim1.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode =
        TIM_ENCODERMODE_TI12;

    sConfig.IC1Polarity =
        TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection =
        TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler =
        TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 4;

    sConfig.IC2Polarity =
        TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection =
        TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler =
        TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 4;

    if (HAL_TIM_Encoder_Init(
            &htim1,
            &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_RESET;

    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim1,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    TIM_Encoder_InitTypeDef sConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode =
        TIM_COUNTERMODE_UP;
    htim2.Init.Period = 65535;
    htim2.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode =
        TIM_ENCODERMODE_TI12;

    sConfig.IC1Polarity =
        TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection =
        TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler =
        TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 4;

    sConfig.IC2Polarity =
        TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection =
        TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler =
        TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 4;

    if (HAL_TIM_Encoder_Init(
            &htim2,
            &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_RESET;

    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim2,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.CounterMode =
        TIM_COUNTERMODE_UP;
    htim3.Init.Period = 4199;
    htim3.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(
            &htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_RESET;

    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim3,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode =
        TIM_OCMODE_PWM1;

    sConfigOC.Pulse = 0;

    sConfigOC.OCPolarity =
        TIM_OCPOLARITY_HIGH;

    sConfigOC.OCFastMode =
        TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3,
            &sConfigOC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3,
            &sConfigOC,
            TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3,
            &sConfigOC,
            TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3,
            &sConfigOC,
            TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 8399;
    htim6.Init.CounterMode =
        TIM_COUNTERMODE_UP;
    htim6.Init.Period = 99;
    htim6.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(
            &htim6) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_RESET;

    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim6,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;

    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength =
        UART_WORDLENGTH_8B;
    huart1.Init.StopBits =
        UART_STOPBITS_1;
    huart1.Init.Parity =
        UART_PARITY_NONE;
    huart1.Init.Mode =
        UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;
    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;

    if (HAL_UART_Init(
            &huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    HAL_GPIO_WritePin(
        STATUS_LED_GPIO_Port,
        STATUS_LED_Pin,
        GPIO_PIN_SET);

    /*
     * BTS7960 enable pins default LOW.
     * Important startup safety.
     */
    HAL_GPIO_WritePin(
        GPIOC,
        LEFT_R_EN_Pin |
        LEFT_L_EN_Pin |
        RIGHT_R_EN_Pin |
        RIGHT_L_EN_Pin,
        GPIO_PIN_RESET);

    GPIO_InitStruct.Pin =
        STATUS_LED_Pin |
        LEFT_R_EN_Pin |
        LEFT_L_EN_Pin |
        RIGHT_R_EN_Pin |
        RIGHT_L_EN_Pin;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOC,
        &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/**
 * @brief TIM6 100 Hz callback.
 *
 * ISR intentionally does NOT run the PI controller.
 * It only sets a scheduler flag.
 */
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        g_control_10ms_flag = 1U;
    }
}


/**
 * @brief USART interrupt receive-complete callback.
 *
 * This is the missing Step 20 callback.
 */
void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart)
{
    Protocol_RxCallback(huart);
}


void HAL_UART_ErrorCallback(
    UART_HandleTypeDef *huart)
{
    Protocol_ErrorCallback(huart);
}

/* USER CODE END 4 */

/**
  * @brief This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    /*
     * Kill PWM and BTS7960 enables first.
     */
    HardwareMotorSafeOff();

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief Reports the name of the source file and the source line number.
  */
void assert_failed(
    uint8_t *file,
    uint32_t line)
{
    /* USER CODE BEGIN 6 */

    (void)file;
    (void)line;

    /* USER CODE END 6 */
}
#endif
