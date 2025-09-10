#include "Q_Main.h"

#include "qpc.h"

#include "spi.h"
#include "uart.h"
#include "led.h"

int main(void)
{
    QF_init();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    LED_Init();
    SPI1_Init();
    UART1_Init();

    StartActiveObjects();

    return QF_run();
}
