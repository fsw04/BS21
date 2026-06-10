/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: LCD + Touch Demo Application (Fixed for BS21). \n
 *              在ST7796S屏幕上显示按钮，通过FT6336U触摸屏点击按钮。 \n
 *
 * History: \n
 * 2024-01-01, Create file. \n
 * 2024-06-08, Fix touch init failure handling. \n
 */
#include "st7796s.h"
#include "ft6336u.h"
#include "soc_osal.h"
#include "app_init.h"
#include "errcode.h"
#include "common_def.h"

#define LCD_TOUCH_TASK_PRIO         24
#define LCD_TOUCH_TASK_STACK_SIZE   0x2000
#define TOUCH_POLL_INTERVAL_MS      50

/* 按钮定义 */
#define BTN_MAX_COUNT   6

typedef struct {
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    const char *label;
    uint16_t color;
    uint16_t text_color;
    bool pressed;
} button_t;

/* 按钮状态计数 */
static uint8_t g_btn_click_count[BTN_MAX_COUNT] = {0};

/* 触摸是否可用 */
static bool g_touch_available = false;

/* 按钮布局 */
static button_t g_buttons[BTN_MAX_COUNT] = {
    { 20,  40,  150, 110, "BTN1", COLOR_BLUE,   COLOR_WHITE, false },
    { 170, 40,  300, 110, "BTN2", COLOR_GREEN,  COLOR_WHITE, false },
    { 20,  130, 150, 200, "BTN3", COLOR_RED,    COLOR_WHITE, false },
    { 170, 130, 300, 200, "BTN4", COLOR_ORANGE, COLOR_WHITE, false },
    { 20,  220, 150, 290, "BTN5", COLOR_CYAN,   COLOR_BLACK, false },
    { 170, 220, 300, 290, "BTN6", COLOR_MAGENTA,COLOR_WHITE, false },
};

/* 绘制单个按钮 */
static void draw_button(const button_t *btn, bool pressed)
{
    uint16_t bg_color = pressed ? COLOR_GRAY : btn->color;
    uint16_t border_color = pressed ? COLOR_WHITE : COLOR_DARKGRAY;

    /* 绘制按钮背景 */
    st7796s_fill_rect(btn->x1, btn->y1, btn->x2, btn->y2, bg_color);

    /* 绘制按钮边框 */
    st7796s_draw_rect(btn->x1, btn->y1, btn->x2, btn->y2, border_color);

    /* 绘制按钮文字(居中) */
    uint8_t len = 0;
    const char *p = btn->label;
    while (*p != '\0') {
        len++;
        p++;
    }
    uint16_t text_width = len * 8 * 2;  /* size=2, 每字符16像素宽 */
    uint16_t text_height = 16 * 2;       /* size=2, 字符高32像素 */
    uint16_t btn_width = btn->x2 - btn->x1 + 1;
    uint16_t btn_height = btn->y2 - btn->y1 + 1;
    uint16_t text_x = btn->x1 + (btn_width - text_width) / 2;
    uint16_t text_y = btn->y1 + (btn_height - text_height) / 2;

    st7796s_draw_string(text_x, text_y, btn->label, btn->text_color, bg_color, 2);
}

/* 绘制所有按钮 */
static void draw_all_buttons(void)
{
    for (int i = 0; i < BTN_MAX_COUNT; i++) {
        draw_button(&g_buttons[i], g_buttons[i].pressed);
    }
}

/* 绘制点击计数显示区域 */
static void draw_status_area(void)
{
    /* 状态栏背景 */
    st7796s_fill_rect(0, 310, ST7796S_WIDTH - 1, ST7796S_HEIGHT - 1, COLOR_DARKGRAY);

    /* 标题 */
    st7796s_draw_string(10, 315, "Click Count:", COLOR_WHITE, COLOR_DARKGRAY, 2);

    /* 显示每个按钮的点击次数 */
    char buf[16] = {0};
    for (int i = 0; i < BTN_MAX_COUNT; i++) {
        uint16_t x = 10 + (i % 3) * 110;
        uint16_t y = 345 + (i / 3) * 35;
        buf[0] = '0' + (i + 1);
        buf[1] = ':';
        buf[2] = ' ';
        buf[3] = '0' + (g_btn_click_count[i] / 10) % 10;
        buf[4] = '0' + g_btn_click_count[i] % 10;
        buf[5] = '\0';
        st7796s_draw_string(x, y, buf, COLOR_YELLOW, COLOR_DARKGRAY, 2);
    }
}

/* 绘制标题 */
static void draw_title(void)
{
    st7796s_fill_rect(0, 0, ST7796S_WIDTH - 1, 35, COLOR_BLACK);
    st7796s_draw_string(40, 5, "ST7796S+FT6336U", COLOR_WHITE, COLOR_BLACK, 2);
}

/* 绘制触摸状态 */
static void draw_touch_status(void)
{
    const char *status = g_touch_available ? "Touch: OK" : "Touch: N/A";
    uint16_t color = g_touch_available ? COLOR_GREEN : COLOR_RED;
    st7796s_draw_string(200, 5, status, color, COLOR_BLACK, 1);
}

/* 检测触摸点是否在按钮内 */
static int check_button_hit(uint16_t tx, uint16_t ty)
{
    for (int i = 0; i < BTN_MAX_COUNT; i++) {
        if (tx >= g_buttons[i].x1 && tx <= g_buttons[i].x2 &&
            ty >= g_buttons[i].y1 && ty <= g_buttons[i].y2) {
            return i;
        }
    }
    return -1;
}

/* 模拟按钮点击（用于触摸不可用时演示） */
static void simulate_button_cycle(void)
{
    static uint32_t counter = 0;
    static int current_btn = 0;

    counter++;
    if (counter % 30 == 0) {  /* 每30次循环约1.5秒 */
        /* 释放上一个按钮 */
        if (current_btn > 0) {
            g_buttons[current_btn - 1].pressed = false;
            draw_button(&g_buttons[current_btn - 1], false);
        }

        /* 按下当前按钮 */
        if (current_btn < BTN_MAX_COUNT) {
            g_buttons[current_btn].pressed = true;
            draw_button(&g_buttons[current_btn], true);
            g_btn_click_count[current_btn]++;
            draw_status_area();
            osal_printk("Simulated Button %d clicked! count=%d\r\n", 
                       current_btn + 1, g_btn_click_count[current_btn]);
            current_btn++;
        } else {
            current_btn = 0;
        }
    }
}

/* 触摸处理 */
static void process_touch(void)
{
    if (!g_touch_available) {
        /* 触摸不可用，演示模式 */
        simulate_button_cycle();
        return;
    }

    ft6336u_touch_t touches[FT6336U_MAX_TOUCHES];
    uint8_t count = 0;

    errcode_t ret = ft6336u_read_touches(touches, &count);
    if (ret != ERRCODE_SUCC || count == 0) {
        /* 没有触摸，释放所有按钮 */
        for (int i = 0; i < BTN_MAX_COUNT; i++) {
            if (g_buttons[i].pressed) {
                g_buttons[i].pressed = false;
                draw_button(&g_buttons[i], false);
            }
        }
        return;
    }

    /* 处理触摸点 */
    bool btn_hit[BTN_MAX_COUNT] = {false};
    for (uint8_t t = 0; t < count; t++) {
        uint16_t tx = touches[t].x;
        uint16_t ty = touches[t].y;

        /* 坐标范围检查 */
        if (tx >= ST7796S_WIDTH) {
            tx = ST7796S_WIDTH - 1;
        }
        if (ty >= ST7796S_HEIGHT) {
            ty = ST7796S_HEIGHT - 1;
        }

        int btn_idx = check_button_hit(tx, ty);
        if (btn_idx >= 0) {
            btn_hit[btn_idx] = true;

            if (touches[t].event == FT6336U_TOUCH_EVENT_DOWN ||
                touches[t].event == FT6336U_TOUCH_EVENT_CONTACT) {
                if (!g_buttons[btn_idx].pressed) {
                    g_buttons[btn_idx].pressed = true;
                    draw_button(&g_buttons[btn_idx], true);
                }
            } else if (touches[t].event == FT6336U_TOUCH_EVENT_UP) {
                if (g_buttons[btn_idx].pressed) {
                    g_buttons[btn_idx].pressed = false;
                    g_btn_click_count[btn_idx]++;
                    draw_button(&g_buttons[btn_idx], false);
                    draw_status_area();
                    osal_printk("Button %d clicked! count=%d\r\n", 
                               btn_idx + 1, g_btn_click_count[btn_idx]);
                }
            }
        }
    }

    /* 释放未被触摸的按钮 */
    for (int i = 0; i < BTN_MAX_COUNT; i++) {
        if (!btn_hit[i] && g_buttons[i].pressed) {
            g_buttons[i].pressed = false;
            draw_button(&g_buttons[i], false);
        }
    }
}

/* 主任务 */
static void *lcd_touch_task(const char *arg)
{
    unused(arg);

    /* 初始化LCD */
    osal_printk("Initializing ST7796S LCD...\r\n");
    errcode_t ret = st7796s_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("ST7796S init failed: %d\r\n", ret);
        return NULL;
    }

    /* 初始化触摸屏 - 添加try-catch风格保护 */
    osal_printk("Initializing FT6336U touch...\r\n");
    ret = ft6336u_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("FT6336U init failed: %d, LCD demo will continue without touch\r\n", ret);
        g_touch_available = false;
        /* 触摸失败不返回，继续运行LCD演示 */
    } else {
        g_touch_available = true;
        osal_printk("FT6336U touch init success\r\n");
    }

    /* 清屏 */
    st7796s_clear(COLOR_BLACK);

    /* 绘制UI */
    draw_title();
    draw_all_buttons();
    draw_status_area();
    draw_touch_status();

    osal_printk("LCD Touch demo started. Touch available: %d\r\n", g_touch_available);

    /* 主循环：轮询触摸 */
    while (1) {
        process_touch();
        osal_msleep(TOUCH_POLL_INTERVAL_MS);
    }

    return NULL;
}

/* 入口函数 */
static void lcd_touch_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)lcd_touch_task, 0,
                                      "LcdTouchTask", LCD_TOUCH_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, LCD_TOUCH_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* 运行入口 */
app_run(lcd_touch_entry);