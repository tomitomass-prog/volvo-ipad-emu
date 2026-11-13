#ifndef IAP_DIAG_H
#define IAP_DIAG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define IAP_MAX_FRAME 63
typedef struct {
    uint8_t lingo, command;
    uint8_t payload[56];
    size_t payload_len;
} iap_frame_t;
typedef struct {
    uint8_t bytes[IAP_MAX_FRAME];
    size_t len;
} iap_tx_t;
bool iap_decode(const uint8_t *buf, size_t len, iap_frame_t *out);
bool iap_identity_reply(const iap_frame_t *in, iap_tx_t *out);
#endif
