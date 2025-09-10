#include "spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include <stdint.h>

void SPI1_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    
    SPI_InitTypeDef SPI_InitStructure;
		GPIO_InitTypeDef GPIO_InitStruct;
    /** \b 编译器(v5.06)bug GPIO结构体声明顺序必须在SPI结构体之后(都是栈内存下，非静态变量),除非优化等级大于2
     * \b 离谱 GPIO_InitTypeDef GPIO_InitStruct; 必须放在SPI_InitTypeDef SPI_InitStructure;之后，否则串口中断进不去
     */

    // 必须先配置SPI 再配置GPIO
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI1_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Pin  = SPI1_MISO_PIN;
    GPIO_Init(SPI1_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin  = SPI1_CS_PIN;
    GPIO_Init(SPI1_PORT, &GPIO_InitStruct);

    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex; // 全双工
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;                 // 主机
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_16b;                // 帧大小16位
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;                    // 空闲时钟低电平
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;                  // 第1边沿采样
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;                    // 软件 CS 引脚
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;       //
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;                // 高字节先发送
    SPI_InitStructure.SPI_CRCPolynomial     = 7;                               // CRC多项式，默认值
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI1_NSS_HIGH();

    SPI_Cmd(SPI1, ENABLE);
}
