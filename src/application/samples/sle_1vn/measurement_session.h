#ifndef MEASUREMENT_SESSION_H
#define MEASUREMENT_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#define MEASUREMENT_ITEM_HEIGHT   (1U << 0)
#define MEASUREMENT_ITEM_WEIGHT   (1U << 1)
#define MEASUREMENT_ITEM_BP       (1U << 2)
#define MEASUREMENT_ITEM_GLUCOSE  (1U << 3)

#define MEASUREMENT_REQUIRED_MASK \
    (MEASUREMENT_ITEM_HEIGHT | MEASUREMENT_ITEM_WEIGHT | MEASUREMENT_ITEM_BP | MEASUREMENT_ITEM_GLUCOSE)

#define MEASUREMENT_TEXT_LEN      32
#define MEASUREMENT_NAME_LEN      32
#define MEASUREMENT_PHONE_LEN     20
#define MEASUREMENT_ID_CARD_LEN   24
#define MEASUREMENT_DEVICE_ID_LEN 32
#define MEASUREMENT_REPORT_LEN    768

typedef struct {
    char name[MEASUREMENT_NAME_LEN];
    char phone[MEASUREMENT_PHONE_LEN];
    char id_card[MEASUREMENT_ID_CARD_LEN];
    char device_id[MEASUREMENT_DEVICE_ID_LEN];
    char height[MEASUREMENT_TEXT_LEN];
    char weight[MEASUREMENT_TEXT_LEN];
    char bmi[MEASUREMENT_TEXT_LEN];
    char blood_pressure[MEASUREMENT_TEXT_LEN];
    char glucose[MEASUREMENT_TEXT_LEN];
    uint32_t required_mask;
    uint32_t received_mask;
} measurement_session_t;

void measurement_session_init(measurement_session_t *session);
void measurement_session_set_identity(measurement_session_t *session, const char *name, const char *id_card);
bool measurement_session_is_complete(const measurement_session_t *session);
void measurement_session_mark_value(measurement_session_t *session, uint32_t item_mask, const char *value);
int measurement_session_build_report(const measurement_session_t *session, char *buffer, uint32_t buffer_len);

#endif
