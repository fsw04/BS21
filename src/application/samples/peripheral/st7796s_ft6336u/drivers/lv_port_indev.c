#include "lv_port_indev.h"
#include "ft6336u.h"
#include "soc_osal.h"

static lv_indev_drv_t indev_drv;
static lv_indev_t *indev_touchpad;

static void touchpad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    ft6336u_point_t pt;
    uint8_t n = ft6336u_read_points(&pt, 1);

    if (n > 0) {
        /* 横屏坐标映射：FT6336U 输出竖屏坐标，需旋转 */
        data->point.x = pt.y;
        data->point.y = 320 - 1 - pt.x;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lv_port_indev_init(void)
{
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read_cb;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}
