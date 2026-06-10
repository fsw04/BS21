#===============================================================================
# ST7796S LCD + FT6336U touch + LVGL health monitor
#===============================================================================

# --- 项目源文件 ---
set(SOURCES "${SOURCES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/st7796s.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/ft6336u.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/lcd_bus.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/lv_port_disp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/lv_port_indev.c"

    "${CMAKE_CURRENT_SOURCE_DIR}/tasks/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tasks/ui_health_lvgl.c"
    PARENT_SCOPE)

# --- SLE 源文件（暂时禁用，验证 LVGL 后再启用） ---
if(CONFIG_ST7796S_SLE_1VN_ENABLE)
    set(SOURCES "${SOURCES}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sle/sle_1vn_server.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sle/sle_1vn_server_adv.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sle/measurement_session.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sle/sensor_collect.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sle/sensor_task.c"
        PARENT_SCOPE)
endif()

# --- LVGL 源文件（仅包含核心 + 必要模块，避免链接未使用代码） ---
set(LVGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src")

# 核心
set(LVGL_CORE
    ${LVGL_DIR}/core/lv_disp.c
    ${LVGL_DIR}/core/lv_event.c
    ${LVGL_DIR}/core/lv_indev.c
    ${LVGL_DIR}/core/lv_obj.c
    ${LVGL_DIR}/core/lv_obj_class.c
    ${LVGL_DIR}/core/lv_obj_draw.c
    ${LVGL_DIR}/core/lv_obj_pos.c
    ${LVGL_DIR}/core/lv_obj_scroll.c
    ${LVGL_DIR}/core/lv_obj_style.c
    ${LVGL_DIR}/core/lv_obj_style_gen.c
    ${LVGL_DIR}/core/lv_obj_tree.c
    ${LVGL_DIR}/core/lv_refr.c
    ${LVGL_DIR}/core/lv_theme.c
)

# 绘制
set(LVGL_DRAW
    ${LVGL_DIR}/draw/lv_draw.c
    ${LVGL_DIR}/draw/lv_draw_arc.c
    ${LVGL_DIR}/draw/lv_draw_rect.c
    ${LVGL_DIR}/draw/lv_draw_label.c
    ${LVGL_DIR}/draw/lv_draw_line.c
    ${LVGL_DIR}/draw/lv_draw_mask.c
    ${LVGL_DIR}/draw/lv_draw_transform.c
    ${LVGL_DIR}/draw/lv_draw_layer.c
    ${LVGL_DIR}/draw/lv_img_buf.c
    ${LVGL_DIR}/draw/lv_img_cache.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_arc.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_blend.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_rect.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_label.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_line.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_layer.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_mask.c
    ${LVGL_DIR}/draw/sw/lv_draw_sw_transform.c
)

# 字体
set(LVGL_FONT
    ${LVGL_DIR}/font/lv_font.c
    ${LVGL_DIR}/font/lv_font_fmt_txt.c
    ${LVGL_DIR}/font/lv_font_unscii_8.c
)

# HAL
set(LVGL_HAL
    ${LVGL_DIR}/hal/lv_hal_disp.c
    ${LVGL_DIR}/hal/lv_hal_indev.c
    ${LVGL_DIR}/hal/lv_hal_tick.c
)

# 杂项
set(LVGL_MISC
    ${LVGL_DIR}/misc/lv_area.c
    ${LVGL_DIR}/misc/lv_async.c
    ${LVGL_DIR}/misc/lv_bidi.c
    ${LVGL_DIR}/misc/lv_color.c
    ${LVGL_DIR}/misc/lv_gc.c
    ${LVGL_DIR}/misc/lv_ll.c
    ${LVGL_DIR}/misc/lv_log.c
    ${LVGL_DIR}/misc/lv_lru.c
    ${LVGL_DIR}/misc/lv_math.c
    ${LVGL_DIR}/misc/lv_mem.c
    ${LVGL_DIR}/misc/lv_printf.c
    ${LVGL_DIR}/misc/lv_style.c
    ${LVGL_DIR}/misc/lv_style_gen.c
    ${LVGL_DIR}/misc/lv_timer.c
    ${LVGL_DIR}/misc/lv_tlsf.c
    ${LVGL_DIR}/misc/lv_txt.c
    ${LVGL_DIR}/misc/lv_txt_ap.c
    ${LVGL_DIR}/misc/lv_utils.c
)

# 控件
set(LVGL_WIDGETS
    ${LVGL_DIR}/widgets/lv_arc.c
    ${LVGL_DIR}/widgets/lv_bar.c
    ${LVGL_DIR}/widgets/lv_btn.c
    ${LVGL_DIR}/widgets/lv_label.c
    ${LVGL_DIR}/extra/widgets/lv_widgets.c
)

# 主题
set(LVGL_EXTRA
    ${LVGL_DIR}/extra/lv_extra.c
    ${LVGL_DIR}/extra/themes/default/lv_theme_default.c
    ${LVGL_DIR}/extra/themes/basic/lv_theme_basic.c
)

# 布局
set(LVGL_LAYOUT
    ${LVGL_DIR}/layout/lv_flex.c
    ${LVGL_DIR}/layout/lv_layout.c
)

set(LVGL_SOURCES
    ${LVGL_CORE}
    ${LVGL_DRAW}
    ${LVGL_FONT}
    ${LVGL_HAL}
    ${LVGL_MISC}
    ${LVGL_WIDGETS}
    ${LVGL_EXTRA}
    ${LVGL_LAYOUT}
)

set(SOURCES "${SOURCES}" ${LVGL_SOURCES} PARENT_SCOPE)

# --- 头文件搜索路径 ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/drivers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sle)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# --- LVGL 编译定义 ---
add_definitions(-DLV_CONF_INCLUDE_SIMPLE=1)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE=1)

# --- LVGL 源码放宽编译警告 ---
set_source_files_properties(${LVGL_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-jump-misses-init -Wno-type-limits -Wno-unused-variable")
