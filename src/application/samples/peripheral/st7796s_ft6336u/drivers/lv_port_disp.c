#include "lv_port_disp.h"
#include "st7796s.h"
#include "lcd_bus.h"
#include "soc_osal.h"

#define LV_DISP_BUF_LINES 20  /* 部分刷新：每次刷新 20 行 */

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * LV_DISP_BUF_LINES];

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;
    uint32_t total = (uint32_t)w * h;

    st7796s_set_window(area->x1, area->y1, area->x2, area->y2);
    lcd_bus_send_cmd(0x2C);

    /* 分块发送，避免单次 SPI 传输过长 */
    uint8_t *data = (uint8_t *)color_p;
    uint32_t byte_len = total * 2;
    uint32_t sent = 0;
    while (sent < byte_len) {
        uint32_t chunk = byte_len - sent;
        if (chunk > 512) chunk = 512;
        lcd_bus_send_data_array(data + sent, chunk);
        sent += chunk;
    }

    lv_disp_flush_ready(drv);
}

void lv_port_disp_init(void)
{
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * LV_DISP_BUF_LINES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
