/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "led_driver.h"      
#include "ring_buffer.h"     
#include "keypad_driver.h"  
#include <stdio.h>
#include <string.h> 

/* Private define ------------------------------------------------------------*/
#define ACCESS_CODE "1234"       // Clave de acceso válida
#define ACCESS_CODE_LEN 4        // Longitud de la clave
#define RING_BUFFER_SIZE 16      // Tamaño del buffer circular para UART
#define KEYPAD_BUFFER_LEN 16     // Tamaño del buffer circular para keypad

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;       // Estructura de la UART2

// Estructura para LED LD2
led_handle_t led1 = { .port = GPIOA, .pin = GPIO_PIN_5 };

// Buffers y estructuras para ring buffer de UART y keypad
uint8_t rb_buffer[RING_BUFFER_SIZE];
ring_buffer_t rb;

keypad_handle_t keypad = {
    .row_ports = {KEYPAD_R1_GPIO_Port, KEYPAD_R2_GPIO_Port, KEYPAD_R3_GPIO_Port, KEYPAD_R4_GPIO_Port},
    .row_pins  = {KEYPAD_R1_Pin, KEYPAD_R2_Pin, KEYPAD_R3_Pin, KEYPAD_R4_Pin},
    .col_ports = {KEYPAD_C1_GPIO_Port, KEYPAD_C2_GPIO_Port, KEYPAD_C3_GPIO_Port, KEYPAD_C4_GPIO_Port},
    .col_pins  = {KEYPAD_C1_Pin, KEYPAD_C2_Pin, KEYPAD_C3_Pin, KEYPAD_C4_Pin}
};

uint8_t keypad_buffer[KEYPAD_BUFFER_LEN];
ring_buffer_t keypad_rb;

char input_code[ACCESS_CODE_LEN + 1] = {0}; // Buffer para la clave ingresada
uint8_t input_index = 0;                    // Índice para la clave ingresada

/* Prototipos de funciones de inicialización */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* --- Callbacks y funciones auxiliares de usuario --- */

// Callback de recepción UART: guarda cada byte recibido en el ring buffer
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2) {
    uint8_t received_byte;
    HAL_UART_Receive_IT(&huart2, &received_byte, 1);
    ring_buffer_write(&rb, received_byte);
  }
}

// Redirección de printf a UART2
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// Callback de interrupción EXTI 
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {}

/* --- Función principal --- */
int main(void)
{
  /* Inicialización de HAL y periféricos básicos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* Inicialización de módulos de usuario */
  led_init(&led1);                                 // Inicializa el LED
  ring_buffer_init(&rb, rb_buffer, RING_BUFFER_SIZE);         // Inicializa ring buffer UART
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_BUFFER_LEN); // Inicializa ring buffer keypad
  keypad_init(&keypad);                            // Inicializa el keypad

  printf("Sistema listo. Esperando pulsaciones del teclado...\r\n");

  char last_key = '\0'; // Para evitar repeticiones por rebote

  while (1) {
    uint8_t key;

    // Escaneo periódico del keypad: detecta y muestra la tecla presionada
    for (int col = 0; col < KEYPAD_COLS; col++) {
        char key_pressed = keypad_scan(&keypad, keypad.col_pins[col]);
        if (key_pressed != '\0' && key_pressed != last_key) {
            printf("Tecla presionada: %c\r\n", key_pressed);
            ring_buffer_write(&keypad_rb, (uint8_t)key_pressed);
            last_key = key_pressed;
            HAL_Delay(150); // debounce simple
        }
        if (key_pressed == '\0') {
            last_key = '\0';
        }
    }

    // Procesa las teclas del keypad y arma la clave ingresada
    while (ring_buffer_read(&keypad_rb, &key)) {
        if (input_index < ACCESS_CODE_LEN &&
            ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'D') || key == '*' || key == '#')) {
            input_code[input_index++] = (char)key;
        }
    }

    // Cuando se ingresan 4 teclas válidas, verifica la clave
    if (input_index == ACCESS_CODE_LEN) {
        input_code[ACCESS_CODE_LEN] = '\0';

        // Muestra la clave ingresada
        char msg[32];
        snprintf(msg, sizeof(msg), "\r\nClave ingresada: %s\r\n", input_code);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

        if (strcmp(input_code, ACCESS_CODE) == 0) {
            // Clave correcta: LED encendido 3 segundos
            led_on(&led1);
            HAL_Delay(3000);
            led_off(&led1);
            char ok_msg[] = "Clave correcta\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)ok_msg, sizeof(ok_msg)-1, HAL_MAX_DELAY);
        } else {
            // Clave incorrecta: LED parpadea durante 3 segundos
            for (int i = 0; i < 6; i++) { // 6 ciclos de 500ms = 3s
                led_toggle(&led1);
                HAL_Delay(250);
                led_toggle(&led1);
                HAL_Delay(250);
            }
            char fail_msg[] = "Clave incorrecta\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)fail_msg, sizeof(fail_msg)-1, HAL_MAX_DELAY);
        }

        char wait_msg[] = "\r\nEsperando pulsaciones del teclado...\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t*)wait_msg, sizeof(wait_msg)-1, HAL_MAX_DELAY);

        // Reinicia el buffer de entrada para un nuevo intento
        input_index = 0;
        memset(input_code, 0, sizeof(input_code));
    }

    HAL_Delay(10); 
  }
}

/* --- Funciones de inicialización generadas por CubeMX --- */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6; // 4 MHz, ajusta si necesitas más velocidad
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Filas como salida
  GPIO_InitStruct.Pin = KEYPAD_R1_Pin|KEYPAD_R2_Pin|KEYPAD_R3_Pin|KEYPAD_R4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KEYPAD_R1_GPIO_Port, &GPIO_InitStruct);

  // Columnas como entrada con pull-down
  GPIO_InitStruct.Pin = KEYPAD_C1_Pin|KEYPAD_C2_Pin|KEYPAD_C3_Pin|KEYPAD_C4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KEYPAD_C1_GPIO_Port, &GPIO_InitStruct);

}

/* --- Inicialización de USART2 --- */
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
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* --- Manejo de errores --- */
void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    
  }
}
