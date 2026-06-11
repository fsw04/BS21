/*
 * BS21E SLE 1VN Server - Core implementation
 * Ported from WS63E sle_1vn_server.c, adapted to BS21E SDK.
 * Protocol logic MUST remain identical to WS63E for cross-platform compatibility.
 */
#include "securec.h"
#include "sle_common.h"
#include "osal_debug.h"
#include "sle_errcode.h"
#include "osal_addr.h"
#include "soc_osal.h"
#include "app_init.h"
#include <stdbool.h>
#include <string.h>
#include "common_def.h"
#include "sle_connection_manager.h"
#include "sle_device_discovery.h"
#include "sle_device_manager.h"
#include "sle_1vn_server_adv.h"
#include "sle_1vn_server.h"

#define OCTET_BIT_LEN  8
#define UUID_LEN_2     2
#define UUID_INDEX     14
#define SLE_1VN_MTU_SIZE 1500
#define SLE_1VN_PROPERTY_VALUE_LEN_MAX SLE_1VN_MTU_SIZE
#define SLE_REPORT_DATA_LEN 1500
#define SLE_REPORT_SAFE_SINGLE_LEN 240
#define SLE_REPORT_FRAGMENT_LEN 200
#define SLE_REPORT_FRAGMENT_PACKET_LEN 240
#define SLE_REPORT_FRAGMENT_INTERVAL_MS 20

static char     g_sle_uuid_app_uuid[UUID_LEN_2] = {0x0, 0x0};
static uint8_t  g_sle_property_value[SLE_1VN_PROPERTY_VALUE_LEN_MAX] = {0};
static uint16_t g_sle_property_value_len = OCTET_BIT_LEN;
static uint16_t g_sle_conn_id = 0;
static bool     g_sle_connected = false;
static uint8_t  g_sle_report_ready = 0;
static uint32_t g_sle_report_mtu = 0;
static uint16_t g_sle_report_msg_id = 0;
static uint8_t  g_server_id = 0;
static uint16_t g_service_handle = 0;
static uint16_t g_property_handle = 0;

static sle_acb_state_t  g_sle_conn_state = SLE_ACB_STATE_NONE;
static sle_pair_state_t g_sle_pair_state = SLE_PAIR_NONE;

#define UUID_16BIT_LEN  2
#define UUID_128BIT_LEN 16
#define SLE_1VN_SERVER_LOG  "[sle 1vn server]"

static uint8_t g_sle_base[] = {
    0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static wearable_identity_t g_wearable_identity = {0};
static uint32_t g_wearable_identity_sequence = 0;
static wearable_identity_callback_t g_wearable_identity_cb = NULL;

static void encode2byte_little(uint8_t *_ptr, uint16_t data)
{
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((data) >> 0x8);
    *(uint8_t *)(_ptr) = (uint8_t)(data);
}

static void sle_uuid_set_base(sle_uuid_t *out)
{
    errcode_t ret;
    ret = memcpy_s(out->uuid, SLE_UUID_LEN, g_sle_base, SLE_UUID_LEN);
    if (ret != EOK) {
        osal_printk("%s sle_uuid_set_base memcpy fail\n", SLE_1VN_SERVER_LOG);
        out->len = 0;
        return;
    }
    out->len = UUID_LEN_2;
}

static void sle_uuid_setu2(uint16_t u2, sle_uuid_t *out)
{
    sle_uuid_set_base(out);
    out->len = UUID_LEN_2;
    encode2byte_little(&out->uuid[UUID_INDEX], u2);
}

static void sle_uuid_print(sle_uuid_t *uuid)
{
    if (uuid == NULL) {
        osal_printk("%s uuid is null\r\n", SLE_1VN_SERVER_LOG);
        return;
    }
    if (uuid->len == UUID_16BIT_LEN) {
        osal_printk("%s uuid: %02x %02x.\n", SLE_1VN_SERVER_LOG,
                    uuid->uuid[14], uuid->uuid[15]);
    } else if (uuid->len == UUID_128BIT_LEN) {
        osal_printk("%s uuid: \n", SLE_1VN_SERVER_LOG);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", SLE_1VN_SERVER_LOG,
                    uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3]);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", SLE_1VN_SERVER_LOG,
                    uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7]);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", SLE_1VN_SERVER_LOG,
                    uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11]);
        osal_printk("%s 0x%02x 0x%02x 0x%02x 0x%02x\n", SLE_1VN_SERVER_LOG,
                    uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    }
}

static void ssaps_mtu_changed_cbk(uint8_t server_id, uint16_t conn_id,
    ssap_exchange_info_t *mtu_size, errcode_t status)
{
    uint32_t mtu = (mtu_size != NULL) ? mtu_size->mtu_size : 0;
    uint32_t version = (mtu_size != NULL) ? mtu_size->version : 0;

    osal_printk("%s report mtu cb server=%u conn=%u status=0x%x mtu=%u version=%u\r\n",
                SLE_1VN_SERVER_LOG, (unsigned int)server_id, (unsigned int)conn_id,
                (unsigned int)status, (unsigned int)mtu, (unsigned int)version);
    if ((status == ERRCODE_SLE_SUCCESS) && (mtu_size != NULL)) {
        g_sle_report_mtu = mtu_size->mtu_size;
        g_sle_report_ready = 1;
    } else {
        g_sle_report_mtu = 0;
        g_sle_report_ready = 0;
    }
}

static errcode_t sle_1vn_send_ssaps_response(uint8_t server_id, uint16_t conn_id, uint16_t request_id,
                                             const uint8_t *data, uint16_t len)
{
    ssaps_send_rsp_t rsp = {0};
    static uint8_t empty_rsp = 0;

    rsp.request_id = request_id;
    rsp.status = ERRCODE_SLE_SUCCESS;
    rsp.value = (uint8_t *)(data != NULL ? data : &empty_rsp);
    rsp.value_len = len;
    return ssaps_send_response(server_id, conn_id, &rsp);
}

static void ssaps_start_service_cbk(uint8_t server_id, uint16_t handle, errcode_t status)
{
    osal_printk("%s start_service server_id:%d, handle:%x, status:%x\r\n",
                SLE_1VN_SERVER_LOG, server_id, handle, status);
    if (sle_1vn_server_adv_init() != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s server_adv_init fail\r\n", SLE_1VN_SERVER_LOG);
    }
}

static void ssaps_add_service_cbk(uint8_t server_id, sle_uuid_t *uuid,
    uint16_t handle, errcode_t status)
{
    osal_printk("%s add_service server_id:%x, handle:%x, status:%x\r\n",
                SLE_1VN_SERVER_LOG, server_id, handle, status);
    sle_uuid_print(uuid);
}

static void ssaps_add_property_cbk(uint8_t server_id, sle_uuid_t *uuid,
    uint16_t service_handle, uint16_t handle, errcode_t status)
{
    osal_printk("%s add_property server_id:%x, svc_handle:%x, handle:%x, status:%x\r\n",
                SLE_1VN_SERVER_LOG, server_id, service_handle, handle, status);
    sle_uuid_print(uuid);
}

static void ssaps_add_descriptor_cbk(uint8_t server_id, sle_uuid_t *uuid,
    uint16_t service_handle, uint16_t property_handle, errcode_t status)
{
    osal_printk("%s add_descriptor server_id:%x, svc_handle:%x, prop_handle:%x, status:%x\r\n",
                SLE_1VN_SERVER_LOG, server_id, service_handle, property_handle, status);
    sle_uuid_print(uuid);
}

static void ssaps_delete_all_service_cbk(uint8_t server_id, errcode_t status)
{
    osal_printk("%s delete_all_service server_id:%x, status:%x\r\n",
                SLE_1VN_SERVER_LOG, server_id, status);
}

static errcode_t sle_uuid_server_service_add(void)
{
    errcode_t ret;
    sle_uuid_t service_uuid = {0};
    sle_uuid_setu2(SLE_UUID_SERVER_SERVICE, &service_uuid);
    ret = ssaps_add_service_sync(g_server_id, &service_uuid, true, &g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s add service fail, ret:%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_uuid_server_property_add(void)
{
    errcode_t ret;
    ssaps_property_info_t property = {0};
    ssaps_desc_info_t descriptor = {0};
    uint8_t desc_value[] = {0x01, 0x00};

    property.permissions = SLE_UUID_TEST_PROPERTIES;
    property.operate_indication = SLE_UUID_TEST_OPERATION_INDICATION;
    sle_uuid_setu2(SLE_UUID_SERVER_NTF_REPORT, &property.uuid);
    property.value_len = g_sle_property_value_len;
    property.value = (uint8_t *)osal_vmalloc(sizeof(g_sle_property_value));
    if (property.value == NULL) {
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(property.value, sizeof(g_sle_property_value),
                 g_sle_property_value, g_sle_property_value_len) != EOK) {
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_add_property_sync(g_server_id, g_service_handle,
                                  &property, &g_property_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s add property fail, ret:%x\r\n", SLE_1VN_SERVER_LOG, ret);
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }

    descriptor.permissions = SLE_UUID_TEST_DESCRIPTOR;
    descriptor.type = SSAP_DESCRIPTOR_USER_DESCRIPTION;
    descriptor.operate_indication = SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE;
    descriptor.value_len = sizeof(desc_value);
    descriptor.value = desc_value;

    ret = ssaps_add_descriptor_sync(g_server_id, g_service_handle,
                                    g_property_handle, &descriptor);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s add descriptor fail, ret:%x\r\n", SLE_1VN_SERVER_LOG, ret);
        osal_vfree(property.value);
        return ERRCODE_SLE_FAIL;
    }

    osal_vfree(property.value);
    return ERRCODE_SLE_SUCCESS;
}

static errcode_t sle_1vn_server_add(void)
{
    errcode_t ret;
    sle_uuid_t app_uuid = {0};
    ssap_exchange_info_t info = {0};

    osal_printk("%s add service in\r\n", SLE_1VN_SERVER_LOG);
    app_uuid.len = sizeof(g_sle_uuid_app_uuid);
    if (memcpy_s(app_uuid.uuid, app_uuid.len, g_sle_uuid_app_uuid,
                 sizeof(g_sle_uuid_app_uuid)) != EOK) {
        return ERRCODE_SLE_FAIL;
    }
    ssaps_register_server(&app_uuid, &g_server_id);

    if (sle_uuid_server_service_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    if (sle_uuid_server_property_add() != ERRCODE_SLE_SUCCESS) {
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    info.mtu_size = SLE_1VN_MTU_SIZE;
    ret = ssaps_set_info(g_server_id, &info);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s set ssaps mtu fail, ret:%x\r\n", SLE_1VN_SERVER_LOG, ret);
        ssaps_unregister_server(g_server_id);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("%s add service, server_id:%x, svc_handle:%x, prop_handle:%x\r\n",
                SLE_1VN_SERVER_LOG, g_server_id, g_service_handle, g_property_handle);

    ret = ssaps_start_service(g_server_id, g_service_handle);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s start service fail, ret:%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ERRCODE_SLE_FAIL;
    }
    osal_printk("%s add service out\r\n", SLE_1VN_SERVER_LOG);
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_1vn_server_send_notify_by_handle(const uint8_t *data, uint16_t len)
{
    ssaps_ntf_ind_t param = {0};
    errcode_t ret;

    if ((data == NULL) || (len == 0) || (!g_sle_connected)) {
        return ERRCODE_SLE_FAIL;
    }

    param.handle = g_property_handle;
    param.type = SSAP_PROPERTY_TYPE_VALUE;
    param.value_len = len;
    param.value = osal_vmalloc(param.value_len);
    if (param.value == NULL) {
        osal_printk("%s send_notify vmalloc fail\r\n", SLE_1VN_SERVER_LOG);
        return ERRCODE_SLE_FAIL;
    }
    if (memcpy_s(param.value, param.value_len, data, len) != EOK) {
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }

    ret = ssaps_notify_indicate(g_server_id, g_sle_conn_id, &param);
    if (ret != ERRCODE_SUCC) {
        osal_printk("%s ssaps_notify_indicate fail len=%u ret=0x%x\r\n",
                    SLE_1VN_SERVER_LOG, (unsigned int)len, (unsigned int)ret);
        osal_vfree(param.value);
        return ERRCODE_SLE_FAIL;
    }
    osal_vfree(param.value);
    return ERRCODE_SLE_SUCCESS;
}

bool sle_1vn_server_is_connected(void)
{
    return g_sle_connected;
}

bool sle_1vn_server_report_is_ready(void)
{
    return g_sle_report_ready != 0;
}

uint32_t sle_1vn_server_report_mtu(void)
{
    return g_sle_report_mtu;
}

static void sle_connect_state_changed_cbk(uint16_t conn_id, const sle_addr_t *addr,
    sle_acb_state_t conn_state, sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    errcode_t ret;

    g_sle_conn_state = conn_state;

    osal_printk("%s conn_state changed conn_id:0x%02x, conn_state:0x%x, "
                "pair_state:0x%x, disc_reason:0x%x\r\n",
                SLE_1VN_SERVER_LOG, conn_id, conn_state, pair_state, disc_reason);
    osal_printk("%s addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_1VN_SERVER_LOG, addr->addr[0], addr->addr[1], addr->addr[2],
                addr->addr[3], addr->addr[4], addr->addr[5]);

    if (conn_state == SLE_ACB_STATE_CONNECTED) {
        g_sle_conn_id = conn_id;
        g_sle_connected = true;
        g_sle_report_ready = 0;
        g_sle_report_mtu = 0;
        ret = sle_set_data_len(conn_id, SLE_REPORT_DATA_LEN);
        osal_printk("%s report set data len conn=%u len=%u ret=0x%x\r\n",
                    SLE_1VN_SERVER_LOG, (unsigned int)conn_id,
                    (unsigned int)SLE_REPORT_DATA_LEN, (unsigned int)ret);
    } else if (conn_state == SLE_ACB_STATE_DISCONNECTED) {
        g_sle_conn_id = 0;
        g_sle_connected = false;
        g_sle_report_ready = 0;
        g_sle_report_mtu = 0;
        g_sle_pair_state = SLE_PAIR_NONE;
        ret = sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
        if (ret != ERRCODE_SLE_SUCCESS) {
            osal_printk("%s start_announce fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        }
    }
}

static void sle_pair_complete_cbk(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    osal_printk("%s pair complete conn_id:%02x, status:%x\r\n", SLE_1VN_SERVER_LOG, conn_id, status);
    osal_printk("%s addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_1VN_SERVER_LOG, addr->addr[0], addr->addr[1], addr->addr[2],
                addr->addr[3], addr->addr[4], addr->addr[5]);
    if (status == ERRCODE_SUCC) {
        g_sle_pair_state = SLE_PAIR_PAIRED;
    }
}

static void sle_connect_param_update_req_cbk(uint16_t conn_id, errcode_t status,
    const sle_connection_param_update_req_t *param)
{
    osal_printk("%s param_update_req conn_id:%02x, status:%x, "
                "intv_min:%02x, intv_max:%02x, latency:%02x, timeout:%02x\r\n",
                SLE_1VN_SERVER_LOG, conn_id, status, param->interval_min, param->interval_max,
                param->max_latency, param->supervision_timeout);
}

static void sle_connect_param_update_cbk(uint16_t conn_id, errcode_t status,
    const sle_connection_param_update_evt_t *param)
{
    osal_printk("%s param_update conn_id:%02x, status:%x, "
                "interval:%02x, latency:%02x, supervision:%02x\r\n",
                SLE_1VN_SERVER_LOG, conn_id, status, param->interval, param->latency, param->supervision);
}

static void sle_auth_complete_cbk(uint16_t conn_id, const sle_addr_t *addr, errcode_t status,
    const sle_auth_info_evt_t *evt)
{
    osal_printk("%s auth complete conn_id:%02x, status:%x\r\n", SLE_1VN_SERVER_LOG, conn_id, status);
    osal_printk("%s addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                SLE_1VN_SERVER_LOG, addr->addr[0], addr->addr[1], addr->addr[2],
                addr->addr[3], addr->addr[4], addr->addr[5]);
    osal_printk("%s link_key:%02x, crypto_algo:%x, key_deriv_algo:%x, integr_chk:%x\r\n",
                SLE_1VN_SERVER_LOG, evt->link_key, evt->crypto_algo, evt->key_deriv_algo,
                evt->integr_chk_ind);
}

static void sle_read_rssi_cbk(uint16_t conn_id, int8_t rssi, errcode_t status)
{
    osal_printk("%s read_rssi conn_id:%02x, rssi:%d, status:%x\r\n",
                SLE_1VN_SERVER_LOG, conn_id, rssi, status);
}

static errcode_t sle_1vn_conn_register_cbks(void)
{
    errcode_t ret;
    sle_connection_callbacks_t conn_cbks = {0};
    conn_cbks.connect_state_changed_cb    = sle_connect_state_changed_cbk;
    conn_cbks.connect_param_update_req_cb = sle_connect_param_update_req_cbk;
    conn_cbks.connect_param_update_cb     = sle_connect_param_update_cbk;
    conn_cbks.auth_complete_cb            = sle_auth_complete_cbk;
    conn_cbks.pair_complete_cb            = sle_pair_complete_cbk;
    conn_cbks.read_rssi_cb               = sle_read_rssi_cbk;

    ret = sle_connection_register_callbacks(&conn_cbks);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s connection_register_callbacks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

static void ssaps_read_request_cbk(uint8_t server_id, uint16_t conn_id,
    ssaps_req_read_cb_t *read_cb_para, errcode_t status)
{
    if (read_cb_para == NULL) {
        return;
    }

    osal_printk("%s read_request server_id:0x%x, conn_id:0x%x, handle:0x%x, "
                "type:0x%x, status:0x%x\r\n",
                SLE_1VN_SERVER_LOG, server_id, conn_id, read_cb_para->handle,
                read_cb_para->type, status);

    if (read_cb_para->need_rsp) {
        (void)sle_1vn_send_ssaps_response(server_id, conn_id, read_cb_para->request_id,
                                          g_sle_property_value, g_sle_property_value_len);
    }
}

static void sle_1vn_store_property_value(const uint8_t *data, uint16_t len)
{
    uint16_t copy_len = len < sizeof(g_sle_property_value) ? len : sizeof(g_sle_property_value);

    if ((data == NULL) || (copy_len == 0)) {
        return;
    }

    if (memcpy_s(g_sle_property_value, sizeof(g_sle_property_value), data, copy_len) == EOK) {
        g_sle_property_value_len = copy_len;
    }
}

static void sle_1vn_print_write_value(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    osal_printk("%s write_request len:%u data:", SLE_1VN_SERVER_LOG, len);
    for (i = 0; i < len; i++) {
        osal_printk(" 0x%02x", data[i]);
    }
    osal_printk("\r\n");
}

static void wearable_sanitize_text(char *text)
{
    uint32_t i;
    uint32_t j = 0;

    if (text == NULL) {
        return;
    }

    for (i = 0; text[i] != '\0'; i++) {
        if ((text[i] == '\r') || (text[i] == '\n')) {
            continue;
        }
        text[j++] = text[i];
    }
    text[j] = '\0';
}

static void ssaps_write_request_cbk(uint8_t server_id, uint16_t conn_id,
    ssaps_req_write_cb_t *write_cb_para, errcode_t status)
{
    (void)status;

    const uint8_t id_prefix[] = {'I', 'D', ':'};
    const uint8_t unbind_prefix[] = {'U', 'N', 'B', 'I', 'N', 'D'};
    uint8_t *value;
    uint16_t length;

    if (write_cb_para == NULL) {
        return;
    }

    if ((write_cb_para->value == NULL) || (write_cb_para->length == 0)) {
        if (write_cb_para->need_rsp) {
            (void)sle_1vn_send_ssaps_response(server_id, conn_id, write_cb_para->request_id, NULL, 0);
        }
        return;
    }

    value = write_cb_para->value;
    length = write_cb_para->length;
    sle_1vn_store_property_value(value, length);
    sle_1vn_print_write_value(value, length);

    if (write_cb_para->need_rsp) {
        (void)sle_1vn_send_ssaps_response(server_id, conn_id, write_cb_para->request_id,
                                          g_sle_property_value, g_sle_property_value_len);
    }

    if ((length >= sizeof(unbind_prefix)) && (memcmp(value, unbind_prefix, sizeof(unbind_prefix)) == 0)) {
        osal_printk("%s ===== UNBIND Request Received =====\r\n", SLE_1VN_SERVER_LOG);
        (void)memset_s(&g_wearable_identity, sizeof(wearable_identity_t),
                       0, sizeof(wearable_identity_t));
        osal_printk("%s identity cleared\r\n", SLE_1VN_SERVER_LOG);

        char *ack = "UNBOUND";
        sle_1vn_server_send_data((uint8_t *)ack, (uint16_t)strlen(ack));
        osal_printk("%s >>> unbind reply sent\r\n", SLE_1VN_SERVER_LOG);
    } else if ((length >= sizeof(id_prefix)) && (memcmp(value, id_prefix, sizeof(id_prefix)) == 0)) {
        char write_text[128] = {0};
        uint16_t copy_len = length < (sizeof(write_text) - 1) ? length : (sizeof(write_text) - 1);
        (void)memcpy_s(write_text, sizeof(write_text), value, copy_len);
        char *name_start = write_text + sizeof(id_prefix);
        char *comma = strstr(name_start, ",");
        char name_buf[WEARABLE_NAME_MAX_LEN] = {0};
        char id_buf[WEARABLE_ID_MAX_LEN] = {0};
        if (comma != NULL) {
            (void)strncpy_s(name_buf, WEARABLE_NAME_MAX_LEN, name_start,
                            (uint32_t)(comma - name_start));
            (void)strncpy_s(id_buf, WEARABLE_ID_MAX_LEN, comma + 1,
                            WEARABLE_ID_MAX_LEN - 1);
        } else {
            (void)strncpy_s(name_buf, WEARABLE_NAME_MAX_LEN, name_start,
                            WEARABLE_NAME_MAX_LEN - 1);
        }
        wearable_sanitize_text(name_buf);
        wearable_sanitize_text(id_buf);
        wearable_on_identity_received(name_buf, id_buf);
        char *ack = "IDENTITY_OK";
        sle_1vn_server_send_data((uint8_t *)ack, (uint16_t)strlen(ack));
    } else if (value[0] == '{') {
        osal_printk("%s ===== Measurement Report Received =====\r\n", SLE_1VN_SERVER_LOG);
        osal_printk("%s report len:%u\r\n", SLE_1VN_SERVER_LOG, length);
        char *ack = "REPORT_OK";
        sle_1vn_server_send_data((uint8_t *)ack, (uint16_t)strlen(ack));
    } else {
        char *data = "Response from SLE server!\n";
        sle_1vn_server_send_data((uint8_t *)data, (uint16_t)strlen(data));
    }
}

static errcode_t sle_1vn_ssaps_register_cbks(void)
{
    errcode_t ret;
    ssaps_callbacks_t ssaps_cbk = {0};
    ssaps_cbk.add_service_cb       = ssaps_add_service_cbk;
    ssaps_cbk.add_property_cb      = ssaps_add_property_cbk;
    ssaps_cbk.add_descriptor_cb    = ssaps_add_descriptor_cbk;
    ssaps_cbk.start_service_cb     = ssaps_start_service_cbk;
    ssaps_cbk.delete_all_service_cb = ssaps_delete_all_service_cbk;
    ssaps_cbk.mtu_changed_cb       = ssaps_mtu_changed_cbk;
    ssaps_cbk.read_request_cb      = ssaps_read_request_cbk;
    ssaps_cbk.write_request_cb     = ssaps_write_request_cbk;
    ret = ssaps_register_callbacks(&ssaps_cbk);
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s ssaps_register_callbacks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_1vn_server_init(void)
{
    errcode_t ret;

    ret = sle_1vn_announce_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s announce_register_cbks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    ret = sle_1vn_conn_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s conn_register_cbks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    ret = sle_1vn_ssaps_register_cbks();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s ssaps_register_cbks fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }

    ret = sle_1vn_server_sample_dev_cbk_register();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s dev_cbk_register fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }

    osal_printk("%s init ok\r\n", SLE_1VN_SERVER_LOG);
    return ERRCODE_SLE_SUCCESS;
}

errcode_t sle_1vn_enable_server_cbk(void)
{
    errcode_t ret;

    ret = sle_1vn_server_add();
    if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s server_add fail :%x\r\n", SLE_1VN_SERVER_LOG, ret);
        return ret;
    }
    return ERRCODE_SLE_SUCCESS;
}

int sle_1vn_server_send_data(uint8_t *data, uint16_t length)
{
    return sle_1vn_server_send_notify_by_handle(data, length);
}

static int sle_1vn_server_send_report_fragments(const char *report, uint16_t length)
{
    uint16_t msg_id;
    uint16_t total;
    uint16_t seq;

    if ((report == NULL) || (length == 0)) {
        return ERRCODE_SLE_FAIL;
    }

    msg_id = ++g_sle_report_msg_id;
    total = (uint16_t)((length + SLE_REPORT_FRAGMENT_LEN - 1) / SLE_REPORT_FRAGMENT_LEN);
    if (total == 0) {
        return ERRCODE_SLE_FAIL;
    }

    for (seq = 0; seq < total; seq++) {
        char packet[SLE_REPORT_FRAGMENT_PACKET_LEN] = {0};
        uint16_t offset = (uint16_t)(seq * SLE_REPORT_FRAGMENT_LEN);
        uint16_t chunk_len = (uint16_t)(length - offset);
        int header_len;
        int packet_len;
        int ret;

        if (chunk_len > SLE_REPORT_FRAGMENT_LEN) {
            chunk_len = SLE_REPORT_FRAGMENT_LEN;
        }

        header_len = snprintf_s(packet, sizeof(packet), sizeof(packet) - 1,
                                "RPT:%u:%u:%u:",
                                (unsigned int)msg_id, (unsigned int)seq,
                                (unsigned int)total);
        if (header_len <= 0) {
            osal_printk("%s report fragment build failed msg=%u seq=%u total=%u\r\n",
                        SLE_1VN_SERVER_LOG, (unsigned int)msg_id,
                        (unsigned int)seq, (unsigned int)total);
            return ERRCODE_SLE_FAIL;
        }
        if (((uint16_t)header_len + chunk_len) >= sizeof(packet)) {
            osal_printk("%s report fragment too large msg=%u seq=%u header=%d chunk=%u\r\n",
                        SLE_1VN_SERVER_LOG, (unsigned int)msg_id,
                        (unsigned int)seq, header_len, (unsigned int)chunk_len);
            return ERRCODE_SLE_FAIL;
        }
        if (memcpy_s(packet + header_len, sizeof(packet) - (uint32_t)header_len,
                     report + offset, chunk_len) != EOK) {
            return ERRCODE_SLE_FAIL;
        }
        packet_len = header_len + chunk_len;
        packet[packet_len] = '\0';

        ret = sle_1vn_server_send_notify_by_handle((const uint8_t *)packet, (uint16_t)packet_len);
        osal_printk("%s report fragment msg=%u seq=%u total=%u chunk=%u packet=%d ret=0x%x\r\n",
                    SLE_1VN_SERVER_LOG, (unsigned int)msg_id, (unsigned int)seq,
                    (unsigned int)total, (unsigned int)chunk_len, packet_len, (unsigned int)ret);
        if (ret != ERRCODE_SLE_SUCCESS) {
            return ret;
        }

        osal_msleep(SLE_REPORT_FRAGMENT_INTERVAL_MS);
    }

    return ERRCODE_SLE_SUCCESS;
}

int sle_1vn_server_send_report(const char *report, uint16_t length)
{
    int ret;

    if (report == NULL || length == 0) {
        return ERRCODE_SLE_FAIL;
    }

    if (length > SLE_REPORT_SAFE_SINGLE_LEN) {
        ret = sle_1vn_server_send_report_fragments(report, length);
        osal_printk("%s report fragmented len=%u ready=%u mtu=%u ret=0x%x\r\n",
                    SLE_1VN_SERVER_LOG, (unsigned int)length,
                    (unsigned int)g_sle_report_ready, (unsigned int)g_sle_report_mtu,
                    (unsigned int)ret);
        return ret;
    }

    ret = sle_1vn_server_send_notify_by_handle((const uint8_t *)report, length);
    osal_printk("%s report notify len=%u ready=%u mtu=%u ret=0x%x\r\n",
                SLE_1VN_SERVER_LOG, (unsigned int)length,
                (unsigned int)g_sle_report_ready, (unsigned int)g_sle_report_mtu,
                (unsigned int)ret);
    return ret;
}

wearable_identity_t *wearable_get_identity(void)
{
    return &g_wearable_identity;
}

void wearable_register_identity_callback(wearable_identity_callback_t cb)
{
    g_wearable_identity_cb = cb;
}

void wearable_on_identity_received(const char *name, const char *id_number)
{
    if (name == NULL || id_number == NULL) {
        return;
    }

    (void)strncpy_s(g_wearable_identity.name, WEARABLE_NAME_MAX_LEN,
                    name, WEARABLE_NAME_MAX_LEN - 1);
    (void)strncpy_s(g_wearable_identity.id_number, WEARABLE_ID_MAX_LEN,
                    id_number, WEARABLE_ID_MAX_LEN - 1);
    g_wearable_identity.identity_received = 1;
    g_wearable_identity.sequence = ++g_wearable_identity_sequence;

    osal_printk("%s ===== NFC Identity Stored =====\r\n", SLE_1VN_SERVER_LOG);
    osal_printk("%s Name: %s\r\n", SLE_1VN_SERVER_LOG, g_wearable_identity.name);
    osal_printk("%s ID:   %s\r\n", SLE_1VN_SERVER_LOG, g_wearable_identity.id_number);

    if (g_wearable_identity_cb != NULL) {
        g_wearable_identity_cb(name, id_number);
    }

    char reply[128];
    sprintf_s(reply, sizeof(reply), "CONNECTED:%s,%s", name, id_number);
    sle_1vn_server_send_data((uint8_t *)reply, (uint16_t)strlen(reply));
    osal_printk("%s >>> connected reply sent\r\n", SLE_1VN_SERVER_LOG);
}

void sle_1vn_wait_client_paired(void)
{
    while (g_sle_pair_state != SLE_PAIR_PAIRED) {
        osal_msleep(100);
    }
}

void sle_1vn_wait_client_connected(void)
{
    while (g_sle_conn_state != SLE_ACB_STATE_CONNECTED) {
        osal_msleep(100);
    }
}
