/**
 * @file lv_conf.h
 * @brief LVGL v8.3 configuration for BS21E RISC-V MCU with ST7796S LCD (480x320)
 *
 * Target: BS21E (1MB Flash, 160KB RAM), ST7796S 480x320 landscape, SPI direct drive
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH      16
#define LV_COLOR_16_SWAP    0
#define LV_COLOR_SCREEN_TRANSP  0
#define LV_COLOR_MIX_ROUND_OFS  0
#define LV_COLOR_CHROMA_KEY     lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM      1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE "soc_osal.h"
    extern void *osal_malloc(size_t);
    extern void  osal_free(void *);
    #define LV_MEM_CUSTOM_ALLOC   osal_malloc
    #define LV_MEM_CUSTOM_FREE    osal_free
#endif

/* Fallback if custom mem disabled */
#define LV_MEM_SIZE        (64U * 1024)
#define LV_MEM_ADR         0
#define LV_MEM_BUF_MAX_NUM 16

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD  30      /* ms */
#define LV_INDEV_DEF_READ_PERIOD 30      /* ms */
#define LV_TICK_CUSTOM           1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "soc_osal.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (osal_msleep(0), (uint32_t)osal_get_tick_count())
#endif

#define LV_DEF_REFR_PERIOD       30
#define LV_DISP_DEF_REFR_PERIOD  30
#define LV_INDEV_DEF_READ_PERIOD 30

/*=======================
   FEATURE CONFIGURATION
 *=======================*/

/* Logging */
#define LV_USE_LOG       0
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_TRACE
    #define LV_LOG_PRINTF 0
#endif

/* Asserts */
#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* Animation */
#define LV_USE_ANIMATION  1

/* Asynchronous */
#define LV_USE_ASYNC      0

/* Keyboard */
#define LV_USE_KEY        0

/* File system */
#define LV_USE_FS_STDIO   0
#define LV_USE_FS_POSIX   0
#define LV_USE_FS_FATFS   0

/* Images */
#define LV_USE_IMG        1
#define LV_USE_IMG_DECODER 1
#if LV_USE_IMG_DECODER
    #define LV_IMG_CF_INDEXED   1
    #define LV_IMG_CF_ALPHA     1
#endif
#define LV_USE_FS_MEMFS   0

/* Glyph (font) storage */
#define LV_USE_FONT_COMPRESSED  0

/* Sub-pixel rendering */
#define LV_USE_FONT_SUBPX      0
#if LV_USE_FONT_SUBPX
    #define LV_FONT_SUBPX_BGR  0
#endif

/*========================
   WIDGET USAGE
 *========================*/
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    0
#define LV_USE_CANVAS       0
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     0
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         0
#define LV_USE_ROLLER       0
#define LV_USE_SLIDER       0
#define LV_USE_SWITCH       0
#define LV_USE_TEXTAREA     0
#define LV_USE_TABLE        0

/* Extra widgets */
#define LV_USE_CHART        0
#define LV_USE_CALENDAR     0
#define LV_USE_COLORWHEEL   0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      0
#define LV_USE_KEYBOARD     0
#define LV_USE_METER        0
#define LV_USE_LIST         0
#define LV_USE_MENU         0
#define LV_USE_MSGBOX       0
#define LV_USE_SPAN         0
#define LV_USE_TABVIEW      0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0
#define LV_USE_LED          0
#define LV_USE_IMGBTN       0
#define LV_USE_PANEL        1

/*==================
   THEMES
 *==================*/
#define LV_USE_THEME_DEFAULT   1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK   1
    #define LV_THEME_DEFAULT_GROW   1
    #define LV_THEME_DEFAULT_TRANSITION_TIME  80
#endif
#define LV_USE_THEME_BASIC     1

/*==================
   LAYOUTS
 *==================*/
#define LV_USE_FLEX    1
#define LV_USE_GRID    0

/*====================
   FONT USAGE
 *====================*/
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    0
#define LV_FONT_MONTSERRAT_12    0
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    0
#define LV_FONT_MONTSERRAT_20    0
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    0
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    0
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0

#define LV_FONT_MONTSERRAT_12_SUBPX       0
#define LV_FONT_MONTSERRAT_28_COMPRESSED   0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW   0
#define LV_FONT_SIMSUN_16_CJK             0
#define LV_FONT_UNSCII_8                   0
#define LV_FONT_UNSCII_16                  0

#define LV_FONT_DEFAULT     &lv_font_montserrat_16
#define LV_FONT_FMT_TXT_LARGE  0
#define LV_USE_FONT_PLACEHOLDER 1

/*====================
   GPU
 *====================*/
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_SDL          0

/*====================
   RESOLUTION
 *====================*/
#define LV_HOR_RES_MAX     480
#define LV_VER_RES_MAX     320

/*====================
   MISC
 *====================*/
#define LV_USE_SNAPSHOT       0
#define LV_USE_SYSMON         0
#define LV_USE_PERF_MONITOR   0
#define LV_USE_MEM_MONITOR    0
#define LV_USE_REFR_MONITOR   0
#define LV_USE_MSG            0
#define LV_USE_IME_PINYIN     0

#endif /* LV_CONF_H */
