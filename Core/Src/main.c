/* USER CODE BEGIN Header */
// Monitor analogico con alarma - Etapa 3: ADC, histéresis y RUN/PAUSE
// NUCLEO-F302R8, reloj del sistema a 64 MHz
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private variables */
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint32_t tiempo_ms = 0;

uint16_t adc_bruto = 0;
uint8_t porcentaje = 0;
uint8_t muestra_valida = 0;
uint8_t adc_en_curso = 0;

uint8_t modo_run = 1;
uint8_t umbral = 70;
uint8_t alarma = 0;

uint8_t boton_raw = 1;
uint8_t boton_estable = 1;
uint32_t boton_cambio_ms = 0;

char tx_linea[112];
uint16_t tx_pos = 0;
uint16_t tx_len = 0;
/* USER CODE END PV */

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM6_Init(void);
static void MX_USART2_UART_Init(void);
static void ActualizarSalidaAlarma(void);
static void EvaluarHisteresis(void);
static void ReiniciarDecisionAlarma(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  // Los dos indicadores comienzan apagados.
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  // Calibracion inicial del ADC para mejorar las mediciones.
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
  {
    Error_Handler();
  }

  uint32_t ultimo_led = tiempo_ms;
  uint32_t ultimo_adc = tiempo_ms;
  uint32_t ultimo_uart = tiempo_ms;

  boton_raw = (HAL_GPIO_ReadPin(BOTON_USER_GPIO_Port, BOTON_USER_Pin) == GPIO_PIN_SET);
  boton_estable = boton_raw;
  boton_cambio_ms = tiempo_ms;
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    uint32_t ahora = tiempo_ms;

    // El boton usa polling y exige estabilidad durante 30 ms.
    uint8_t lectura_boton = (HAL_GPIO_ReadPin(BOTON_USER_GPIO_Port,
                                             BOTON_USER_Pin) == GPIO_PIN_SET);
    if (lectura_boton != boton_raw)
    {
      boton_raw = lectura_boton;
      boton_cambio_ms = ahora;
    }
    if (boton_raw != boton_estable && (uint32_t)(ahora - boton_cambio_ms) >= 30U)
    {
      boton_estable = boton_raw;
      if (boton_estable == 0U)
      {
        modo_run = !modo_run;
        ReiniciarDecisionAlarma();
      }
    }

    if ((uint32_t)(ahora - ultimo_led) >= 500U)
    {
      ultimo_led += 500U;
      HAL_GPIO_TogglePin(LED_ACTIVIDAD_GPIO_Port, LED_ACTIVIDAD_Pin);
    }

    if ((uint32_t)(ahora - ultimo_adc) >= 100U)
    {
      ultimo_adc += 100U;
      if (!adc_en_curso && HAL_ADC_Start(&hadc1) == HAL_OK)
      {
        adc_en_curso = 1;
      }
    }

    if (adc_en_curso && __HAL_ADC_GET_FLAG(&hadc1, ADC_FLAG_EOC))
    {
      adc_bruto = (uint16_t)HAL_ADC_GetValue(&hadc1);
      porcentaje = (uint8_t)(((uint32_t)adc_bruto * 100U + 2047U) / 4095U);
      muestra_valida = 1;
      adc_en_curso = 0;
      HAL_ADC_Stop(&hadc1);
      EvaluarHisteresis();
    }

    if ((uint32_t)(ahora - ultimo_uart) >= 1000U)
    {
      ultimo_uart += 1000U;
      if (muestra_valida && tx_pos == tx_len)
      {
        int n = snprintf(tx_linea, sizeof(tx_linea),
                         "t=%lu;ADC=%u;PCT=%u;THR=%u;MODE=%s;ALARM=%u\r\n",
                         (unsigned long)ahora,
                         (unsigned int)adc_bruto,
                         (unsigned int)porcentaje,
                         (unsigned int)umbral,
                         modo_run ? "RUN" : "PAUSE",
                         (unsigned int)alarma);
        if (n > 0 && n < (int)sizeof(tx_linea))
        {
          tx_pos = 0;
          tx_len = (uint16_t)n;
        }
      }
    }

    // Se transmite un caracter por iteracion, sin retardos bloqueantes.
    if (tx_pos < tx_len && __HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE))
    {
      huart2.Instance->TDR = (uint8_t)tx_linea[tx_pos++];
    }
    /* USER CODE END 3 */
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  // HSE 8 MHz / 2 * 16 = 64 MHz.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC1;
  PeriphClkInit.Adc1ClockSelection = RCC_ADC1PLLCLK_DIV1;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;

  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  // TIM6: 64 MHz / ((63 + 1) * (999 + 1)) = 1000 Hz.
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 63;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

  // PA8: LED rojo de alarma.
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // PB13: LED verde de actividad.
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // PC13: pulsador activo en bajo con pull-up.
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
static void ActualizarSalidaAlarma(void)
{
  HAL_GPIO_WritePin(LED_ALARMA_GPIO_Port, LED_ALARMA_Pin,
                    alarma ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void EvaluarHisteresis(void)
{
  if (!modo_run || !muestra_valida)
  {
    alarma = 0;
  }
  else if (porcentaje >= umbral)
  {
    alarma = 1;
  }
  else if (porcentaje < (uint8_t)(umbral - 5U))
  {
    alarma = 0;
  }
  ActualizarSalidaAlarma();
}

static void ReiniciarDecisionAlarma(void)
{
  // Al reanudar no se conserva el estado previo de la histeresis.
  alarma = (modo_run && muestra_valida && porcentaje >= umbral) ? 1U : 0U;
  ActualizarSalidaAlarma();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    tiempo_ms++;
  }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
