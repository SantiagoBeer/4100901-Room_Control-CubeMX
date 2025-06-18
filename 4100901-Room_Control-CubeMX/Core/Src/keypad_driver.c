#include "keypad_driver.h"

static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

void keypad_init(keypad_handle_t* keypad) {
    // Configurar los pines de las filas como salidas
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET); // Inicializar en alto
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = keypad->row_pins[i];
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(keypad->row_ports[i], &GPIO_InitStruct);
    }

    // Configurar los pines de las columnas como entradas con pull-up y con interrupción por flanco de bajada
    for (int j = 0; j < KEYPAD_COLS; j++) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = keypad->col_pins[j];
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(keypad->col_ports[j], &GPIO_InitStruct);
    }
}

char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    HAL_Delay(5);

    int col_index = -1;
    for (int i = 0; i < KEYPAD_COLS; i++) {
        if (keypad->col_pins[i] == col_pin) {
            col_index = i;
            break;
        }
    }
    if (col_index == -1) {
        return '\0';
    }

    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }

    char key_pressed = '\0';
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

        GPIO_PinState state = HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]);
        if (state == GPIO_PIN_RESET) {
            key_pressed = keypad_map[row][col_index];
            HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_SET);
            break;
        }
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_SET);
    }

    keypad_init(keypad); 
    return key_pressed;
}