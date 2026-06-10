/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: Health monitor with LVGL + SLE 1VN \n
 *
 * History: \n
 * 2024-03-04, Create file. \n
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
#include "sle_1vn_server.h"
#include "sensor_task.h"

#define UI_TASK_PRIO                    24
#define UI_TASK_STACK_SIZE              0x8000
#define LVGL_TICK_PERIOD_MS             5
#define SLE_STATUS_POLL_MS              500

static void on_identity_received(const char *name, const char *id_number)
{
    osal_printk("[main] identity: %s %s\r\n", name, id_number);
    ui_health_set_identity(name, id_number);
}

static void update_sle_status(void)
{
    bool connected = sle_1vn_server_is_connected();
    ui_health_set_sle_status(connected, "watch-01", connected ? -45 : 0);
}

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

    /* 注册身份回调 */
    wearable_register_identity_callback(on_identity_received);

    /* 初始化 SLE 服务端 */
    ret = sle_1vn_server_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[main] SLE init failed 0x%x\r\n", ret);
    } else {
        osal_printk("[main] SLE init ok\r\n");
        sensor_task_start();
    }

    osal_printk("[main] LVGL loop start\r\n");

    uint32_t last_sle_poll = 0;

    for (;;) {
        uint32_t now = osal_jiffies_to_msecs((unsigned int)osal_get_jiffies());

        /* LVGL 主循环 */
        lv_timer_handler();

        /* 定期更新 SLE 状态 */
        if ((now - last_sle_poll) >= SLE_STATUS_POLL_MS) {
            update_sle_status();
            last_sle_poll = now;
        }

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
