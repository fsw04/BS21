#include "ui_page_health.h"
#include "../drivers/st7796s.h"
#include "../drivers/lcd_bus.h"
#include "../drivers/font.h"
#include "../drivers/ft6336u.h"
#include "soc_osal.h"
#include "securec.h"

/*============================================================
 * 颜色定义 (RGB565)
 *============================================================*/
#define COL_BG          0x0000   /* 黑色背景 */
#define COL_CARD_BG     0x18E3   /* 深蓝灰卡片背景 */
#define COL_CARD_BG2    0x2104   /* 深色卡片背景 */
#define COL_FG          0xFFFF   /* 白色文字 */
#define COL_FG_DIM      0xB5B6   /* 暗白色次要文字 */
#define COL_ACCENT      0x04FF   /* 青色强调 */
#define COL_RED         0xF800
#define COL_GREEN       0x07E0
#define COL_YELLOW      0xFFE0
#define COL_ORANGE      0xFD20
#define COL_BORDER      0x3186   /* 边框色 */
#define COL_BP_COLOR    0xF977   /* 血压卡片强调色（粉红） */
#define COL_BS_COLOR    0xB5F6   /* 血糖卡片强调色（浅蓝） */
#define COL_HT_COLOR    0x07EF   /* 身高卡片强调色（浅绿） */
#define COL_WT_COLOR    0xFEA0   /* 体重卡片强调色（浅橙） */

/*============================================================
 * 布局常量 (320×480 屏幕)
 *============================================================*/
#define LAYOUT_MARGIN        8
#define LAYOUT_CARD_RADIUS   4

/* 左栏 (0 ~ 199) */
#define LEFT_X              8
#define LEFT_W              192
/* 右栏 (200 ~ 319) */
#define RIGHT_X             204
#define RIGHT_W             108

/* 身份卡片 */
#define ID_CARD_Y           8
#define ID_CARD_H           80

/* 血压卡片 */
#define BP_CARD_Y           96
#define BP_CARD_H           64

/* 数据小卡片 (2×2 网格) */
#define DATA_GRID_Y         168
#define DATA_CARD_W         92
#define DATA_CARD_H         80
#define DATA_GRID_GAP       8

/* SLE 状态卡片 */
#define SLE_CARD_Y          8
#define SLE_CARD_H          100

/* 预留区域 */
#define RESERVED_Y          116

/*============================================================
 * 页面状态
 *============================================================*/
typedef struct {
    bool     sle_connected;
    char     sle_device[20];
    int8_t   sle_rssi;
    char     identity_name[32];
    char     identity_id[20];
    char     bp_value[32];
    char     bs_value[32];
    char     ht_value[32];
    char     wt_value[32];
    char     bmi_value[16];
    bool     dirty;
} health_state_t;

static health_state_t g_health = {0};

/*============================================================
 * 基础绘图：填充矩形
 *============================================================*/
static void health_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (!w || !h) return;
    if (x >= ST7796S_WIDTH || y >= ST7796S_HEIGHT) return;
    if (x + w > ST7796S_WIDTH)  w = ST7796S_WIDTH - x;
    if (y + h > ST7796S_HEIGHT) h = ST7796S_HEIGHT - y;

    uint32_t total = (uint32_t)w * h;
    uint8_t buf[256 * 2];

    for (uint16_t i = 0; i < 256; i++) {
        buf[i * 2]     = color >> 8;
        buf[i * 2 + 1] = color & 0xFF;
    }

    st7796s_set_window(x, y, x + w - 1, y + h - 1);
    lcd_bus_send_cmd(0x2C);

    while (total) {
        uint16_t chunk = (total > 256) ? 256 : (uint16_t)total;
        lcd_bus_send_data_array(buf, chunk * 2);
        total -= chunk;
    }
}

/*============================================================
 * 基础绘图：圆角矩形（简化版，仅填充内部 + 四角圆弧近似）
 *============================================================*/
static void health_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                   uint16_t r, uint16_t color)
{
    /* 简化：先填充整个矩形，再在四角画背景色方块模拟圆角 */
    health_fill_rect(x, y, w, h, color);

    if (r > 0) {
        /* 四角用背景色覆盖，模拟圆角 */
        health_fill_rect(x, y, r, r, COL_BG);
        health_fill_rect(x + w - r, y, r, r, COL_BG);
        health_fill_rect(x, y + h - r, r, r, COL_BG);
        health_fill_rect(x + w - r, y + h - r, r, r, COL_BG);
    }
}

/*============================================================
 * 基础绘图：边框
 *============================================================*/
static void health_draw_border(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                               uint16_t color, uint8_t thickness)
{
    health_fill_rect(x, y, w, thickness, color);
    health_fill_rect(x, y + h - thickness, w, thickness, color);
    health_fill_rect(x, y, thickness, h, color);
    health_fill_rect(x + w - thickness, y, thickness, h, color);
}

/*============================================================
 * 绘制 RSSI 信号条
 *============================================================*/
static void health_draw_rssi_bars(uint16_t x, uint16_t y, int8_t rssi)
{
    /* 4 根信号条，高度递增 */
    uint8_t bars[4] = {4, 8, 12, 16};
    uint8_t active;

    if (rssi >= -40) {
        active = 4;
    } else if (rssi >= -55) {
        active = 3;
    } else if (rssi >= -70) {
        active = 2;
    } else if (rssi != 0) {
        active = 1;
    } else {
        active = 0;
    }

    for (uint8_t i = 0; i < 4; i++) {
        uint16_t color = (i < active) ? COL_ACCENT : 0x4208;
        uint16_t bar_h = bars[i];
        health_fill_rect(x + i * 6, y + 16 - bar_h, 4, bar_h, color);
    }
}

/*============================================================
 * 绘制连接状态指示点
 *============================================================*/
static void health_draw_status_dot(uint16_t x, uint16_t y, bool connected)
{
    uint16_t color = connected ? COL_GREEN : COL_RED;
    health_fill_rect(x, y, 6, 6, color);
}

/*============================================================
 * 绘制图标占位（简化版：用小矩形/色块代替）
 *============================================================*/
static void health_draw_icon_person(uint16_t x, uint16_t y, uint16_t color)
{
    /* 简化人形图标：头(圆) + 身体(矩形) */
    health_fill_rect(x + 2, y, 8, 8, color);     /* 头 */
    health_fill_rect(x, y + 10, 12, 10, color);   /* 身体 */
}

static void health_draw_icon_blood(uint16_t x, uint16_t y, uint16_t color)
{
    /* 简化血滴图标 */
    health_fill_rect(x + 3, y, 6, 4, color);
    health_fill_rect(x + 2, y + 4, 8, 6, color);
    health_fill_rect(x + 3, y + 10, 6, 4, color);
    health_fill_rect(x + 4, y + 14, 4, 2, color);
}

/*============================================================
 * 绘制身份信息卡片（左上）
 *============================================================*/
static void health_draw_identity_card(void)
{
    uint16_t x = LEFT_X;
    uint16_t y = ID_CARD_Y;
    uint16_t w = LEFT_W;
    uint16_t h = ID_CARD_H;

    health_fill_round_rect(x, y, w, h, LAYOUT_CARD_RADIUS, COL_CARD_BG);
    health_draw_border(x, y, w, h, COL_BORDER, 1);

    /* 人形图标 */
    health_draw_icon_person(x + 8, y + 8, COL_ACCENT);

    /* 姓名 */
    const char *name = g_health.identity_name[0] ? g_health.identity_name : "---";
    font_draw_string(x + 28, y + 8, name, COL_FG, COL_CARD_BG, FONT_SCALE_1X);

    /* 身份证号 */
    const char *id = g_health.identity_id[0] ? g_health.identity_id : "---";
    font_draw_string(x + 28, y + 28, id, COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);

    /* 分隔线 */
    health_fill_rect(x + 8, y + 48, w - 16, 1, COL_BORDER);

    /* 底部提示 */
    if (g_health.identity_name[0]) {
        font_draw_string(x + 8, y + 56, FONT_STR_DEVICE, COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);
        font_draw_string(x + 8 + font_string_width(FONT_STR_DEVICE, FONT_SCALE_1X) + 8,
                         y + 56, ": OK", COL_GREEN, COL_CARD_BG, FONT_SCALE_1X);
    } else {
        font_draw_string(x + 8, y + 56, "waiting...", COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);
    }
}

/*============================================================
 * 绘制血压卡片（左中）
 *============================================================*/
static void health_draw_bp_card(void)
{
    uint16_t x = LEFT_X;
    uint16_t y = BP_CARD_Y;
    uint16_t w = LEFT_W;
    uint16_t h = BP_CARD_H;

    health_fill_round_rect(x, y, w, h, LAYOUT_CARD_RADIUS, COL_CARD_BG2);
    health_draw_border(x, y, w, h, COL_BP_COLOR, 1);

    /* 血滴图标 + 标签 */
    health_draw_icon_blood(x + 8, y + 8, COL_BP_COLOR);
    font_draw_string(x + 24, y + 8, FONT_STR_BP, COL_BP_COLOR, COL_CARD_BG2, FONT_SCALE_1X);

    /* 血压数值（大号） */
    const char *val = g_health.bp_value[0] ? g_health.bp_value : "---";
    font_draw_string(x + 8, y + 32, val, COL_FG, COL_CARD_BG2, FONT_SCALE_2X);
}

/*============================================================
 * 绘制数据小卡片（2×2 网格：血糖、身高、体重、BMI）
 *============================================================*/
static void health_draw_data_card(uint16_t x, uint16_t y, const char *label,
                                  const char *value, uint16_t accent_color)
{
    uint16_t w = DATA_CARD_W;
    uint16_t h = DATA_CARD_H;

    health_fill_round_rect(x, y, w, h, LAYOUT_CARD_RADIUS, COL_CARD_BG);
    health_draw_border(x, y, w, h, accent_color, 1);

    /* 顶部强调条 */
    health_fill_rect(x + 1, y + 1, w - 2, 3, accent_color);

    /* 标签 */
    font_draw_string(x + 6, y + 8, label, COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);

    /* 数值（大号） */
    const char *val = (value && value[0]) ? value : "--";
    font_draw_string(x + 6, y + 32, val, COL_FG, COL_CARD_BG, FONT_SCALE_2X);
}

static void health_draw_data_grid(void)
{
    uint16_t base_y = DATA_GRID_Y;
    uint16_t col1_x = LEFT_X;
    uint16_t col2_x = LEFT_X + DATA_CARD_W + DATA_GRID_GAP;

    /* 第一行：血糖、身高 */
    health_draw_data_card(col1_x, base_y, FONT_STR_BS,
                          g_health.bs_value[0] ? g_health.bs_value : NULL, COL_BS_COLOR);
    health_draw_data_card(col2_x, base_y, FONT_STR_HT,
                          g_health.ht_value[0] ? g_health.ht_value : NULL, COL_HT_COLOR);

    /* 第二行：体重、BMI */
    uint16_t row2_y = base_y + DATA_CARD_H + DATA_GRID_GAP;
    health_draw_data_card(col1_x, row2_y, FONT_STR_WT,
                          g_health.wt_value[0] ? g_health.wt_value : NULL, COL_WT_COLOR);
    health_draw_data_card(col2_x, row2_y, FONT_STR_BMI,
                          g_health.bmi_value[0] ? g_health.bmi_value : NULL, COL_ACCENT);
}

/*============================================================
 * 绘制 SLE 状态卡片（右上）
 *============================================================*/
static void health_draw_sle_card(void)
{
    uint16_t x = RIGHT_X;
    uint16_t y = SLE_CARD_Y;
    uint16_t w = RIGHT_W;
    uint16_t h = SLE_CARD_H;

    health_fill_round_rect(x, y, w, h, LAYOUT_CARD_RADIUS, COL_CARD_BG);
    health_draw_border(x, y, w, h, COL_BORDER, 1);

    /* 连接状态 */
    health_draw_status_dot(x + 8, y + 8, g_health.sle_connected);
    const char *status_str = g_health.sle_connected ? FONT_STR_CONNECTED : FONT_STR_DISCONNECTED;
    font_draw_string(x + 20, y + 6, status_str,
                     g_health.sle_connected ? COL_GREEN : COL_RED, COL_CARD_BG, FONT_SCALE_1X);

    /* 设备名称 */
    font_draw_string(x + 8, y + 28, FONT_STR_DEVICE, COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);
    const char *dev = g_health.sle_device[0] ? g_health.sle_device : "---";
    font_draw_string(x + 8, y + 44, dev, COL_FG, COL_CARD_BG, FONT_SCALE_1X);

    /* RSSI 信号条 */
    if (g_health.sle_rssi != 0) {
        health_draw_rssi_bars(x + 8, y + 68, g_health.sle_rssi);
        char rssi_str[12];
        (void)sprintf_s(rssi_str, sizeof(rssi_str), "%ddBm", (int)g_health.sle_rssi);
        font_draw_string(x + 36, y + 72, rssi_str, COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);
    } else {
        font_draw_string(x + 8, y + 72, "RSSI: ---", COL_FG_DIM, COL_CARD_BG, FONT_SCALE_1X);
    }
}

/*============================================================
 * 绘制预留区域（右下）
 *============================================================*/
static void health_draw_reserved(void)
{
    uint16_t x = RIGHT_X;
    uint16_t y = RESERVED_Y;
    uint16_t w = RIGHT_W;
    uint16_t h = 480 - y - LAYOUT_MARGIN;

    health_fill_round_rect(x, y, w, h, LAYOUT_CARD_RADIUS, COL_CARD_BG);
    health_draw_border(x, y, w, h, COL_BORDER, 1);
}

/*============================================================
 * 公共 API
 *============================================================*/
errcode_t ui_health_init(void)
{
    errcode_t ret;

    ret = st7796s_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[health] LCD init failed 0x%x\r\n", ret);
        return ret;
    }

    ret = ft6336u_init();
    if (ret != ERRCODE_SUCC) {
        osal_printk("[health] Touch init failed 0x%x\r\n", ret);
        return ret;
    }

    /* 清屏 */
    st7796s_fill_color(COL_BG);

    /* 初始化状态 */
    (void)memset_s(&g_health, sizeof(g_health), 0, sizeof(g_health));
    g_health.dirty = true;

    osal_printk("[health] init ok\r\n");
    return ERRCODE_SUCC;
}

void ui_health_set_sle_status(bool connected, const char *device_name, int8_t rssi)
{
    g_health.sle_connected = connected;
    g_health.sle_rssi = rssi;

    if (device_name != NULL) {
        (void)strncpy_s(g_health.sle_device, sizeof(g_health.sle_device),
                        device_name, sizeof(g_health.sle_device) - 1);
    } else {
        g_health.sle_device[0] = '\0';
    }
    g_health.dirty = true;
}

void ui_health_set_identity(const char *name, const char *id_number)
{
    if (name != NULL) {
        (void)strncpy_s(g_health.identity_name, sizeof(g_health.identity_name),
                        name, sizeof(g_health.identity_name) - 1);
    } else {
        g_health.identity_name[0] = '\0';
    }

    if (id_number != NULL) {
        (void)strncpy_s(g_health.identity_id, sizeof(g_health.identity_id),
                        id_number, sizeof(g_health.identity_id) - 1);
    } else {
        g_health.identity_id[0] = '\0';
    }
    g_health.dirty = true;
}

void ui_health_set_sensor_value(uint32_t item_mask, const char *value)
{
    char *target = NULL;
    uint32_t target_size = 0;

    switch (item_mask) {
        case HEALTH_ITEM_BP:
            target = g_health.bp_value;
            target_size = sizeof(g_health.bp_value);
            break;
        case HEALTH_ITEM_GLUCOSE:
            target = g_health.bs_value;
            target_size = sizeof(g_health.bs_value);
            break;
        case HEALTH_ITEM_HEIGHT:
            target = g_health.ht_value;
            target_size = sizeof(g_health.ht_value);
            break;
        case HEALTH_ITEM_WEIGHT:
            target = g_health.wt_value;
            target_size = sizeof(g_health.wt_value);
            break;
        default:
            return;
    }

    if (value != NULL) {
        (void)strncpy_s(target, target_size, value, target_size - 1);
    } else {
        target[0] = '\0';
    }
    g_health.dirty = true;
}

void ui_health_set_bmi(const char *bmi)
{
    if (bmi != NULL) {
        (void)strncpy_s(g_health.bmi_value, sizeof(g_health.bmi_value),
                        bmi, sizeof(g_health.bmi_value) - 1);
    } else {
        g_health.bmi_value[0] = '\0';
    }
    g_health.dirty = true;
}

void ui_health_invalidate(void)
{
    g_health.dirty = true;
}

void ui_health_refresh(void)
{
    if (!g_health.dirty) {
        return;
    }
    g_health.dirty = false;

    /* 重绘所有区域 */
    health_draw_identity_card();
    health_draw_bp_card();
    health_draw_data_grid();
    health_draw_sle_card();
    health_draw_reserved();
}

void ui_health_touch_loop(void)
{
    ft6336u_point_t pts[FT6336U_MAX_POINTS];
    uint8_t n = ft6336u_read_points(pts, FT6336U_MAX_POINTS);

    /* 当前页面无按钮交互，仅打印触摸坐标用于调试 */
    if (n > 0) {
        osal_printk("[health] touch x=%u y=%u\r\n", pts[0].x, pts[0].y);
    }
}
