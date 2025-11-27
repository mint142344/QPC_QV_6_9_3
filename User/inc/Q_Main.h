#pragma once

#include <qpc.h>

typedef enum Signal {
    SIG_DUMMY = Q_USER_SIG,
    // LED
    SIG_LED_TIMEOUT,

    // 屏幕刷新
    SIG_SCREEN_REFRESH_TIMEOUT,

    // 按键
    SIG_KEY_TIMEOUT,        // 屏幕按键捕获
    SIG_KEY_PRESSED,        // 短按事件
    SIG_KEY_RELEASED,       // 短按释放事件
    SIG_KEY_LONG_PRESSED,   // 长按事件
    SIG_KEY_LONG_RELEASED,  // 长按释放事件
    SIG_KEY_REPEAT_TIMEOUT, // 长按重复超时事件

    SIG_SIZE
} Signal_t;

// 构造所有活动对象
void StartActiveObjects(void);
