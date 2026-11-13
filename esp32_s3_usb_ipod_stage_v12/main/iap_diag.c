#include "iap_diag.h"
#include <string.h>

/* Classic iAP1 frame: [optional 00] 55 LEN LINGO CMD DATA CHECKSUM.
 * LEN includes LINGO + CMD + DATA; sum(LEN..CHECKSUM) == 0 mod 256.
 * Diagnostic v1.2 intentionally supports 1-byte General Lingo commands only.
 */
bool iap_decode(const uint8_t *buf, size_t len, iap_frame_t *out) {
    if (!buf || !out || len < 5) return false;
    /* HID payloads may have a 00 link-control byte before sync. */
    size_t at = (buf[0] == 0x00 && len > 1) ? 1u : 0u;
    if (buf[at] != 0x55 || len - at < 5u) return false;
    uint8_t body_len = buf[at + 1u];
    if (body_len < 2u || body_len > 58u) return false;
    size_t needed = at + 2u + (size_t)body_len + 1u;
    if (needed > len) return false;
    uint8_t sum = 0;
    for (size_t i = at + 1u; i < needed; ++i) sum = (uint8_t)(sum + buf[i]);
    if (sum != 0) return false;
    out->lingo = buf[at + 2u];
    out->command = buf[at + 3u];
    out->payload_len = (size_t)body_len - 2u;
    if (out->payload_len > sizeof(out->payload)) return false;
    if (out->payload_len) memcpy(out->payload, buf + at + 4u, out->payload_len);
    return true;
}

static bool frame_reply(uint8_t command, const uint8_t *data,
                        size_t size, iap_tx_t *out) {
    if (!out || size > IAP_MAX_FRAME - 6u) return false;
    memset(out, 0, sizeof(*out));
    out->bytes[0] = 0x00;        /* iAP HID link control */
    out->bytes[1] = 0x55;        /* frame sync */
    out->bytes[2] = (uint8_t)(2u + size);
    out->bytes[3] = 0x00;        /* General Lingo */
    out->bytes[4] = command;
    if (size) memcpy(out->bytes + 5, data, size);
    size_t last = 5u + size;
    uint8_t sum = 0;
    for (size_t i = 2u; i < last; ++i) sum = (uint8_t)(sum + out->bytes[i]);
    out->bytes[last] = (uint8_t)(0u - sum);
    out->len = last + 1u;
    return true;
}

bool iap_identity_reply(const iap_frame_t *in, iap_tx_t *out) {
    if (!in || !out || in->lingo != 0x00 || in->payload_len != 0) return false;
    /* Supported minimal iAP1 identity query/response command pairs
       per Rockbox's General Lingo implementation. */
    static const uint8_t name[] = "Volvo BT Prototype"; /* includes NUL */
    static const uint8_t sw[] = {2, 0, 1};
    static const uint8_t serial[] = "S3PROTO12";
    static const uint8_t model[] = {0, 0, 0, 0, 'P', 'R', 'O', 'T', 'O', 0};
    switch (in->command) {
        case 0x07: return frame_reply(0x08, name, sizeof(name), out);
        case 0x09: return frame_reply(0x0A, sw, sizeof(sw), out);
        case 0x0B: return frame_reply(0x0C, serial, sizeof(serial), out);
        case 0x0D: return frame_reply(0x0E, model, sizeof(model), out);
        default: return false;
    }
}
