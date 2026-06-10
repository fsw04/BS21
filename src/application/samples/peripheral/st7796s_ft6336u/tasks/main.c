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
#include "ui_page_health.h"
#include "font.h"
#include "sle_1vn_server.h"
#include "sensor_task.h"
#include "measurement_session.h"

#define UI_TASK_PRIO                    24
#define UI_TASK_STACK_SIZE              0x2000
#define SLE_TASK_PRIO                   26
#define SLE_TASK_STACK_SIZE             0x4000

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

    errcode_t ret = ui_health_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[main] UI init failed\r\n");
        return NULL;
    }

    font_init(NULL, 0);  /* HZK16 data not available yet, Chinese shows as blocks */

    /* Register identity callback */
    wearable_register_identity_callback(on_identity_received);

    /* Initialize SLE server */
    ret = sle_1vn_server_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[main] SLE init failed 0x%x\r\n", ret);
    } else {
        osal_printk("[main] SLE init ok\r\n");
        sensor_task_start();
    }

    osal_printk("[main] UI loop start\r\n");

    for (;;) {
        update_sle_status();
        ui_health_refresh();
        ui_health_touch_loop();
        osal_msleep(50);
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
