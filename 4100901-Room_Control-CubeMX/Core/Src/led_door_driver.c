#include "led_door_driver.h"

void led_door_init(led_door_handle_t *led) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
}

void led_door_on(led_door_handle_t *led) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
}

void led_door_off(led_door_handle_t *led) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
}

void led_door_toggle(led_door_handle_t *led) {
    HAL_GPIO_TogglePin(led->port, led->pin);
}