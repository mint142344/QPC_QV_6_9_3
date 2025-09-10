#pragma once
#include "stm32f10x.h"

#define SPI1_CS_PIN   GPIO_Pin_4
#define SPI1_SCK_PIN  GPIO_Pin_5
#define SPI1_MISO_PIN GPIO_Pin_6
#define SPI1_MOSI_PIN GPIO_Pin_7
#define SPI1_PORT     GPIOA
#define SPI1_RCC      RCC_APB2Periph_GPIOA

#define SPI1_NSS_HIGH()                             \
    do {                                            \
        GPIO_SetBits(SPI1_PORT, SPI1_CS_PIN);       \
        for (uint16_t i = 0; i < 500; i++) __NOP(); \
    } while (0)

#define SPI1_NSS_LOW()                              \
    do {                                            \
        GPIO_ResetBits(SPI1_PORT, SPI1_CS_PIN);     \
        for (uint16_t i = 0; i < 500; i++) __NOP(); \
    } while (0)

void SPI1_Init(void);