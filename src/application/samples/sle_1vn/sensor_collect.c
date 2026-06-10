#include "sensor_collect.h"

#include <string.h>
#include "securec.h"
#include "soc_osal.h"

#define SENSOR_COLLECT_LOG "[sensor collect]"
#define SENSOR_CONNECT_DELAY_MS 500
#define SENSOR_MEASURE_DELAY_MS 800
#define SENSOR_DISCONNECT_DELAY_MS 200

static const char *sensor_collect_demo_value(uint32_t item_mask)
{
    switch (item_mask) {
        case MEASUREMENT_ITEM_HEIGHT:
            return "160 cm";
        case MEASUREMENT_ITEM_WEIGHT:
            return "55 kg";
        case MEASUREMENT_ITEM_BP:
            return "120/80 mmHg";
        case MEASUREMENT_ITEM_GLUCOSE:
            return "5.2 mmol/L";
        default:
            return "";
    }
}

int sensor_collect_item(const sensor_collect_target_t *target, char *value, uint32_t value_len)
{
    const char *demo_value;

    if ((target == NULL) || (value == NULL) || (value_len == 0)) {
        return -1;
    }

    /*
     * TODO: replace this demo body with the real short-lived SLE sensor link:
     * scan target->target_name, connect, read one measurement, disconnect.
     */
    osal_printk("%s connect sensor:%s target:%s\r\n",
                SENSOR_COLLECT_LOG, target->name, target->target_name);
    osal_msleep(SENSOR_CONNECT_DELAY_MS);

    osal_printk("%s measuring:%s\r\n", SENSOR_COLLECT_LOG, target->name);
    osal_msleep(SENSOR_MEASURE_DELAY_MS);

    demo_value = sensor_collect_demo_value(target->item_mask);
    if (strncpy_s(value, value_len, demo_value, value_len - 1) != EOK) {
        return -1;
    }

    osal_printk("%s disconnect sensor:%s value:%s\r\n",
                SENSOR_COLLECT_LOG, target->name, value);
    osal_msleep(SENSOR_DISCONNECT_DELAY_MS);
    return 0;
}
