#===============================================================================
# ST7796S LCD + FT6336U touch + SLE 1VN health monitor (手写UI)
#===============================================================================

set(SOURCES "${SOURCES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/st7796s.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/ft6336u.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/lcd_bus.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/font.c"

    "${CMAKE_CURRENT_SOURCE_DIR}/tasks/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tasks/ui_page_health.c"

    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sle_1vn_server.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sle_1vn_server_adv.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/measurement_session.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sensor_collect.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sle/sensor_task.c"
    PARENT_SCOPE)

# 子目录头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/drivers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sle)
