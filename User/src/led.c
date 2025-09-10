#include "led.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"

#define LED_GREEN_PIN GPIO_Pin_0
#define LED_RED_PIN   GPIO_Pin_1
#define LED_PORT      GPIOA

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin   = LED_GREEN_PIN | LED_RED_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

void LED_On(LED_Number_t led)
{
    switch (led) {
        case LED_GREEN:
            GPIO_SetBits(LED_PORT, LED_GREEN_PIN);
            break;
        case LED_RED:
            GPIO_SetBits(LED_PORT, LED_RED_PIN);
            break;
    }
}
void LED_Off(LED_Number_t led)
{
    switch (led) {
        case LED_GREEN:
            GPIO_ResetBits(LED_PORT, LED_GREEN_PIN);
            break;
        case LED_RED:
            GPIO_ResetBits(LED_PORT, LED_RED_PIN);
            break;
    }
}
void LED_Toggle(LED_Number_t led)
{
    switch (led) {
        case LED_GREEN:
            GPIO_WriteBit(LED_PORT, LED_GREEN_PIN, (BitAction) !(GPIO_ReadOutputDataBit(LED_PORT, LED_GREEN_PIN)));
            break;
        case LED_RED:
            GPIO_WriteBit(LED_PORT, LED_RED_PIN, (BitAction) !(GPIO_ReadOutputDataBit(LED_PORT, LED_RED_PIN)));
            break;
    }
}
