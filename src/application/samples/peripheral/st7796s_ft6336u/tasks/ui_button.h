#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include <stdint.h>
#include "errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_MAX_BUTTONS      5
#define UI_TASK_INTERVAL_MS 20

/* RGB565 常用颜色（暴露给应用层使用） */
#define COL_BLACK   0x0000
#define COL_WHITE   0xFFFF
#define COL_RED     0xF800
#define COL_GREEN   0x07E0
#define COL_BLUE    0x001F
#define COL_YELLOW  0xFFE0
#define COL_GRAY    0x8410

/* 按钮释放时触发的回调 */
typedef void (*ui_button_cb_t)(void);

typedef struct {
    uint16_t x, y;          /* 左上角坐标 */
    uint16_t width, height; /* 尺寸 */
    uint16_t color;         /* 正常状态颜色 (RGB565) */
    uint16_t color_pressed; /* 按下状态颜色 (RGB565) */
    uint16_t border_color;  /* 边框颜色 */
    const char *label;      /* 文字标签（预留，需配合字库） */
    ui_button_cb_t on_click;/* 释放时触发的回调 */
    uint8_t  pressed;       /* 内部状态：是否处于按下态 */
} ui_button_t;

/* 初始化 LCD + 触摸屏，并清屏为黑色 */
errcode_t ui_init(void);

/* 注册按钮到指定槽位（id: 0 ~ UI_MAX_BUTTONS-1） */
void ui_button_create(uint8_t id, ui_button_t *btn);

/* 绘制单个按钮 / 全部按钮 */
void ui_button_draw(uint8_t id);
void ui_button_draw_all(void);

/* 单次触摸扫描与状态处理（需在任务循环中周期性调用） */
void ui_touch_task_loop(void);

#ifdef __cplusplus
}
#endif

#endif