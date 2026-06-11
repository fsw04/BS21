#include "measurement_session.h"

#include <string.h>
#include "securec.h"
#include "sle_1vn_server_adv.h"

static void measurement_copy_text(char *dst, uint32_t dst_len, const char *src)
{
    if ((dst == NULL) || (dst_len == 0)) {
        return;
    }
    dst[0] = '\0';
    if (src == NULL) {
        return;
    }
    (void)strncpy_s(dst, dst_len, src, dst_len - 1);
}

void measurement_session_init(measurement_session_t *session)
{
    if (session == NULL) {
        return;
    }

    (void)memset_s(session, sizeof(measurement_session_t), 0, sizeof(measurement_session_t));
    session->required_mask = MEASUREMENT_REQUIRED_MASK;

    const char *local_name = sle_1vn_get_local_name();
    if (local_name != NULL) {
        measurement_copy_text(session->device_id, sizeof(session->device_id), local_name);
    } else {
        measurement_copy_text(session->device_id, sizeof(session->device_id), "watch_unknown");
    }
}

void measurement_session_set_identity(measurement_session_t *session, const char *name, const char *id_card)
{
    if (session == NULL) {
        return;
    }

    measurement_copy_text(session->name, sizeof(session->name), name);
    measurement_copy_text(session->phone, sizeof(session->phone), "");
    measurement_copy_text(session->id_card, sizeof(session->id_card), id_card);
}

bool measurement_session_is_complete(const measurement_session_t *session)
{
    if (session == NULL) {
        return false;
    }
    return (session->received_mask & session->required_mask) == session->required_mask;
}

static void measurement_update_bmi(measurement_session_t *session)
{
    if ((session == NULL) || (session->height[0] == '\0') || (session->weight[0] == '\0')) {
        return;
    }

    /* Keep the current demo deterministic until real sensor values are parsed as numbers. */
    measurement_copy_text(session->bmi, sizeof(session->bmi), "21.5");
}

void measurement_session_mark_value(measurement_session_t *session, uint32_t item_mask, const char *value)
{
    if ((session == NULL) || (value == NULL)) {
        return;
    }

    switch (item_mask) {
        case MEASUREMENT_ITEM_HEIGHT:
            measurement_copy_text(session->height, sizeof(session->height), value);
            break;
        case MEASUREMENT_ITEM_WEIGHT:
            measurement_copy_text(session->weight, sizeof(session->weight), value);
            break;
        case MEASUREMENT_ITEM_BP:
            measurement_copy_text(session->blood_pressure, sizeof(session->blood_pressure), value);
            break;
        case MEASUREMENT_ITEM_GLUCOSE:
            measurement_copy_text(session->glucose, sizeof(session->glucose), value);
            break;
        default:
            return;
    }

    session->received_mask |= item_mask;
    measurement_update_bmi(session);
}

int measurement_session_build_report(const measurement_session_t *session, char *buffer, uint32_t buffer_len)
{
    if ((session == NULL) || (buffer == NULL) || (buffer_len == 0)) {
        return -1;
    }

    return sprintf_s(buffer, buffer_len,
        "{"
        "\"deviceId\":\"%s\","
        "\"type\":\"vitals\","
        "\"complete\":%s,"
        "\"receivedMask\":%u,"
        "\"requiredMask\":%u,"
        "\"data\":{"
            "\"name\":\"%s\","
            "\"phone\":\"%s\","
            "\"idCard\":\"%s\","
            "\"height\":\"%s\","
            "\"weight\":\"%s\","
            "\"bmi\":\"%s\","
            "\"bloodPressure\":\"%s\","
            "\"fastingBloodGlucose\":\"%s\""
        "}"
        "}",
        session->device_id,
        measurement_session_is_complete(session) ? "true" : "false",
        session->received_mask,
        session->required_mask,
        session->name,
        session->phone,
        session->id_card,
        session->height,
        session->weight,
        session->bmi,
        session->blood_pressure,
        session->glucose);
}
