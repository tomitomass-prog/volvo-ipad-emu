#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "tusb.h"
#include "iap_protocol.h"

static const char *TAG = "IAP_PROTO";

// Bufor przechowujący aktualny tytuł i wykonawcę z Bluetooth (domyślnie 1 utwór w bazie)
static char current_title[128] = "Volvo BT Audio";
static char current_artist[128] = "Bluetooth Stream";

void iap_update_track_title(const char *title) {
    if (title && strlen(title) > 0) {
        snprintf(current_title, sizeof(current_title), "%s", title);
        ESP_LOGI(TAG, "Zaktualizowano tytul: %s", current_title);
    }
}

void iap_update_track_artist(const char *artist) {
    if (artist && strlen(artist) > 0) {
        snprintf(current_artist, sizeof(current_artist), "%s", artist);
        ESP_LOGI(TAG, "Zaktualizowano artyste: %s", current_artist);
    }
}

// Obliczanie sumy kontrolnej iAP (negacja sumy bajtów od długości do końca ładunku)
static uint8_t iap_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(0x100 - sum);
}

// Wysyłanie odpowiedzi iAP po USB HID (dobieramy odpowiedni Report ID do długości ramki)
static void iap_send_packet(uint8_t lingo, uint16_t cmd, const uint8_t *payload, uint8_t payload_len) {
    uint8_t packet[260] = {0};
    uint8_t report_id = 2; // Domyślnie Report ID 2 (do 63 bajtów)
    uint8_t report_size = 63;

    // Nagłówek iAP: 0x00 (Link Control) + 0x55 (Sync) + Len + Lingo + Cmd
    size_t idx = 0;
    packet[idx++] = 0x00; // HID Link Control Byte
    packet[idx++] = 0x55; // Sync Byte

    if (lingo == 0x04) {
        uint8_t len = 3 + payload_len; // Lingo (1) + Cmd (2) + Payload
        packet[idx++] = len;
        packet[idx++] = lingo;
        packet[idx++] = (cmd >> 8) & 0xFF;
        packet[idx++] = cmd & 0xFF;
    } else {
        uint8_t len = 2 + payload_len; // Lingo (1) + Cmd (1) + Payload
        packet[idx++] = len;
        packet[idx++] = lingo;
        packet[idx++] = cmd & 0xFF;
    }

    if (payload && payload_len > 0) {
        memcpy(&packet[idx], payload, payload_len);
        idx += payload_len;
    }

    // Suma kontrolna liczona od bajtu długości (indeks 2)
    packet[idx] = iap_checksum(&packet[2], idx - 2);
    idx++;

    // Dobór Report ID zależnie od całkowitej długości pakietu
    if (idx <= 12) {
        report_id = 1;
        report_size = 12;
    } else if (idx <= 63) {
        report_id = 2;
        report_size = 63;
    } else if (idx <= 127) {
        report_id = 3;
        report_size = 127;
    } else {
        report_id = 4;
        report_size = 255;
    }

    if (tud_hid_ready()) {
        tud_hid_report(report_id, packet, report_size);
    }
}

void iap_process_hid_packet(uint8_t report_id, const uint8_t *buffer, uint16_t bufsize, int uart_num) {
    (void) report_id;
    if (bufsize < 5) return;

    // Szukamy bajtu synchronizacji 0x55
    size_t start = 0;
    while (start < bufsize && buffer[start] != 0x55) {
        start++;
    }
    if (start + 4 >= bufsize) return;

    uint8_t len   = buffer[start + 1];
    uint8_t lingo = buffer[start + 2];
    (void)len;

    // --- LINGO 0x00: General Lingo (Inicjalizacja połączenia z radiem Volvo) ---
    if (lingo == 0x00) {
        uint8_t cmd = buffer[start + 3];

        if (cmd == 0x01 || cmd == 0x0F || cmd == 0x13) {
            // Identify / IdentifyDeviceLingoes -> Odpowiadamy ACK (0x02)
            uint8_t ack[2] = { 0x00, cmd };
            iap_send_packet(0x00, 0x02, ack, sizeof(ack));
        } else if (cmd == 0x07) {
            // RequestiPodName -> Zwracamy nazwę "Volvo iPod"
            const char *name = "Volvo iPod";
            iap_send_packet(0x00, 0x08, (const uint8_t *)name, strlen(name) + 1);
        } else if (cmd == 0x09) {
            // RequestiPodSoftwareVersion -> Zwracamy wersję 2.0.1
            uint8_t ver[3] = { 0x02, 0x00, 0x01 };
            iap_send_packet(0x00, 0x0A, ver, sizeof(ver));
        } else if (cmd == 0x0B) {
            // RequestiPodSerialNum -> Zwracamy numer seryjny
            const char *serial = "000000000001";
            iap_send_packet(0x00, 0x0C, (const uint8_t *)serial, strlen(serial) + 1);
        } else if (cmd == 0x0D) {
            // RequestiPodModelNum -> Zwracamy model iPod Touch / Classic
            uint8_t model[10] = { 0x00, 0x0B, 0x00, 0x00, 'P', 'A', '2', '9', '3', 0x00 };
            iap_send_packet(0x00, 0x0E, model, sizeof(model));
        } else if (cmd == 0x27) {
            // GetDevAuthenticationInfo -> Odpowiadamy pustym certyfikatem (tryb bez chipa MFi)
            uint8_t auth[4] = { 0x02, 0x00, 0x00, 0x00 };
            iap_send_packet(0x00, 0x28, auth, sizeof(auth));
        } else if (cmd == 0x29) {
            // RetDevAuthenticationSignature -> Potwierdzamy ACK
            uint8_t ack[2] = { 0x00, 0x29 };
            iap_send_packet(0x00, 0x02, ack, sizeof(ack));
        }
    }

    // --- LINGO 0x04: Extended Interface Lingo (Baza utworów MP3 i sterowanie) ---
    else if (lingo == 0x04) {
        if (start + 4 >= bufsize) return;
        uint16_t cmd = (buffer[start + 3] << 8) | buffer[start + 4];

        // 0x0018: GetNumberCategorizedDBRecords (Volvo pyta, ile jest piosenek w pamięci!)
        if (cmd == 0x0018) {
            // Zawsze zgłaszamy 1 utwór w pamięci (0x00000001), aby odblokować iPoda w Volvo
            uint8_t count_resp[4] = { 0x00, 0x00, 0x00, 0x01 };
            iap_send_packet(0x04, 0x0019, count_resp, sizeof(count_resp));
        }
        // 0x001A: RetrieveCategorizedDatabaseRecords (Volvo pobiera nazwę naszego 1 wirtualnego MP3)
        else if (cmd == 0x001A) {
            uint8_t resp[140] = {0};
            resp[0] = 0x00; resp[1] = 0x00; resp[2] = 0x00; resp[3] = 0x00; // Indeks utworu: 0
            size_t tlen = strlen(current_title) + 1;
            memcpy(&resp[4], current_title, tlen);
            iap_send_packet(0x04, 0x001B, resp, 4 + tlen);
        }
        // 0x001C: GetPlayStatus (Stan odtwarzania, długość utworu, aktualna pozycja)
        else if (cmd == 0x001C) {
            uint8_t status[9] = {
                0x00, 0x03, 0xA9, 0x80, // Długość utworu: 240 000 ms (4 minuty)
                0x00, 0x00, 0x27, 0x10, // Aktualna pozycja: 10 000 ms
                0x01                    // Status: 0x01 = Playing
            };
            iap_send_packet(0x04, 0x001D, status, sizeof(status));
        }
        // 0x0020: GetIndexedPlayingTrackTitle (Volvo prosi o tytuł aktualnie granej piosenki)
        else if (cmd == 0x0020) {
            iap_send_packet(0x04, 0x0021, (const uint8_t *)current_title, strlen(current_title) + 1);
        }
        // 0x0022: GetIndexedPlayingTrackArtistName (Volvo prosi o nazwę wykonawcy)
        else if (cmd == 0x0022) {
            iap_send_packet(0x04, 0x0023, (const uint8_t *)current_artist, strlen(current_artist) + 1);
        }
        // 0x0024: GetIndexedPlayingTrackAlbumName (Volvo prosi o nazwę albumu)
        else if (cmd == 0x0024) {
            const char *album = "Bluetooth Audio";
            iap_send_packet(0x04, 0x0025, (const uint8_t *)album, strlen(album) + 1);
        }
        // 0x0029: PlayControl (Przyciski na kierownicy Volvo: Play/Pause, Next, Prev)
        else if (cmd == 0x0029 && start + 5 < bufsize) {
            uint8_t action = buffer[start + 5];
            const char *uart_cmd = NULL;

            if (action == 0x01) {
                uart_cmd = "CMD:PLAY\n";
            } else if (action == 0x02) {
                uart_cmd = "CMD:PAUSE\n";
            } else if (action == 0x03) {
                uart_cmd = "CMD:NEXT\n";
            } else if (action == 0x04) {
                uart_cmd = "CMD:PREV\n";
            }

            if (uart_cmd != NULL) {
                uart_write_bytes(uart_num, uart_cmd, strlen(uart_cmd));
            }
            // Potwierdzenie wykonania komendy (ACK 0x0001)
            uint8_t ack[3] = { 0x00, 0x00, 0x29 };
            iap_send_packet(0x04, 0x0001, ack, sizeof(ack));
        }
        // Pozostałe komendy z Lingo 0x04 (np. ResetDBSelection, PlayCurrentSelection) -> ACK
        else {
            uint8_t ack[3] = { 0x00, (cmd >> 8) & 0xFF, cmd & 0xFF };
            iap_send_packet(0x04, 0x0001, ack, sizeof(ack));
        }
    }
}
