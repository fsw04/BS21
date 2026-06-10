/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
 *
 * Description: ST7796S LCD Driver Header. \n
 *
 * History: \n
 * 2024-01-01, Create file. \n
 */
#ifndef ST7796S_H
#define ST7796S_H

#include <stdint.h>
#include <stdbool.h>
#include "errcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* ST7796S 屏幕分辨率 */
#define ST7796S_WIDTH       320
#define ST7796S_HEIGHT      480

/* ST7796S 颜色定义 (RGB565) */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_ORANGE        0xFD20
#define COLOR_GRAY          0x8410
#define COLOR_DARKGRAY      0x4208
#define COLOR_LIGHTGRAY     0xC618

/* ST7796S 命令定义 */
#define ST7796S_NOP         0x00
#define ST7796S_SWRESET     0x01
#define ST7796S_SLPIN       0x10
#define ST7796S_SLPOUT      0x11
#define ST7796S_PTLON       0x12
#define ST7796S_NORON       0x13
#define ST7796S_RDMODE      0x0A
#define ST7796S_RDMADCTL    0x0B
#define ST7796S_RDPIXFMT   0x0C
#define ST7796S_RDIMGFMT   0x0D
#define ST7796S_RDSELFDIAG 0x0F
#define ST7796S_INVOFF      0x20
#define ST7796S_INVON       0x21
#define ST7796S_GAMSET      0x26
#define ST7796S_DISPOFF     0x28
#define ST7796S_DISPON      0x29
#define ST7796S_CASET       0x2A
#define ST7796S_PASET       0x2B
#define ST7796S_RAMWR       0x2C
#define ST7796S_RAMRD       0x2E
#define ST7796S_MADCTL      0x36
#define ST7796S_PIXFMT      0x3A
#define ST7796S_FRMCTR1     0xB1
#define ST7796S_FRMCTR2     0xB2
#define ST7796S_FRMCTR3     0xB3
#define ST7796S_INVCTR      0xB4
#define ST7796S_DFUNCTR     0xB6
#define ST7796S_PWCTR1      0xC0
#define ST7796S_PWCTR2      0xC1
#define ST7796S_PWCTR3      0xC2
#define ST7796S_PWCTR4      0xC3
#define ST7796S_PWCTR5      0xC4
#define ST7796S_VMCTR1      0xC5
#define ST7796S_VMOFCTR     0xC7
#define ST7796S_RDID1       0xDA
#define ST7796S_RDID2       0xDB
#define ST7796S_RDID3       0xDC
#define ST7796S_RDID4       0xDD
#define ST7796S_PWCTR6      0xFC
#define ST7796S_GMCTRP1     0xE0
#define ST7796S_GMCTRN1     0xE1

/**
 * @brief 初始化ST7796S LCD驱动
 * @return 0成功，其他失败
 */
errcode_t st7796s_init(void);

/**
 * @brief 反初始化ST7796S LCD驱动
 */
void st7796s_deinit(void);

/**
 * @brief 设置显示窗口区域
 * @param x1 起始X坐标
 * @param y1 起始Y坐标
 * @param x2 结束X坐标
 * @param y2 结束Y坐标
 */
void st7796s_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief 清屏
 * @param color 填充颜色(RGB565)
 */
void st7796s_clear(uint16_t color);

/**
 * @brief 填充指定区域
 * @param x1 起始X坐标
 * @param y1 起始Y坐标
 * @param x2 结束X坐标
 * @param y2 结束Y坐标
 * @param color 填充颜色(RGB565)
 */
void st7796s_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 画一个点
 * @param x X坐标
 * @param y Y坐标
 * @param color 颜色(RGB565)
 */
void st7796s_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief 画线
 * @param x1 起始X
 * @param y1 起始Y
 * @param x2 结束X
 * @param y2 结束Y
 * @param color 颜色(RGB565)
 */
void st7796s_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 画矩形(仅边框)
 * @param x1 起始X
 * @param y1 起始Y
 * @param x2 结束X
 * @param y2 结束Y
 * @param color 颜色(RGB565)
 */
void st7796s_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 画填充矩形
 * @param x1 起始X
 * @param y1 起始Y
 * @param x2 结束X
 * @param y2 结束Y
 * @param color 颜色(RGB565)
 */
void st7796s_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 在指定位置绘制8x16 ASCII字符
 * @param x X坐标
 * @param y Y坐标
 * @param ch 字符
 * @param fc 前景色(RGB565)
 * @param bc 背景色(RGB565)
 * @param size 字体大小(1=8x16, 2=16x32)
 */
void st7796s_draw_char(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc, uint8_t size);

/**
 * @brief 在指定位置绘制字符串
 * @param x X坐标
 * @param y Y坐标
 * @param str 字符串
 * @param fc 前景色(RGB565)
 * @param bc 背景色(RGB565)
 * @param size 字体大小(1=8x16, 2=16x32)
 */
void st7796s_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc, uint8_t size);

/**
 * @brief 设置背光
 * @param on true开启，false关闭
 */
void st7796s_set_backlight(bool on);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* ST7796S_H */
