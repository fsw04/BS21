#===============================================================================
# ST7796S LCD + FT6336U touch + LVGL + SLE 1VN health monitor
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

    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sle_1vn_server.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sle_1vn_server_adv.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/measurement_session.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sensor_collect.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sensor_task.c"
    PARENT_SCOPE)

# --- LVGL 源文件 ---
file(GLOB_RECURSE LVGL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/*.c")
set(SOURCES "${SOURCES}" ${LVGL_SOURCES} PARENT_SCOPE)

# --- 头文件搜索路径 ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/drivers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sle)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# --- LVGL 编译定义 ---
add_definitions(-DLV_CONF_INCLUDE_SIMPLE=1)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE=1)
