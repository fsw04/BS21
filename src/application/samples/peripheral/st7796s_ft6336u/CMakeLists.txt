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

# --- LVGL 源文件（GLOB_RECURSE 但排除 GPU/SDL 等不需要的目录） ---
file(GLOB_RECURSE LVGL_ALL "${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/*.c")

# 排除不需要的目录
list(FILTER LVGL_ALL EXCLUDE REGEX "/draw/sdl/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/draw/stm32_dma2d/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/draw/nxp/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/draw/arm2d/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/draw/renesas/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/draw/swm341_dma2d/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/libs/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/others/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/span/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/spinner/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/keyboard/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/colorwheel/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/meter/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/win/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/animimg/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/imgbtn/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/msgbox/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/calendar/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/spinbox/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/tileview/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/chart/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/tabview/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/list/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/led/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/widgets/menu/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/extra/layouts/grid/")
list(FILTER LVGL_ALL EXCLUDE REGEX "/font/lv_font_montserrat_")
list(FILTER LVGL_ALL EXCLUDE REGEX "/font/lv_font_dejavu_")
list(FILTER LVGL_ALL EXCLUDE REGEX "/font/lv_font_simsun_")
list(FILTER LVGL_ALL EXCLUDE REGEX "/font/lv_font_unscii_16")

set(LVGL_SOURCES ${LVGL_ALL})
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
