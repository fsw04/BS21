#include "ui_health_lvgl.h"
#include "lvgl.h"

/*============================================================
 * Color defines (use lv_color_hex for all colors)
 *============================================================*/
#define COLOR_BG          lv_color_hex(0x000000)
#define COLOR_CARD_BG     lv_color_hex(0x182040)
#define COLOR_CARD_BG2    lv_color_hex(0x1A1A2E)
#define COLOR_ACCENT      lv_color_hex(0x00E5FF)
#define COLOR_BP          lv_color_hex(0xFF6B9D)
#define COLOR_BS          lv_color_hex(0x64B5F6)
#define COLOR_HT          lv_color_hex(0x66BB6A)
#define COLOR_WT          lv_color_hex(0xFFA726)
#define COLOR_FG          lv_color_hex(0xFFFFFF)
#define COLOR_FG_DIM      lv_color_hex(0xB0BEC5)
#define COLOR_GREEN       lv_color_hex(0x4CAF50)
#define COLOR_RED         lv_color_hex(0xF44336)
#define COLOR_RSSI_INACT  lv_color_hex(0x3A3A5C)

/*============================================================
 * Layout constants (480x320 landscape)
 *============================================================*/
#define SCREEN_W           480
#define SCREEN_H           320
#define MARGIN             8
#define GAP                6
#define CARD_RADIUS        6

/* Left panel */
#define LEFT_PANEL_W       280
#define LEFT_PANEL_X       0
#define LEFT_PANEL_Y       0

/* Right panel */
#define RIGHT_PANEL_X      (LEFT_PANEL_W + GAP)
#define RIGHT_PANEL_W      (SCREEN_W - RIGHT_PANEL_X)

/* Identity card */
#define ID_CARD_H          70

/* BP card */
#define BP_CARD_H          60

/* Data grid: 2 columns, 2 rows */
#define DATA_COL_W         ((LEFT_PANEL_W - MARGIN * 2 - GAP) / 2)
#define DATA_ROW_H         60

/* SLE card */
#define SLE_CARD_H         100

/* RSSI bar dimensions */
#define RSSI_BAR_W         4
#define RSSI_BAR_GAP       2
#define RSSI_BAR_MAX_H     16

/*============================================================
 * Static label / object pointers
 *============================================================*/
static lv_obj_t *label_name;
static lv_obj_t *label_id;
static lv_obj_t *label_bp;
static lv_obj_t *label_bs;
static lv_obj_t *label_ht;
static lv_obj_t *label_wt;
static lv_obj_t *label_bmi;
static lv_obj_t *label_sle_status;
static lv_obj_t *label_sle_device;
static lv_obj_t *label_sle_rssi;
static lv_obj_t *obj_sle_dot;
static lv_obj_t *obj_rssi_bars[4];

/*============================================================
 * Helper: create a card panel with rounded corners and border
 *============================================================*/
static lv_obj_t *create_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                             lv_coord_t w, lv_coord_t h, lv_color_t bg_color,
                             lv_color_t border_color, lv_coord_t border_w)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, bg_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_border_width(card, border_w, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/*============================================================
 * Helper: create a label with given style
 *============================================================*/
static lv_obj_t *create_label(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                              const char *text, lv_color_t color,
                              const lv_font_t *font)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_pos(lbl, x, y);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    return lbl;
}

/*============================================================
 * Create RSSI signal bars inside a parent object
 *============================================================*/
static void create_rssi_bars(lv_obj_t *parent, lv_coord_t x, lv_coord_t y)
{
    uint8_t heights[4] = {4, 8, 12, 16};

    for (int i = 0; i < 4; i++) {
        lv_obj_t *bar = lv_obj_create(parent);
        lv_obj_set_size(bar, RSSI_BAR_W, heights[i]);
        lv_obj_set_pos(bar, x + i * (RSSI_BAR_W + RSSI_BAR_GAP),
                       y + RSSI_BAR_MAX_H - heights[i]);
        lv_obj_set_style_bg_color(bar, COLOR_RSSI_INACT, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 1, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
        obj_rssi_bars[i] = bar;
    }
}

/*============================================================
 * Create the left panel (identity + BP + data grid)
 *============================================================*/
static void create_left_panel(lv_obj_t *screen)
{
    lv_coord_t cur_y = MARGIN;

    /* --- Identity card --- */
    lv_obj_t *id_card = create_card(screen, MARGIN, cur_y,
                                    LEFT_PANEL_W - MARGIN * 2, ID_CARD_H,
                                    COLOR_CARD_BG, COLOR_ACCENT, 1);

    /* Person icon placeholder (small square) */
    lv_obj_t *icon_person = lv_obj_create(id_card);
    lv_obj_set_size(icon_person, 16, 16);
    lv_obj_set_pos(icon_person, 10, 10);
    lv_obj_set_style_bg_color(icon_person, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(icon_person, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(icon_person, 8, 0);
    lv_obj_set_style_border_width(icon_person, 0, 0);
    lv_obj_set_style_pad_all(icon_person, 0, 0);
    lv_obj_clear_flag(icon_person, LV_OBJ_FLAG_SCROLLABLE);

    /* Name label */
    label_name = create_label(id_card, 34, 8, "---", COLOR_FG, &lv_font_montserrat_14);

    /* ID number label */
    label_id = create_label(id_card, 34, 28, "---", COLOR_FG_DIM, &lv_font_montserrat_12);

    /* Separator line */
    lv_obj_t *sep = lv_obj_create(id_card);
    lv_obj_set_size(sep, LEFT_PANEL_W - MARGIN * 2 - 20, 1);
    lv_obj_set_pos(sep, 10, 48);
    lv_obj_set_style_bg_color(sep, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    cur_y += ID_CARD_H + GAP;

    /* --- BP card --- */
    lv_obj_t *bp_card = create_card(screen, MARGIN, cur_y,
                                    LEFT_PANEL_W - MARGIN * 2, BP_CARD_H,
                                    COLOR_CARD_BG2, COLOR_BP, 1);

    /* Top accent bar */
    lv_obj_t *bp_accent = lv_obj_create(bp_card);
    lv_obj_set_size(bp_accent, LEFT_PANEL_W - MARGIN * 2 - 2, 3);
    lv_obj_set_pos(bp_accent, 1, 1);
    lv_obj_set_style_bg_color(bp_accent, COLOR_BP, 0);
    lv_obj_set_style_bg_opa(bp_accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bp_accent, 0, 0);
    lv_obj_set_style_border_width(bp_accent, 0, 0);
    lv_obj_set_style_pad_all(bp_accent, 0, 0);
    lv_obj_clear_flag(bp_accent, LV_OBJ_FLAG_SCROLLABLE);

    /* BP label */
    create_label(bp_card, 10, 8, "BP", COLOR_BP, &lv_font_montserrat_12);

    /* BP value */
    label_bp = create_label(bp_card, 10, 28, "---", COLOR_FG, &lv_font_montserrat_18);

    cur_y += BP_CARD_H + GAP;

    /* --- Data grid (2x2) --- */
    /* Row 1: BS, HT */
    lv_coord_t col1_x = MARGIN;
    lv_coord_t col2_x = MARGIN + DATA_COL_W + GAP;

    /* BS card */
    lv_obj_t *bs_card = create_card(screen, col1_x, cur_y,
                                    DATA_COL_W, DATA_ROW_H,
                                    COLOR_CARD_BG, COLOR_BS, 1);

    lv_obj_t *bs_accent = lv_obj_create(bs_card);
    lv_obj_set_size(bs_accent, DATA_COL_W - 2, 3);
    lv_obj_set_pos(bs_accent, 1, 1);
    lv_obj_set_style_bg_color(bs_accent, COLOR_BS, 0);
    lv_obj_set_style_bg_opa(bs_accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bs_accent, 0, 0);
    lv_obj_set_style_border_width(bs_accent, 0, 0);
    lv_obj_set_style_pad_all(bs_accent, 0, 0);
    lv_obj_clear_flag(bs_accent, LV_OBJ_FLAG_SCROLLABLE);

    create_label(bs_card, 8, 8, "BS", COLOR_BS, &lv_font_montserrat_12);
    label_bs = create_label(bs_card, 8, 28, "--", COLOR_FG, &lv_font_montserrat_18);

    /* HT card */
    lv_obj_t *ht_card = create_card(screen, col2_x, cur_y,
                                    DATA_COL_W, DATA_ROW_H,
                                    COLOR_CARD_BG, COLOR_HT, 1);

    lv_obj_t *ht_accent = lv_obj_create(ht_card);
    lv_obj_set_size(ht_accent, DATA_COL_W - 2, 3);
    lv_obj_set_pos(ht_accent, 1, 1);
    lv_obj_set_style_bg_color(ht_accent, COLOR_HT, 0);
    lv_obj_set_style_bg_opa(ht_accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ht_accent, 0, 0);
    lv_obj_set_style_border_width(ht_accent, 0, 0);
    lv_obj_set_style_pad_all(ht_accent, 0, 0);
    lv_obj_clear_flag(ht_accent, LV_OBJ_FLAG_SCROLLABLE);

    create_label(ht_card, 8, 8, "HT", COLOR_HT, &lv_font_montserrat_12);
    label_ht = create_label(ht_card, 8, 28, "--", COLOR_FG, &lv_font_montserrat_18);

    cur_y += DATA_ROW_H + GAP;

    /* Row 2: WT, BMI */
    /* WT card */
    lv_obj_t *wt_card = create_card(screen, col1_x, cur_y,
                                    DATA_COL_W, DATA_ROW_H,
                                    COLOR_CARD_BG, COLOR_WT, 1);

    lv_obj_t *wt_accent = lv_obj_create(wt_card);
    lv_obj_set_size(wt_accent, DATA_COL_W - 2, 3);
    lv_obj_set_pos(wt_accent, 1, 1);
    lv_obj_set_style_bg_color(wt_accent, COLOR_WT, 0);
    lv_obj_set_style_bg_opa(wt_accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(wt_accent, 0, 0);
    lv_obj_set_style_border_width(wt_accent, 0, 0);
    lv_obj_set_style_pad_all(wt_accent, 0, 0);
    lv_obj_clear_flag(wt_accent, LV_OBJ_FLAG_SCROLLABLE);

    create_label(wt_card, 8, 8, "WT", COLOR_WT, &lv_font_montserrat_12);
    label_wt = create_label(wt_card, 8, 28, "--", COLOR_FG, &lv_font_montserrat_18);

    /* BMI card */
    lv_obj_t *bmi_card = create_card(screen, col2_x, cur_y,
                                     DATA_COL_W, DATA_ROW_H,
                                     COLOR_CARD_BG, COLOR_ACCENT, 1);

    lv_obj_t *bmi_accent = lv_obj_create(bmi_card);
    lv_obj_set_size(bmi_accent, DATA_COL_W - 2, 3);
    lv_obj_set_pos(bmi_accent, 1, 1);
    lv_obj_set_style_bg_color(bmi_accent, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(bmi_accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bmi_accent, 0, 0);
    lv_obj_set_style_border_width(bmi_accent, 0, 0);
    lv_obj_set_style_pad_all(bmi_accent, 0, 0);
    lv_obj_clear_flag(bmi_accent, LV_OBJ_FLAG_SCROLLABLE);

    create_label(bmi_card, 8, 8, "BMI", COLOR_ACCENT, &lv_font_montserrat_12);
    label_bmi = create_label(bmi_card, 8, 28, "--", COLOR_FG, &lv_font_montserrat_18);
}

/*============================================================
 * Create the right panel (SLE status + reserved area)
 *============================================================*/
static void create_right_panel(lv_obj_t *screen)
{
    lv_coord_t cur_y = MARGIN;
    lv_coord_t card_w = RIGHT_PANEL_W - MARGIN * 2;

    /* --- SLE status card --- */
    lv_obj_t *sle_card = create_card(screen, RIGHT_PANEL_X + MARGIN, cur_y,
                                     card_w, SLE_CARD_H,
                                     COLOR_CARD_BG, COLOR_ACCENT, 1);

    /* Status dot */
    obj_sle_dot = lv_obj_create(sle_card);
    lv_obj_set_size(obj_sle_dot, 8, 8);
    lv_obj_set_pos(obj_sle_dot, 10, 10);
    lv_obj_set_style_bg_color(obj_sle_dot, COLOR_RED, 0);
    lv_obj_set_style_bg_opa(obj_sle_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj_sle_dot, 4, 0);
    lv_obj_set_style_border_width(obj_sle_dot, 0, 0);
    lv_obj_set_style_pad_all(obj_sle_dot, 0, 0);
    lv_obj_clear_flag(obj_sle_dot, LV_OBJ_FLAG_SCROLLABLE);

    /* Status text */
    label_sle_status = create_label(sle_card, 24, 6, "Disconnected",
                                    COLOR_RED, &lv_font_montserrat_12);

    /* Device label + value */
    create_label(sle_card, 10, 28, "Device:", COLOR_FG_DIM, &lv_font_montserrat_10);
    label_sle_device = create_label(sle_card, 10, 42, "---", COLOR_FG, &lv_font_montserrat_12);

    /* RSSI label */
    create_label(sle_card, 10, 58, "RSSI:", COLOR_FG_DIM, &lv_font_montserrat_10);

    /* RSSI bars */
    create_rssi_bars(sle_card, 50, 58);

    /* RSSI value label */
    label_sle_rssi = create_label(sle_card, 74, 58, "---", COLOR_FG_DIM, &lv_font_montserrat_10);

    cur_y += SLE_CARD_H + GAP;

    /* --- Reserved area --- */
    lv_coord_t reserved_h = SCREEN_H - cur_y - MARGIN;
    if (reserved_h > 0) {
        create_card(screen, RIGHT_PANEL_X + MARGIN, cur_y,
                    card_w, reserved_h,
                    COLOR_CARD_BG2, COLOR_ACCENT, 1);
    }
}

/*============================================================
 * Public API: create the health monitoring UI
 *============================================================*/
void ui_health_create(void)
{
    lv_obj_t *screen = lv_scr_act();

    /* Set screen background to black */
    lv_obj_set_style_bg_color(screen, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    /* Build left and right panels */
    create_left_panel(screen);
    create_right_panel(screen);
}

/*============================================================
 * Public API: update SLE connection status
 *============================================================*/
void ui_health_set_sle_status(bool connected, const char *device_name, int8_t rssi)
{
    /* Update status dot color */
    lv_obj_set_style_bg_color(obj_sle_dot, connected ? COLOR_GREEN : COLOR_RED, 0);

    /* Update status text */
    lv_label_set_text(label_sle_status, connected ? "Connected" : "Disconnected");
    lv_obj_set_style_text_color(label_sle_status, connected ? COLOR_GREEN : COLOR_RED, 0);

    /* Update device name */
    if (device_name != NULL) {
        lv_label_set_text(label_sle_device, device_name);
    } else {
        lv_label_set_text(label_sle_device, "---");
    }

    /* Update RSSI bars */
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

    for (int i = 0; i < 4; i++) {
        lv_color_t bar_color = (i < active) ? COLOR_ACCENT : COLOR_RSSI_INACT;
        lv_obj_set_style_bg_color(obj_rssi_bars[i], bar_color, 0);
    }

    /* Update RSSI text */
    if (rssi != 0) {
        char rssi_str[16];
        lv_snprintf(rssi_str, sizeof(rssi_str), "%ddBm", (int)rssi);
        lv_label_set_text(label_sle_rssi, rssi_str);
    } else {
        lv_label_set_text(label_sle_rssi, "---");
    }
}

/*============================================================
 * Public API: update identity information
 *============================================================*/
void ui_health_set_identity(const char *name, const char *id_number)
{
    if (name != NULL) {
        lv_label_set_text(label_name, name);
    } else {
        lv_label_set_text(label_name, "---");
    }

    if (id_number != NULL) {
        lv_label_set_text(label_id, id_number);
    } else {
        lv_label_set_text(label_id, "---");
    }
}

/*============================================================
 * Public API: update sensor values
 *============================================================*/
void ui_health_set_sensor_value(uint32_t item_mask, const char *value)
{
    const char *display = (value != NULL) ? value : "--";

    if (item_mask & 0x04U) {  /* HEALTH_ITEM_BP */
        lv_label_set_text(label_bp, display);
    }
    if (item_mask & 0x08U) {  /* HEALTH_ITEM_GLUCOSE */
        lv_label_set_text(label_bs, display);
    }
    if (item_mask & 0x01U) {  /* HEALTH_ITEM_HEIGHT */
        lv_label_set_text(label_ht, display);
    }
    if (item_mask & 0x02U) {  /* HEALTH_ITEM_WEIGHT */
        lv_label_set_text(label_wt, display);
    }
}

/*============================================================
 * Public API: update BMI value
 *============================================================*/
void ui_health_set_bmi(const char *bmi)
{
    if (bmi != NULL) {
        lv_label_set_text(label_bmi, bmi);
    } else {
        lv_label_set_text(label_bmi, "--");
    }
}
