#===============================================================================
# @brief    cmake file
# Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
#===============================================================================
if(DEFINED CONFIG_SAMPLE_SUPPORT_LCD_TOUCH)
    set(SOURCES "${SOURCES}" "${CMAKE_CURRENT_SOURCE_DIR}/st7796s.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/ft6336u.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/lcd_touch_demo.c" PARENT_SCOPE)
endif()
