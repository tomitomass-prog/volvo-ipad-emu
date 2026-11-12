#ifndef IAP_PROTOCOL_H_
#define IAP_PROTOCOL_H_

#include <stdint.h>

void iap_update_track_title(const char *title);
void iap_update_track_artist(const char *artist);
void iap_process_hid_packet(uint8_t report_id, const uint8_t *buffer, uint16_t bufsize, int uart_num);

#endif /* IAP_PROTOCOL_H_ */
