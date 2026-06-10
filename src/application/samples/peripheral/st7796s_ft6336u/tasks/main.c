/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: Provides UI button sample source \n
 *
 * History: \n
 * 2024-03-04, Create file. \n
 */
#include "soc_osal.h"
#include "securec.h"
#include "app_init.h"
#include "ui_button.h"

#define UI_TASK_PRIO                    24
#define UI_TASK_STACK_SIZE              0x2000

/* 按钮点击回调 */
static void btn_ok_click(void)
{
    osal_printk("[UI] >>> OK Button Clicked!\r\n");
}

static void btn_cancel_click(void)
{
    osal_printk("[UI] >>> Cancel Button Clicked!\r\n");
}

static void *ui_task(const char *arg)
{
    (void)(arg);  /* 消除 unused parameter 警告，替代 unused(arg) */

    errcode_t ret = ui_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[UI] init failed, task exit\r\n");
        return NULL;
    }

    /* 定义按钮1：OK（绿色） */
    static ui_button_t btn_ok = {
        .x = 60,  .y = 180,
        .width = 200, .height = 60,
        .color = COL_GREEN,
        .color_pressed = 0xAFE5,   /* 浅绿 */
        .border_color = COL_WHITE,
        .label = "OK",
        .on_click = btn_ok_click,
        .pressed = 0
    };

    /* 定义按钮2：Cancel（红色） */
    static ui_button_t btn_cancel = {
        .x = 60,  .y = 260,
        .width = 200, .height = 60,
        .color = COL_RED,
        .color_pressed = 0xFD20,   /* 浅红 */
        .border_color = COL_WHITE,
        .label = "Cancel",
        .on_click = btn_cancel_click,
        .pressed = 0
    };

    ui_button_create(0, &btn_ok);
    ui_button_create(1, &btn_cancel);
    ui_button_draw_all();

    osal_printk("[UI] touch loop start\r\n");

    for (;;) {
        ui_touch_task_loop();
        osal_msleep(UI_TASK_INTERVAL_MS);
    }

    return NULL;
}

static void ui_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)ui_task, 0, "UITask", UI_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, UI_TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

/* Run the ui_entry. */
app_run(ui_entry);