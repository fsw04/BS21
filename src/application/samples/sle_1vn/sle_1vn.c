/*
 * BS21E SLE 1VN Server - Main entry
 * Adapted from WS63E sle_1vn_server_demo.c for BS21E SDK.
 */
#include <stdio.h>
#include "soc_osal.h"
#include "app_init.h"
#include "common_def.h"
#include "sle_errcode.h"
#include "sensor_task.h"
#include "sle_1vn_server/sle_1vn_server.h"

static int sle_1vn_server_demo_task(const char *arg)
{
    uint32_t last_identity_sequence = 0;

    unused(arg);

    osal_printk("=== SLE 1VN Server Demo (BS21E) ===\r\n");


    osal_msleep(5000);

    errcode_t ret = sle_1vn_server_init();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("[demo] init FAILED, ret:%x\r\n", ret);
        return -1;
    }

    osal_printk("[demo] init OK, waiting for client...\r\n");
    sensor_task_start();

    while (1) {
        osal_msleep(5000);
        wearable_identity_t *id = wearable_get_identity();
        if (id != NULL && id->identity_received) {
            if (id->sequence != last_identity_sequence) {
                last_identity_sequence = id->sequence;
                osal_printk("[demo] identity seq:%u: %s %s\r\n", id->sequence, id->name, id->id_number);
            }
        } else if (sle_1vn_server_is_connected()) {
            osal_printk("[demo] master server connected, waiting identity/data...\r\n");
        } else {
            osal_printk("[demo] server running, no master connection yet...\r\n");
        }
    }
    return 0;
}

static void sle_1vn_server_demo_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)sle_1vn_server_demo_task, 0,
                                      "sle_1vn_srv", 0x4000);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, 26);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

app_run(sle_1vn_server_demo_entry);
