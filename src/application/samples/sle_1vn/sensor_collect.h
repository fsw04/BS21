#ifndef SENSOR_COLLECT_H
#define SENSOR_COLLECT_H

#include <stdint.h>

#include "measurement_session.h"

typedef struct {
    uint32_t item_mask;
    const char *name;
    const char *target_name;
} sensor_collect_target_t;

int sensor_collect_item(const sensor_collect_target_t *target, char *value, uint32_t value_len);

#endif
