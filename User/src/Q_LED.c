#include "Q_LED.h"
#include "Q_Main.h"

#include "led.h"

#include <stdio.h>

static QLED s_led;

Q_DEFINE_THIS_FILE

static void QLED_Ctor(QLED *const me);
QState QLED_Initial(QLED *const me, QEvt const *const e);
QState QLED_Active(QLED *const me, QEvt const *const e);

void QLED_Start(uint_fast8_t prio,
                QEvt const **const qSto, uint_fast16_t const qLen,
                void *const stkSto, uint_fast16_t const stkSize,
                void const *const par)
{
    QLED_Ctor(&s_led);
    QACTIVE_START(&s_led.super, prio, qSto, qLen, stkSto, stkSize, par);
}

void QLED_Ctor(QLED *const me)
{
    QActive_ctor(&me->super, Q_STATE_CAST(&QLED_Initial));
    QTimeEvt_ctorX(&me->m_timer, &me->super, SIG_LED_TIMEOUT, 0U);
}

QState QLED_Initial(QLED *const me, QEvt const *const e)
{
    (void)e;

    QTimeEvt_armX(&me->m_timer, 1000U, 5000U);

    return Q_TRAN(&QLED_Active);
}

QState QLED_Active(QLED *const me, QEvt const *const e)
{
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG:
            LED_Off(LED_GREEN);
            status = Q_HANDLED();
            break;
        case SIG_LED_TIMEOUT:
            LED_Toggle(LED_GREEN);
            status = Q_HANDLED();
            break;
        case Q_EXIT_SIG:
            status = Q_HANDLED();
            break;
        default:
            status = Q_SUPER(&QHsm_top);
            break;
    }
    return status;
}
