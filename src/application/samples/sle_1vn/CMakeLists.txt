#===============================================================================
# @brief    cmake file for SLE 1VN Server (BS21E)
# Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
#===============================================================================
set(SLE_1VN_SAMPLE_ENABLED false)
if(DEFINED CONFIG_ENABLE_SLE_1VN AND NOT DEFINED CONFIG_ST7796S_SLE_1VN_ENABLE)
  if(CONFIG_ENABLE_SLE_1VN OR "CONFIG_ENABLE_SLE_1VN=1" IN_LIST DEFINES)
    set(SLE_1VN_SAMPLE_ENABLED true)
  endif()
endif()

if(SLE_1VN_SAMPLE_ENABLED)
set(SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_1vn_server/sle_1vn_server_adv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_1vn_server/sle_1vn_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/measurement_session.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_collect.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_1vn.c
)
endif()
set(SOURCES "${SOURCES}" ${SOURCES_LIST} PARENT_SCOPE)
set(PUBLIC_HEADER "${PUBLIC_HEADER}" PARENT_SCOPE)
