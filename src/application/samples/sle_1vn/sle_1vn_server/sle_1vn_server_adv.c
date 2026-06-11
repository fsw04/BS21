/*
 * BS21E SLE 1VN Server - Announcement implementation
 * Ported from WS63E sle_1vn_server_adv.c, adapted to BS21E SDK.
 */
#include <stdio.h>
#include <string.h>
#include "securec.h"
#include "errcode.h"
#include "common_def.h"
#include "osal_addr.h"
#include "sle_common.h"
#include "sle_connection_manager.h"
#include "sle_device_discovery.h"
#include "sle_device_manager.h"
#include "sle_errcode.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "sle_1vn_server.h"
#include "sle_1vn_server_adv.h"
#include "efuse.h"

#define NAME_MAX_LENGTH                     16
#define SLE_CONN_INTV_MIN_DEFAULT           0x64
#define SLE_CONN_INTV_MAX_DEFAULT           0x64
#define SLE_ADV_INTERVAL_MIN_DEFAULT        0xC8
#define SLE_ADV_INTERVAL_MAX_DEFAULT        0xC8
#define SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT 0x1F4
#define SLE_CONN_MAX_LATENCY                0x1F3
#define SLE_ADV_TX_POWER                    10
#define SLE_ADV_DATA_LEN_MAX                251

#define MAC_ADDR_MOV_BIT_NUM_1 8
#define MAC_ADDR_MOV_BIT_NUM_2 16
#define MAC_ADDR_MOV_BIT_NUM_3 24
#define MAC_ADDR_MOV_BIT_NUM_4 32
#define MAC_ADDR_MOV_BIT_NUM_5 40
#define UUID_16BIT_HIGH_BYTE_SHIFT 8

#define SLE_1VN_SERVER_LOG "[sle 1vn server]"

extern errcode_t sle_1vn_enable_server_cbk(void);

static char g_sle_1vn_local_name[NAME_MAX_LENGTH] = {0};

static uint16_t sle_adv_append_tlv(uint8_t *data, uint16_t max_len, uint16_t index,
                                   uint8_t type, const uint8_t *value, uint8_t value_len)
{
    if ((data == NULL) || (value == NULL) || ((uint32_t)index + 2 + value_len > max_len)) {
        return 0;
    }

    data[index++] = type;
    data[index++] = value_len;
    if (memcpy_s(&data[index], max_len - index, value, value_len) != EOK) {
        osal_printk("%s append tlv fail type:0x%02x\r\n", SLE_1VN_SERVER_LOG, type);
        return 0;
    }

    return (uint16_t)(index + value_len);
}

const char *sle_1vn_get_local_name(void)
{
    if (g_sle_1vn_local_name[0] != '\0') {
        return g_sle_1vn_local_name;
    }

    uint8_t soc_id[20] = {0};
    errcode_t ret = uapi_soc_read_id(soc_id, sizeof(soc_id));
    if (ret == ERRCODE_SUCC) {
        if (sprintf_s(g_sle_1vn_local_name, sizeof(g_sle_1vn_local_name), "%s-%02x%02x%02x%02x",
                      CONFIG_SLE_1VN_SERVER_NAME_PREFIX,
                      soc_id[16], soc_id[17], soc_id[18], soc_id[19]) < 0) {
            osal_printk("%s build local_name fail\r\n", SLE_1VN_SERVER_LOG);
            return NULL;
        }
    } else {
        if (sprintf_s(g_sle_1vn_local_name, sizeof(g_sle_1vn_local_name), "%s-%02d",
                      CONFIG_SLE_1VN_SERVER_NAME_PREFIX, CONFIG_SLE_1VN_SERVER_ID) < 0) {
            osal_printk("%s build local_name fail\r\n", SLE_1VN_SERVER_LOG);
            return NULL;
        }
    }

    return g_sle_1vn_local_name;
}

static void sle_power_on_cbk(uint8_t status)
{
    osal_printk("%s sle power on: 0x%x\r\n", SLE_1VN_SERVER_LOG, status);
    enable_sle();
}

static void sle_dev_enable_cbk(uint8_t status)
{
    unused(status);
    osal_printk("%s sle dev enable: 0x%x\r\n", SLE_1VN_SERVER_LOG, status);
    sle_remove_all_pairs();
    sle_1vn_enable_server_cbk();
}

errcode_t sle_1vn_server_sample_dev_cbk_register(void)
{
    errcode_t ret;
    sle_dev_manager_callbacks_t dev_mgr_cbks = {0};
    dev_mgr_cbks.sle_power_on_cb = sle_power_on_cbk;
    dev_mgr_cbks.sle_enable_cb = sle_dev_enable_cbk;
    ret = sle_dev_manager_register_callbacks(&dev_mgr_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s dev_manager_register_cbks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
#if (CORE_NUMS < 2)
    enable_sle();
#endif
    return ERRCODE_SLE_SUCCESS;
}

static uint16_t sle_set_adv_local_name(uint8_t *adv_data, uint16_t max_len)
{
    const char *local_name = sle_1vn_get_local_name();
    uint8_t local_name_len;
    uint8_t i;

    if (local_name == NULL) {
        return 0;
    }
    local_name_len = (uint8_t)strlen(local_name);

    osal_printk("%s local_name_len = %d\r\n", SLE_1VN_SERVER_LOG, local_name_len);
    osal_printk("%s local_name: ", SLE_1VN_SERVER_LOG);
    for (i = 0; i < local_name_len; i++) {
        osal_printk("0x%02x ", local_name[i]);
    }
    osal_printk("\r\n");

    return sle_adv_append_tlv(adv_data, max_len, 0, SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME,
                              (const uint8_t *)local_name, local_name_len);
}

static uint16_t sle_set_adv_data(uint8_t *adv_data)
{
    uint16_t idx = 0;
    uint8_t discovery_level = SLE_ANNOUNCE_LEVEL_NORMAL;
    uint8_t access_mode = 0;
    uint8_t service_uuid[2] = {
        (uint8_t)(SLE_UUID_SERVER_SERVICE & 0xFF),
        (uint8_t)((SLE_UUID_SERVER_SERVICE >> UUID_16BIT_HIGH_BYTE_SHIFT) & 0xFF)
    };
    uint16_t next_idx;

    next_idx = sle_adv_append_tlv(adv_data, SLE_ADV_DATA_LEN_MAX, idx,
                                  SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL, &discovery_level,
                                  sizeof(discovery_level));
    if (next_idx == 0) {
        osal_printk("%s adv_disc_level append fail\r\n", SLE_1VN_SERVER_LOG);
        return 0;
    }
    idx = next_idx;

    next_idx = sle_adv_append_tlv(adv_data, SLE_ADV_DATA_LEN_MAX, idx,
                                  SLE_ADV_DATA_TYPE_ACCESS_MODE, &access_mode,
                                  sizeof(access_mode));
    if (next_idx == 0) {
        osal_printk("%s adv_access_mode append fail\r\n", SLE_1VN_SERVER_LOG);
        return 0;
    }
    idx = next_idx;

    next_idx = sle_adv_append_tlv(adv_data, SLE_ADV_DATA_LEN_MAX, idx,
                                  SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS,
                                  service_uuid, sizeof(service_uuid));
    if (next_idx == 0) {
        osal_printk("%s adv_service_uuid append fail\r\n", SLE_1VN_SERVER_LOG);
        return 0;
    }
    idx = next_idx;

    next_idx = sle_set_adv_local_name(&adv_data[idx], SLE_ADV_DATA_LEN_MAX - idx);
    if (next_idx == 0) {
        return 0;
    }
    idx += next_idx;
    return idx;
}

static uint16_t sle_set_scan_response_data(uint8_t *scan_rsp_data)
{
    uint16_t idx = 0;
    uint8_t tx_power_level = SLE_ADV_TX_POWER;
    uint16_t next_idx;

    next_idx = sle_adv_append_tlv(scan_rsp_data, SLE_ADV_DATA_LEN_MAX, idx,
                                  SLE_ADV_DATA_TYPE_TX_POWER_LEVEL, &tx_power_level,
                                  sizeof(tx_power_level));
    if (next_idx == 0) {
        osal_printk("%s scan_response_data append fail\r\n", SLE_1VN_SERVER_LOG);
        return 0;
    }
    idx = next_idx;

    next_idx = sle_set_adv_local_name(&scan_rsp_data[idx], SLE_ADV_DATA_LEN_MAX - idx);
    if (next_idx == 0) {
        return 0;
    }
    idx += next_idx;
    return idx;
}

static int sle_set_default_announce_param(void)
{
    errno_t ret;
    sle_announce_param_t param = {0};
    const char *local_name = sle_1vn_get_local_name();
    uint64_t config_addr = CONFIG_SLE_1VN_SERVER_MAC_ADDR;
    unsigned char local_addr[SLE_ADDR_LEN] = {
        (uint8_t)((config_addr >> MAC_ADDR_MOV_BIT_NUM_5) & 0xff),
        (uint8_t)((config_addr >> MAC_ADDR_MOV_BIT_NUM_4) & 0xff),
        (uint8_t)((config_addr >> MAC_ADDR_MOV_BIT_NUM_3) & 0xff),
        (uint8_t)((config_addr >> MAC_ADDR_MOV_BIT_NUM_2) & 0xff),
        (uint8_t)((config_addr >> MAC_ADDR_MOV_BIT_NUM_1) & 0xff),
        (uint8_t)(config_addr & 0xff)
    };

    if (local_name == NULL) {
        return ERRCODE_SLE_FAIL;
    }

    osal_printk("%s ownAddr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_1VN_SERVER_LOG, local_addr[0], local_addr[1], local_addr[2],
                local_addr[3], local_addr[4], local_addr[5]);

    param.announce_mode = SLE_ANNOUNCE_MODE_CONNECTABLE_SCANABLE;
    param.announce_handle = SLE_ADV_HANDLE_DEFAULT;
    param.announce_gt_role = SLE_ANNOUNCE_ROLE_T_CAN_NEGO;
    param.announce_level = SLE_ANNOUNCE_LEVEL_NORMAL;
    param.announce_channel_map = SLE_ADV_CHANNEL_MAP_DEFAULT;
    param.announce_interval_min = SLE_ADV_INTERVAL_MIN_DEFAULT;
    param.announce_interval_max = SLE_ADV_INTERVAL_MAX_DEFAULT;
    param.conn_interval_min = SLE_CONN_INTV_MIN_DEFAULT;
    param.conn_interval_max = SLE_CONN_INTV_MAX_DEFAULT;
    param.conn_max_latency = SLE_CONN_MAX_LATENCY;
    param.conn_supervision_timeout = SLE_CONN_SUPERVISION_TIMEOUT_DEFAULT;
    param.own_addr.type = 0;

    ret = memcpy_s(param.own_addr.addr, SLE_ADDR_LEN, local_addr, SLE_ADDR_LEN);
    if (ret != EOK) {
        osal_printk("%s set_default_announce_param memcpy fail\r\n", SLE_1VN_SERVER_LOG);
        return 0;
    }

    sle_addr_t local_address = {0};
    local_address.type = 0;
    (void)memcpy_s(local_address.addr, SLE_ADDR_LEN, local_addr, SLE_ADDR_LEN);
    sle_set_local_addr(&local_address);

    sle_set_local_name((uint8_t *)local_name, (uint8_t)(strlen(local_name) + 1));

    return sle_set_announce_param(param.announce_handle, &param);
}

static int sle_set_default_announce_data(void)
{
    errcode_t ret;
    uint8_t announce_data_len = 0;
    uint8_t seek_data_len = 0;
    sle_announce_data_t data = {0};
    uint8_t adv_handle = SLE_ADV_HANDLE_DEFAULT;
    uint8_t announce_data[SLE_ADV_DATA_LEN_MAX] = {0};
    uint8_t seek_rsp_data[SLE_ADV_DATA_LEN_MAX] = {0};
    uint8_t data_index = 0;

    announce_data_len = sle_set_adv_data(announce_data);
    data.announce_data = announce_data;
    data.announce_data_len = announce_data_len;

    osal_printk("%s announce_data_len = %d\r\n", SLE_1VN_SERVER_LOG, data.announce_data_len);
    osal_printk("%s announce_data: ", SLE_1VN_SERVER_LOG);
    for (data_index = 0; data_index < data.announce_data_len; data_index++) {
        osal_printk("0x%02x ", data.announce_data[data_index]);
    }
    osal_printk("\r\n");

    seek_data_len = sle_set_scan_response_data(seek_rsp_data);
    data.seek_rsp_data = seek_rsp_data;
    data.seek_rsp_data_len = seek_data_len;

    osal_printk("%s seek_rsp_data_len = %d\r\n", SLE_1VN_SERVER_LOG, data.seek_rsp_data_len);
    osal_printk("%s seek_rsp_data: ", SLE_1VN_SERVER_LOG);
    for (data_index = 0; data_index < data.seek_rsp_data_len; data_index++) {
        osal_printk("0x%02x ", data.seek_rsp_data[data_index]);
    }
    osal_printk("\r\n");

    ret = sle_set_announce_data(adv_handle, &data);
    if (ret == ERRCODE_SLE_SUCCESS) {
        osal_printk("%s set announce data success.\r\n", SLE_1VN_SERVER_LOG);
    } else {
        osal_printk("%s set announce data fail.\r\n", SLE_1VN_SERVER_LOG);
    }

    return ERRCODE_SLE_SUCCESS;
}

static void sle_announce_enable_cbk(uint32_t announce_id, errcode_t status)
{
    osal_printk("%s announce_enable id:%02x, state:%x\r\n", SLE_1VN_SERVER_LOG, announce_id, status);
}

static void sle_announce_disable_cbk(uint32_t announce_id, errcode_t status)
{
    osal_printk("%s announce_disable id:%02x, state:%x\r\n", SLE_1VN_SERVER_LOG, announce_id, status);
}

static void sle_announce_terminal_cbk(uint32_t announce_id)
{
    osal_printk("%s announce_terminal id:%02x\r\n", SLE_1VN_SERVER_LOG, announce_id);
}

errcode_t sle_1vn_announce_register_cbks(void)
{
    errcode_t ret;
    sle_announce_seek_callbacks_t seek_cbks = {0};
    seek_cbks.announce_enable_cb  = sle_announce_enable_cbk;
    seek_cbks.announce_disable_cb = sle_announce_disable_cbk;
    seek_cbks.announce_terminal_cb = sle_announce_terminal_cbk;
    ret = sle_announce_seek_register_callbacks(&seek_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s register_callbacks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_1vn_server_adv_init(void)
{
    errcode_t ret;

    sle_set_default_announce_param();
    sle_set_default_announce_data();

    ret = sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s start_announce fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}
