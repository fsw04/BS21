#ifndef UI_PAGE_HEALTH_H
#define UI_PAGE_HEALTH_H

#include <stdint.h>
#include <stdbool.h>
#include "errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 传感器数据项掩码（与 measurement_session.h 一致） */
#define HEALTH_ITEM_HEIGHT   (1U << 0)
#define HEALTH_ITEM_WEIGHT   (1U << 1)
#define HEALTH_ITEM_BP       (1U << 2)
#define HEALTH_ITEM_GLUCOSE  (1U << 3)

/**
 * @brief 初始化健康监测页面（LCD + 触控 + 清屏）
 */
errcode_t ui_health_init(void);

/**
 * @brief 设置 SLE 连接状态
 * @param connected  是否已连接
 * @param device_name 设备名称（如 "watch-01"），可为 NULL
 * @param rssi       信号强度 dBm，0 表示未知
 */
void ui_health_set_sle_status(bool connected, const char *device_name, int8_t rssi);

/**
 * @brief 设置身份信息
 * @param name      姓名（GB2312 编码），可为 NULL
 * @param id_number 身份证号，可为 NULL
 */
void ui_health_set_identity(const char *name, const char *id_number);

/**
 * @brief 设置传感器数值
 * @param item_mask  数据项掩码 (HEALTH_ITEM_*)
 * @param value      数值字符串（如 "120/80 mmHg"），可为 NULL 清除
 */
void ui_health_set_sensor_value(uint32_t item_mask, const char *value);

/**
 * @brief 设置 BMI 值
 * @param bmi  BMI 字符串（如 "21.5"），可为 NULL
 */
void ui_health_set_bmi(const char *bmi);

/**
 * @brief 标记整个页面需要重绘
 */
void ui_health_invalidate(void);

/**
 * @brief 刷新显示（在主循环中调用，仅脏区域重绘）
 */
void ui_health_refresh(void);

/**
 * @brief 触摸事件处理（在主循环中调用）
 */
void ui_health_touch_loop(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_PAGE_HEALTH_H */
