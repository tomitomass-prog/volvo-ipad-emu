#ifndef IAP_PROTOCOL_H_
#define IAP_PROTOCOL_H_

#include <stdint.h>
#include <stddef.h>

void iap_init(void);
void iap_process_hid_packet(const uint8_t *buffer, uint16_t bufsize);
void iap_update_track_info(const char *title, const char *artist);

#endif
