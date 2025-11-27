#pragma once

#include <qpc.h>

typedef struct QLED {
    QActive super;
    QTimeEvt m_timer;
} QLED;

void QLED_Start(uint_fast8_t prio,
                QEvt const **const qSto, uint_fast16_t const qLen,
                void *const stkSto, uint_fast16_t const stkSize,
                void const *const par);