#include "sensor_task.h"

#include <string.h>
#include "common_def.h"
#include "securec.h"
#include "soc_osal.h"

#include "measurement_session.h"
#include "sensor_collect.h"
#include "sle_1vn_server/sle_1vn_server.h"

#define SENSOR_TASK_LOG "[sensor task]"
#define SENSOR_TASK_STACK_SIZE 0x1000
#define SENSOR_TASK_PRIO 26
#define SENSOR_WAIT_UPLINK_MS 500
#define SENSOR_WAIT_IDENTITY_MS 500
#define SENSOR_SESSION_INTERVAL_MS 10000
#define SENSOR_ITEM_VALUE_LEN 32

static const sensor_collect_target_t g_collect_targets[] = {
    {MEASUREMENT_ITEM_HEIGHT, "height", "sensor-height"},
    {MEASUREMENT_ITEM_WEIGHT, "weight", "sensor-weight"},
    {MEASUREMENT_ITEM_BP, "blood-pressure", "sensor-bp"},
    {MEASUREMENT_ITEM_GLUCOSE, "glucose", "sensor-glucose"},
};

static void sensor_task_wait_uplink(void)
{
    while (!sle_1vn_server_is_connected()) {
        osal_printk("%s waiting master server long connection...\r\n", SENSOR_TASK_LOG);
        osal_msleep(SENSOR_WAIT_UPLINK_MS);
    }
}

static void sensor_task_wait_identity(uint32_t last_sequence, wearable_identity_t *identity)
{
    wearable_identity_t *current = NULL;

    if (identity == NULL) {
        return;
    }

    while (1) {
        current = wearable_get_identity();
        if ((current != NULL) && (current->identity_received != 0) &&
            (current->sequence != last_sequence)) {
            (void)memcpy_s(identity, sizeof(wearable_identity_t), current, sizeof(wearable_identity_t));
            osal_printk("%s identity ready seq:%u name:%s id:%s\r\n",
                        SENSOR_TASK_LOG, identity->sequence, identity->name, identity->id_number);
            return;
        }

        osal_printk("%s waiting identity...\r\n", SENSOR_TASK_LOG);
        osal_msleep(SENSOR_WAIT_IDENTITY_MS);
    }
}

static void sensor_task_print_json(const char *stage, const measurement_session_t *session)
{
    char report[MEASUREMENT_REPORT_LEN] = {0};
    int len = measurement_session_build_report(session, report, sizeof(report));

    if (len <= 0) {
        osal_printk("%s build %s json failed\r\n", SENSOR_TASK_LOG, stage);
        return;
    }

    osal_printk("%s %s json:%s\r\n", SENSOR_TASK_LOG, stage, report);
}

static void sensor_task_collect_session(measurement_session_t *session)
{
    uint32_t i;
    char value[SENSOR_ITEM_VALUE_LEN] = {0};

    for (i = 0; i < (sizeof(g_collect_targets) / sizeof(g_collect_targets[0])); i++) {
        if ((session->received_mask & g_collect_targets[i].item_mask) != 0) {
            continue;
        }

        (void)memset_s(value, sizeof(value), 0, sizeof(value));
        if (sensor_collect_item(&g_collect_targets[i], value, sizeof(value)) == 0) {
            measurement_session_mark_value(session, g_collect_targets[i].item_mask, value);
            osal_printk("%s collected %s:%s mask:0x%x/0x%x\r\n",
                        SENSOR_TASK_LOG, g_collect_targets[i].name, value,
                        session->received_mask, session->required_mask);
        } else {
            osal_printk("%s collect %s failed\r\n", SENSOR_TASK_LOG, g_collect_targets[i].name);
        }
        sensor_task_print_json(g_collect_targets[i].name, session);
    }
}

static void sensor_task_upload_report(const measurement_session_t *session)
{
    char report[MEASUREMENT_REPORT_LEN] = {0};
    int len = measurement_session_build_report(session, report, sizeof(report));

    if (len <= 0) {
        osal_printk("%s build report failed\r\n", SENSOR_TASK_LOG);
        return;
    }

    sensor_task_wait_uplink();
    if (sle_1vn_server_send_report(report, (uint16_t)len) == 0) {
        osal_printk("%s report uploaded len:%d\r\n", SENSOR_TASK_LOG, len);
    } else {
        osal_printk("%s report upload failed len:%d ready:%u mtu:%u\r\n",
                    SENSOR_TASK_LOG, len,
                    sle_1vn_server_report_is_ready() ? 1 : 0,
                    (unsigned int)sle_1vn_server_report_mtu());
    }
}

static int sensor_main_task(const char *arg)
{
    uint32_t handled_identity_sequence = 0;

    unused(arg);

    osal_printk("%s start\r\n", SENSOR_TASK_LOG);
    while (1) {
        wearable_identity_t identity = {0};
        measurement_session_t session;

        sensor_task_wait_uplink();
        sensor_task_wait_identity(handled_identity_sequence, &identity);
        handled_identity_sequence = identity.sequence;

        measurement_session_init(&session);
        measurement_session_set_identity(&session, identity.name, identity.id_number);
        sensor_task_print_json("identity", &session);
        sensor_task_collect_session(&session);

        if (measurement_session_is_complete(&session)) {
            osal_printk("%s session complete, upload report\r\n", SENSOR_TASK_LOG);
            sensor_task_upload_report(&session);
        } else {
            osal_printk("%s session incomplete mask:0x%x/0x%x\r\n",
                        SENSOR_TASK_LOG, session.received_mask, session.required_mask);
        }

        osal_msleep(SENSOR_SESSION_INTERVAL_MS);
    }
    return 0;
}

void sensor_task_start(void)
{
    osal_task *task_handle = NULL;

    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)sensor_main_task, 0,
                                      "SensorTask", SENSOR_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, SENSOR_TASK_PRIO);
        osal_kfree(task_handle);
        osal_printk("%s create ok\r\n", SENSOR_TASK_LOG);
    } else {
        osal_printk("%s create failed\r\n", SENSOR_TASK_LOG);
    }
    osal_kthread_unlock();
}
