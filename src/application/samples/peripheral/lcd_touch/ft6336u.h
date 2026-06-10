/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: FT6336U Touch Driver Header. \n
 *
 * History: \n
 * 2024-01-01, Create file. \n
 */
#ifndef FT6336U_H
#define FT6336U_H

#include <stdint.h>
#include <stdbool.h>
#include "errcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* FT6336U I2C设备地址 */
#define FT6336U_I2C_ADDR        0x38

/* FT6336U 寄存器地址 */
#define FT6336U_REG_DEV_MODE    0x00    /* 设备模式 */
#define FT6336U_REG_GEST_ID     0x01    /* 手势ID */
#define FT6336U_REG_TD_STATUS   0x02    /* 触摸状态 */
#define FT6336U_REG_TOUCH1_XH   0x03    /* 触摸1 X高4位+事件 */
#define FT6336U_REG_TOUCH1_XL   0x04    /* 触摸1 X低8位 */
#define FT6336U_REG_TOUCH1_YH   0x05    /* 触摸1 Y高4位+触摸ID */
#define FT6336U_REG_TOUCH1_YL   0x06    /* 触摸1 Y低8位 */
#define FT6336U_REG_TOUCH1_WEIGHT 0x07  /* 触摸1重量 */
#define FT6336U_REG_TOUCH1_AREA 0x08    /* 触摸1面积 */
#define FT6336U_REG_TOUCH2_XH   0x09    /* 触摸2 X高4位+事件 */
#define FT6336U_REG_TOUCH2_XL   0x0A    /* 触摸2 X低8位 */
#define FT6336U_REG_TOUCH2_YH   0x0B    /* 触摸2 Y高4位+触摸ID */
#define FT6336U_REG_TOUCH2_YL   0x0C    /* 触摸2 Y低8位 */
#define FT6336U_REG_TOUCH2_WEIGHT 0x0D  /* 触摸2重量 */
#define FT6336U_REG_TOUCH2_AREA 0x0E    /* 触摸2面积 */
#define FT6336U_REG_THRESHHOLD 0x80     /* 触摸阈值 */
#define FT6336U_REG_PERIOD_ACTIVE 0x88  /* 活跃模式扫描周期 */
#define FT6336U_REG_PERIOD_MONITOR 0x89 /* 监视模式扫描周期 */
#define FT6336U_REG_RADIAN_VALUE 0x91   /* 手势角度值 */
#define FT6336U_REG_OFFSET_LEFT_RIGHT 0x92  /* 左右偏移 */
#define FT6336U_REG_OFFSET_UP_DOWN 0x93     /* 上下偏移 */
#define FT6336U_REG_DISTANCE_LEFT_RIGHT 0x94  /* 左右距离 */
#define FT6336U_REG_DISTANCE_UP_DOWN 0x95      /* 上下距离 */
#define FT6336U_REG_DISTANCE_ZOOM 0x96         /* 缩放距离 */
#define FT6336U_REG_LIB_VERSION_H 0xA1  /* 库版本高字节 */
#define FT6336U_REG_LIB_VERSION_L 0xA2  /* 库版本低字节 */
#define FT6336U_REG_CHIP_ID_H    0xA3   /* 芯片ID高字节 */
#define FT6336U_REG_CHIP_ID_L    0xA4   /* 芯片ID低字节 */
#define FT6336U_REG_FIRM_VERS    0xA6   /* 固件版本 */
#define FT6336U_REG_VENDOR1_ID   0xA8   /* 供应商ID1 */
#define FT6336U_REG_CIPHER       0xA9   /* 加密 */
#define FT6336U_REG_VENDOR2_ID   0xAA   /* 供应商ID2 */
#define FT6336U_REG_RELEASE_CODE 0xAF   /* 发布代码 */
#define FT6336U_REG_STATE        0xBC   /* 设备状态 */
#define FT6336U_REG_G_MODE       0xA4   /* 中断模式 */

/* 触摸事件类型 */
#define FT6336U_TOUCH_EVENT_DOWN    0x00
#define FT6336U_TOUCH_EVENT_UP      0x01
#define FT6336U_TOUCH_EVENT_CONTACT 0x02

/* 最多支持的触摸点数 */
#define FT6336U_MAX_TOUCHES     2

/**
 * @brief 触摸点信息
 */
typedef struct {
    uint16_t x;         /*!< X坐标 */
    uint16_t y;         /*!< Y坐标 */
    uint8_t event;      /*!< 触摸事件 */
    uint8_t weight;     /*!< 触摸重量 */
    uint8_t area;       /*!< 触摸面积 */
    uint8_t id;         /*!< 触摸ID */
} ft6336u_touch_t;

/**
 * @brief 初始化FT6336U触摸驱动
 * @return 0成功，其他失败
 */
errcode_t ft6336u_init(void);

/**
 * @brief 反初始化FT6336U触摸驱动
 */
void ft6336u_deinit(void);

/**
 * @brief 读取触摸点数
 * @return 当前触摸点数(0-2)
 */
uint8_t ft6336u_get_touch_count(void);

/**
 * @brief 读取触摸点信息
 * @param touches 触摸点数组(至少FT6336U_MAX_TOUCHES个元素)
 * @param count 返回触摸点数
 * @return 0成功，其他失败
 */
errcode_t ft6336u_read_touches(ft6336u_touch_t *touches, uint8_t *count);

/**
 * @brief 读取单个寄存器
 * @param reg 寄存器地址
 * @param val 返回的值
 * @return 0成功，其他失败
 */
errcode_t ft6336u_read_reg(uint8_t reg, uint8_t *val);

/**
 * @brief 写入单个寄存器
 * @param reg 寄存器地址
 * @param val 写入的值
 * @return 0成功，其他失败
 */
errcode_t ft6336u_write_reg(uint8_t reg, uint8_t val);

/**
 * @brief 读取多个寄存器
 * @param reg 起始寄存器地址
 * @param buf 数据缓冲区
 * @param len 读取长度
 * @return 0成功，其他失败
 */
errcode_t ft6336u_read_regs(uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * @brief 获取芯片ID
 * @return 芯片ID
 */
uint16_t ft6336u_get_chip_id(void);

/**
 * @brief 获取固件版本
 * @return 固件版本
 */
uint8_t ft6336u_get_firmware_version(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* FT6336U_H */
