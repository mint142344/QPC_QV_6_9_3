#pragma once

typedef enum LED_Number {
    LED_GREEN,
    LED_RED,
} LED_Number_t;

void LED_Init(void);
void LED_On(LED_Number_t led);
void LED_Off(LED_Number_t led);
void LED_Toggle(LED_Number_t led);
