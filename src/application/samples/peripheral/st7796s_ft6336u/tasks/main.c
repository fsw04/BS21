/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: Health monitor with LVGL \n
 */
#include "soc_osal.h"
#include "securec.h"
#include "app_init.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui_health_lvgl.h"
#include "st7796s.h"
#include "ft6336u.h"
#include "lcd_bus.h"

#define UI_TASK_PRIO                    24
#define UI_TASK_STACK_SIZE              0x3000
#define LVGL_TICK_PERIOD_MS             5

static void *ui_task(const char *arg)
{
    (void)arg;

    osal_printk("[main] ui_task enter\r\n");

    /* 初始化 LCD 硬件 */
    osal_printk("[main] LCD init...\r\n");
    errcode_t ret = st7796s_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[main] LCD init failed 0x%x\r\n", ret);
        return NULL;
    }
    osal_printk("[main] LCD init ok\r\n");

    /* 初始化触控 */
    osal_printk("[main] Touch init...\r\n");
    ret = ft6336u_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[main] Touch init failed 0x%x\r\n", ret);
        return NULL;
    }
    osal_printk("[main] Touch init ok\r\n");

    /* 初始化 LVGL */
    osal_printk("[main] LVGL init...\r\n");
    lv_init();
    osal_printk("[main] LVGL disp init...\r\n");
    lv_port_disp_init();
    osal_printk("[main] LVGL indev init...\r\n");
    lv_port_indev_init();
    osal_printk("[main] LVGL init ok\r\n");

    /* 创建健康监测 UI */
    osal_printk("[main] UI create...\r\n");
    ui_health_create();
    osal_printk("[main] UI create ok\r\n");

    osal_printk("[main] LVGL loop start\r\n");

    for (;;) {
        lv_timer_handler();
        osal_msleep(LVGL_TICK_PERIOD_MS);
    }

    return NULL;
}

static void app_entry(void)
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

app_run(app_entry);
