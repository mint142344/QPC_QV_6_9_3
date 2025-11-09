#include "Q_Main.h"
#include "Q_LED.h"

#include "stm32f10x.h"
#include <stdio.h>

static QEvt const *s_led_events[10]; // LED 事件队列

void StartActiveObjects(void)
{
    uint8_t priority = 1U;
    QLED_Start(priority++, s_led_events, Q_DIM(s_led_events), (void *)0, 0U, (QEvt *)0);
}

// QP框架回调函数
void Q_onAssert(char const *const module, int_t const location)
{
    (void)module;
    (void)location;

    printf("Assertion failed in module: %s, ID: %d\r\n", module, location);
    // NVIC_SystemReset();
    QF_INT_DISABLE();
    while (1) {
    }
}

void QF_onStartup(void)
{
    // 开启系统定时器 使能中断
    NVIC_SetPriorityGrouping(2U);
    SysTick_Config(SystemCoreClock / 1000);

    // 设置SysTick中断优先级
    NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI);

    printf("All peripheral and QP framework initialized.\r\n");
}

void QF_onCleanup(void)
{
}

void QV_onIdle(void)
{
#if defined NDEBUG
    /* Put the CPU and peripherals to the low-power mode */
    QV_CPU_SLEEP(); /* atomically go to sleep and enable interrupts */
#else
    QF_INT_ENABLE(); /* just enable interrupts */
#endif
}
