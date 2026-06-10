#include "ui_button.h"
#include "../drivers/st7796s.h"
#include "../drivers/lcd_bus.h"
#include "../drivers/ft6336u.h"
#include "soc_osal.h"

#define UI_RAMWR            0x2C
#define UI_CHUNK_PIXELS     256

/* RGB565 常用颜色 */
#define COL_BLACK   0x0000
#define COL_WHITE   0xFFFF
#define COL_RED     0xF800
#define COL_GREEN   0x07E0
#define COL_BLUE    0x001F
#define COL_YELLOW  0xFFE0
#define COL_GRAY    0x8410

static ui_button_t *g_btns[UI_MAX_BUTTONS] = {0};

/*============================================================
 * 基础图元：填充矩形（分块传输，避免栈溢出）
 *============================================================*/
static void ui_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (!w || !h) return;
    if (x >= ST7796S_WIDTH || y >= ST7796S_HEIGHT) return;
    if (x + w > ST7796S_WIDTH)  w = ST7796S_WIDTH - x;
    if (y + h > ST7796S_HEIGHT) h = ST7796S_HEIGHT - y;

    uint32_t total = (uint32_t)w * h;
    uint8_t  buf[UI_CHUNK_PIXELS * 2];

    for (uint16_t i = 0; i < UI_CHUNK_PIXELS; i++) {
        buf[i * 2]     = color >> 8;
        buf[i * 2 + 1] = color & 0xFF;
    }

    st7796s_set_window(x, y, x + w - 1, y + h - 1);
    lcd_bus_send_cmd(UI_RAMWR);

    while (total) {
        uint16_t chunk = (total > UI_CHUNK_PIXELS) ? UI_CHUNK_PIXELS : (uint16_t)total;
        lcd_bus_send_data_array(buf, chunk * 2);
        total -= chunk;
    }
}

/* 绘制边框 */
static void ui_draw_border(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t color, uint8_t thickness)
{
    ui_fill_rect(x, y, w, thickness, color);                    /* 上 */
    ui_fill_rect(x, y + h - thickness, w, thickness, color);    /* 下 */
    ui_fill_rect(x, y, thickness, h, color);                  /* 左 */
    ui_fill_rect(x + w - thickness, y, thickness, h, color);    /* 右 */
}

/*============================================================
 * 按钮管理
 *============================================================*/
errcode_t ui_init(void)
{
    errcode_t ret;

    ret = st7796s_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[UI] LCD init failed 0x%x\r\n", ret);
        return ret;
    }

    ret = ft6336u_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[UI] Touch init failed 0x%x\r\n", ret);
        return ret;
    }

    st7796s_fill_color(COL_BLACK);
    osal_printk("[UI] init ok, screen %dx%d\r\n", ST7796S_WIDTH, ST7796S_HEIGHT);
    return ERRCODE_SUCC;
}

void ui_button_create(uint8_t id, ui_button_t *btn)
{
    if (id < UI_MAX_BUTTONS) {
        g_btns[id] = btn;
    }
}

void ui_button_draw(uint8_t id)
{
    if (id >= UI_MAX_BUTTONS || g_btns[id] == NULL) return;

    ui_button_t *b = g_btns[id];
    uint16_t c = b->pressed ? b->color_pressed : b->color;

    /* 背景 */
    ui_fill_rect(b->x, b->y, b->width, b->height, c);

    /* 边框（若与背景色不同） */
    if (b->border_color != c) {
        ui_draw_border(b->x, b->y, b->width, b->height, b->border_color, 2);
    }

    /* TODO: 在此处调用字库渲染函数，将 b->label 绘制到按钮中心 */
}

void ui_button_draw_all(void)
{
    for (uint8_t i = 0; i < UI_MAX_BUTTONS; i++) {
        if (g_btns[i] != NULL) {
            ui_button_draw(i);
        }
    }
}

/*============================================================
 * 触摸处理（单次扫描，供任务循环调用）
 *============================================================*/
static uint8_t ui_hit_test(uint16_t x, uint16_t y, const ui_button_t *b)
{
    return (x >= b->x && x < b->x + b->width &&
            y >= b->y && y < b->y + b->height);
}

/* 坐标映射：若触摸屏与屏幕方向不一致，在此做镜像/旋转/缩放 */
static void ui_map_touch(uint16_t *x, uint16_t *y)
{
    /* 示例：若 X/Y 需要交换，或需要镜像，请修改此处 */
    /* uint16_t tmp = *x; *x = *y; *y = ST7796S_HEIGHT - 1 - tmp; */
    (void)x;
    (void)y;
}

void ui_touch_task_loop(void)
{
    ft6336u_point_t pts[FT6336U_MAX_POINTS];
    uint8_t btn_touched[UI_MAX_BUTTONS];

    uint8_t n = ft6336u_read_points(pts, FT6336U_MAX_POINTS);

    /* 清零本次扫描结果 */
    for (uint8_t i = 0; i < UI_MAX_BUTTONS; i++) {
        btn_touched[i] = 0;
    }

    /* 遍历所有触摸点，检测落在哪个按钮上 */
    for (uint8_t i = 0; i < n; i++) {
        uint16_t tx = pts[i].x;
        uint16_t ty = pts[i].y;
        ui_map_touch(&tx, &ty);

        for (uint8_t b = 0; b < UI_MAX_BUTTONS; b++) {
            if (g_btns[b] == NULL) continue;

            if (ui_hit_test(tx, ty, g_btns[b])) {
                btn_touched[b] = 1;

                /* 首次按下：切换为按下态并重绘 */
                if (!g_btns[b]->pressed) {
                    g_btns[b]->pressed = 1;
                    ui_button_draw(b);
                }
            }
        }
    }

    /* 检测释放：本次未被触摸，但之前处于按下态 */
    for (uint8_t b = 0; b < UI_MAX_BUTTONS; b++) {
        if (g_btns[b] == NULL) continue;

        if (!btn_touched[b] && g_btns[b]->pressed) {
            g_btns[b]->pressed = 0;
            ui_button_draw(b);          /* 恢复为正常颜色 */

            /* 触发点击回调（释放时触发，防误触） */
            if (g_btns[b]->on_click != NULL) {
                g_btns[b]->on_click();
            }
        }
    }
}