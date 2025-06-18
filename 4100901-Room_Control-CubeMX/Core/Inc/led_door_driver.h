#ifndef LED_DOOR_DRIVER_H
#define LED_DOOR_DRIVER_H

#include <stdint.h>
#include "main.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} led_door_handle_t;

void led_door_init(led_door_handle_t *led);
void led_door_on(led_door_handle_t *led);
void led_door_off(led_door_handle_t *led);
void led_door_toggle(led_door_handle_t *led);

#endif // LED_DOOR_DRIVER_H