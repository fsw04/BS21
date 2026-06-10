#ifndef UI_HEALTH_LVGL_H
#define UI_HEALTH_LVGL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_health_create(void);

void ui_health_set_sle_status(bool connected, const char *device_name, int8_t rssi);
void ui_health_set_identity(const char *name, const char *id_number);
void ui_health_set_sensor_value(uint32_t item_mask, const char *value);
void ui_health_set_bmi(const char *bmi);

#ifdef __cplusplus
}
#endif

#endif
