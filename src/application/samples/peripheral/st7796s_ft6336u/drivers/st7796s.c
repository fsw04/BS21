#include "st7796s.h"
#include "lcd_bus.h"
#include "soc_osal.h"

#define ST7796S_SLPOUT  0x11
#define ST7796S_DISPON  0x29
#define ST7796S_CASET   0x2A
#define ST7796S_RASET   0x2B
#define ST7796S_RAMWR   0x2C
#define ST7796S_MADCTL  0x36
#define ST7796S_COLMOD  0x3A

#define ST7796S_HIGH_BYTE_SHIFT 8
#define ST7796S_LOW_BYTE_MASK   0xFF
#define ST7796S_FILL_CHUNK_PIXELS 256

static errcode_t st7796s_write_cmd_data(uint8_t cmd, const uint8_t *data, uint32_t len)
{
    errcode_t ret = lcd_bus_send_cmd(cmd);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    return lcd_bus_send_data_array(data, len);
}

static errcode_t st7796s_write_cmd_byte(uint8_t cmd, uint8_t data)
{
    errcode_t ret = lcd_bus_send_cmd(cmd);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    return lcd_bus_send_data(data);
}

errcode_t st7796s_init(void)
{
    /* ST7796S 标准初始化参数 */
    static const uint8_t frame_rate[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    static const uint8_t power_ctrl1[] = {0xA4, 0xA1};
    static const uint8_t positive_gamma[] = {
        0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31,
        0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34
    };
    static const uint8_t negative_gamma[] = {
        0xD0, 0x08, 0x0E, 0x09, 0x09, 0x15, 0x31,
        0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34
    };
    errcode_t ret;

    ret = lcd_bus_init();
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    lcd_bus_reset();
    osal_msleep(120);

    /* Sleep Out */
    ret = lcd_bus_send_cmd(ST7796S_SLPOUT);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(120);

    /* ========== ST7796S 标准初始化序列 ========== */
    ret = st7796s_write_cmd_byte(ST7796S_COLMOD, 0x05);        /* 像素格式: 16bpp RGB565 */
    ret |= st7796s_write_cmd_data(0xB2, frame_rate, sizeof(frame_rate));  /* 帧率控制 */
    ret |= st7796s_write_cmd_byte(0xB7, 0x35);                 /* 门控制 */
    ret |= st7796s_write_cmd_byte(0xBB, 0x32);                 /* VCOM 设置 */
    ret |= st7796s_write_cmd_byte(0xC2, 0x01);                 /* 电源控制 3 */
    ret |= st7796s_write_cmd_byte(0xC3, 0x15);                 /* 电源控制 4 (VCOMH) */
    ret |= st7796s_write_cmd_byte(0xC4, 0x20);                 /* 电源控制 5 (VCOML) */
    ret |= st7796s_write_cmd_byte(0xC6, 0x0F);                 /* VCOM 偏移控制 */
    ret |= st7796s_write_cmd_data(0xD0, power_ctrl1, sizeof(power_ctrl1)); /* 电源控制 1 */
    ret |= st7796s_write_cmd_data(0xE0, positive_gamma, sizeof(positive_gamma)); /* 正伽马 */
    ret |= st7796s_write_cmd_data(0xE1, negative_gamma, sizeof(negative_gamma)); /* 负伽马 */
    ret |= st7796s_write_cmd_byte(ST7796S_MADCTL, CONFIG_ST7796S_MADCTL); /* 扫描方向: 0x48 */

    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    osal_msleep(120);

    /* Display ON */
    ret = lcd_bus_send_cmd(ST7796S_DISPON);
    if (ret == ERRCODE_SUCC) {
        lcd_bus_backlight_set(1);
    }

    return ret;
}

errcode_t st7796s_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t col[] = {
        x0 >> ST7796S_HIGH_BYTE_SHIFT,
        x0 & ST7796S_LOW_BYTE_MASK,
        x1 >> ST7796S_HIGH_BYTE_SHIFT,
        x1 & ST7796S_LOW_BYTE_MASK,
    };
    uint8_t row[] = {
        y0 >> ST7796S_HIGH_BYTE_SHIFT,
        y0 & ST7796S_LOW_BYTE_MASK,
        y1 >> ST7796S_HIGH_BYTE_SHIFT,
        y1 & ST7796S_LOW_BYTE_MASK,
    };

    errcode_t ret = st7796s_write_cmd_data(ST7796S_CASET, col, sizeof(col));
    ret |= st7796s_write_cmd_data(ST7796S_RASET, row, sizeof(row));
    return ret;
}

errcode_t st7796s_write_pixels(const uint8_t *data, uint32_t len)
{
    errcode_t ret = lcd_bus_send_cmd(ST7796S_RAMWR);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    return lcd_bus_send_data_array(data, len);
}

errcode_t st7796s_fill_color(uint16_t color)
{
    uint8_t line[ST7796S_FILL_CHUNK_PIXELS * 2];
    uint32_t pixels_left = ST7796S_WIDTH * ST7796S_HEIGHT;
    uint32_t chunk_pixels;
    errcode_t ret;

    for (uint32_t i = 0; i < ST7796S_FILL_CHUNK_PIXELS; i++) {
        line[(i * 2)]     = color >> ST7796S_HIGH_BYTE_SHIFT;
        line[(i * 2) + 1] = color & ST7796S_LOW_BYTE_MASK;
    }

    ret = st7796s_set_window(0, 0, ST7796S_WIDTH - 1, ST7796S_HEIGHT - 1);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = lcd_bus_send_cmd(ST7796S_RAMWR);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    while (pixels_left > 0) {
        chunk_pixels = (pixels_left > ST7796S_FILL_CHUNK_PIXELS) ? ST7796S_FILL_CHUNK_PIXELS : pixels_left;
        ret = lcd_bus_send_data_array(line, chunk_pixels * 2);
        if (ret != ERRCODE_SUCC) {
            return ret;
        }
        pixels_left -= chunk_pixels;
    }

    return ERRCODE_SUCC;
}