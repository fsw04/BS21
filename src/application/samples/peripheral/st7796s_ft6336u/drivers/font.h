#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stdbool.h>
#include "errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ASCII 字体尺寸 */
#define FONT_ASCII_W       8
#define FONT_ASCII_H      16

/* HZK16 中文字体尺寸 */
#define FONT_HZK16_W      16
#define FONT_HZK16_H      16

/* 缩放倍数 */
#define FONT_SCALE_1X      1
#define FONT_SCALE_2X      2

/* GB2312 中文字符串常量（hex 转义，避免编码问题） */
#define FONT_STR_BP        "\xD1\xAA\xD1\xB9"          /* 血压 */
#define FONT_STR_BS        "\xD1\xAA\xCC\xC7"          /* 血糖 */
#define FONT_STR_HT        "\xC9\xED\xB8\xDF"          /* 身高 */
#define FONT_STR_WT        "\xCC\xE5\xD6\xD8"          /* 体重 */
#define FONT_STR_BMI       "BMI"
#define FONT_STR_CONNECTED "\xD2\xD1\xC1\xAC\xBD\xD3"  /* 已连接 */
#define FONT_STR_DISCONNECTED "\xCE\xB4\xC1\xAC\xBD\xD3" /* 未连接 */
#define FONT_STR_DEVICE    "\xC9\xE8\xB1\xB8"          /* 设备 */
#define FONT_STR_MEASURE   "\xB2\xE2\xC1\xBF"          /* 测量 */
#define FONT_STR_UPLOAD    "\xC9\xCF\xB4\xAB"          /* 上传 */

/**
 * @brief 初始化字体系统，注册 HZK16 字库数据指针
 * @param hzk16_data  HZK16 字库数据指针（可为 NULL，此时中文显示为方块）
 * @param hzk16_size  HZK16 字库数据长度
 */
void font_init(const uint8_t *hzk16_data, uint32_t hzk16_size);

/**
 * @brief 绘制单个 ASCII 字符
 * @param x       左上角 X 坐标
 * @param y       左上角 Y 坐标
 * @param ch      ASCII 字符 (0x20-0x7E)
 * @param fg      前景色 (RGB565)
 * @param bg      背景色 (RGB565)，若与 fg 相同则透明
 * @param scale   缩放倍数 (1 或 2)
 */
void font_draw_ascii(uint16_t x, uint16_t y, char ch,
                     uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 绘制单个 GB2312 中文字符
 * @param x       左上角 X 坐标
 * @param y       左上角 Y 坐标
 * @param gb_hi   GB2312 高字节
 * @param gb_lo   GB2312 低字节
 * @param fg      前景色 (RGB565)
 * @param bg      背景色 (RGB565)，若与 fg 相同则透明
 * @param scale   缩放倍数 (1 或 2)
 */
void font_draw_gb2312(uint16_t x, uint16_t y, uint8_t gb_hi, uint8_t gb_lo,
                      uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 绘制混合字符串（ASCII + GB2312 中文）
 * @param x       左上角 X 坐标
 * @param y       左上角 Y 坐标
 * @param str     字符串（ASCII 单字节，中文 GB2312 双字节）
 * @param fg      前景色 (RGB565)
 * @param bg      背景色 (RGB565)，若与 fg 相同则透明
 * @param scale   缩放倍数 (1 或 2)
 * @return        绘制结束后的 X 坐标
 */
uint16_t font_draw_string(uint16_t x, uint16_t y, const char *str,
                          uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 计算字符串像素宽度
 * @param str     字符串
 * @param scale   缩放倍数
 * @return        像素宽度
 */
uint16_t font_string_width(const char *str, uint8_t scale);

/**
 * @brief 获取当前字体行高
 * @param scale   缩放倍数
 * @return        像素高度
 */
uint16_t font_get_line_height(uint8_t scale);

/**
 * @brief HZK16 字库是否可用
 * @return true 可用 / false 不可用（中文将显示为方块）
 */
bool font_hzk16_available(void);

#ifdef __cplusplus
}
#endif

#endif /* FONT_H */
