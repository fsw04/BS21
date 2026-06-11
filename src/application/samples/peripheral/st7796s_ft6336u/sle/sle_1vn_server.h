/*
 * BS21E SLE 1VN Server - Protocol definitions
 * Ported from WS63E sle_1vn_server.h, adapted to BS21E SDK.
 * Protocol MUST remain identical to WS63E server for cross-compatibility.
 */

#ifndef SLE_1VN_SERVER_H
#define SLE_1VN_SERVER_H

#include <stdbool.h>
#include <stdint.h>
#include "sle_ssap_server.h"
#include "errcode.h"

#define SLE_UUID_SERVER_SERVICE        0x060B
#define SLE_UUID_SERVER_NTF_REPORT     0x1122
#define SLE_UUID_TEST_PROPERTIES       (SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE)
#define SLE_UUID_TEST_OPERATION_INDICATION \
    (SSAP_OPERATE_INDICATION_BIT_READ | SSAP_OPERATE_INDICATION_BIT_WRITE \
     | SSAP_OPERATE_INDICATION_BIT_WRITE_NO_RSP | SSAP_OPERATE_INDICATION_BIT_NOTIFY)
#define SLE_UUID_TEST_DESCRIPTOR       (SSAP_PERMISSION_READ | SSAP_PERMISSION_WRITE)

#define WEARABLE_NAME_MAX_LEN   32
#define WEARABLE_ID_MAX_LEN     20

typedef struct {
    char name[WEARABLE_NAME_MAX_LEN];
    char id_number[WEARABLE_ID_MAX_LEN];
    uint8_t identity_received;
    uint32_t sequence;
} wearable_identity_t;

typedef void (*wearable_identity_callback_t)(const char *name, const char *id_number);

errcode_t sle_1vn_server_init(void);
errcode_t sle_1vn_server_send_notify_by_handle(const uint8_t *data, uint16_t len);
int sle_1vn_server_send_data(uint8_t *data, uint16_t length);
int sle_1vn_server_send_report(const char *report, uint16_t length);
bool sle_1vn_server_is_connected(void);
bool sle_1vn_server_report_is_ready(void);
uint32_t sle_1vn_server_report_mtu(void);
errcode_t sle_1vn_enable_server_cbk(void);
void sle_1vn_wait_client_paired(void);
void sle_1vn_wait_client_connected(void);

wearable_identity_t *wearable_get_identity(void);
void wearable_register_identity_callback(wearable_identity_callback_t cb);
void wearable_on_identity_received(const char *name, const char *id_number);

#endif
