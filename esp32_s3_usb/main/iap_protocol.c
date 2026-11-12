#include <string.h>
#include <stdio.h>
#include "iap_protocol.h"
#include "tusb.h"
#include "driver/uart.h"

// Wirtualna baza danych (1 utwor w pamieci, aby radio Volvo wykrylo zawartosc)
static char current_title[64]  = "Bluetooth Audio Stream";
static char current_artist[64] = "Volvo P3 Bridge";
static uint8_t play_state = 0x01; // 0x01 = Playing

void iap_init(void) {
    // Inicjalizacja wirtualnego utworu
}

void iap_update_track_info(const char *title, const char *artist) {
    if (title && strlen(title) > 0) {
        strncpy(current_title, title, sizeof(current_title) - 1);
        current_title[sizeof(current_title) - 1] = '\0';
    }
    if (artist && strlen(artist) > 0) {
        strncpy(current_artist, artist, sizeof(current_artist) - 1);
        current_artist[sizeof(current_artist) - 1] = '\0';
    }
}

// Obliczanie sumy kontrolnej ramki Apple iAP (0x100 - suma bajtow od Lingo)
static uint8_t iap_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(0x100 - sum);
}

// Wysylanie ramki iAP przez USB HID (dobiera odpowiedni Report ID do dlugosci)
static void iap_send_frame(uint8_t lingo, uint8_t cmd_id, const uint8_t *payload, uint16_t payload_len) {
    uint8_t frame[256];
    uint16_t idx = 0;

    // Naglowek HID Link + iAP Sync (0xFF 0x55)
    frame[idx++] = 0x00; // Link control byte
    frame[idx++] = 0xFF;
    frame[idx++] = 0x55;
    
    uint8_t iap_len = 2 + payload_len; // Lingo + CmdID + Payload
    frame[idx++] = iap_len;
    frame[idx++] = lingo;
    frame[idx++] = cmd_id;

    if (payload && payload_len > 0) {
        memcpy(&frame[idx], payload, payload_len);
        idx += payload_len;
    }

    // Suma kontrolna liczona od bajtu dlugosci (index 3)
    frame[idx] = iap_checksum(&frame[3], iap_len + 1);
    idx++;

    uint8_t report_id = 1;
    uint8_t report_size = 12;
    if (idx > 127) { report_id = 4; report_size = 255; }
    else if (idx > 63) { report_id = 3; report_size = 127; }
    else if (idx > 12) { report_id = 2; report_size = 63; }

    uint8_t hid_buf[256] = {0};
    memcpy(hid_buf, frame, idx);

    if (tud_hid_ready()) {
        tud_hid_report(report_id, hid_buf, report_size);
    }
}

// Wysylanie ACK (Potwierdzenia wykonania komendy)
static void iap_send_ack(uint8_t lingo, uint8_t cmd_id, uint8_t status) {
    uint8_t payload[2] = { status, cmd_id };
    if (lingo == 0x00) {
        iap_send_frame(0x00, 0x02, payload, 2); // General Lingo ACK
    } else if (lingo == 0x04) {
        iap_send_frame(0x04, 0x01, payload, 2); // Extended Interface ACK
    }
}

// Parsowanie przychodzacych zadan z radia Volvo
void iap_process_hid_packet(const uint8_t *buffer, uint16_t bufsize) {
    // Szukamy znacznika poczatku ramki iAP (0xFF 0x55)
    int start = -1;
    for (int i = 0; i < bufsize - 4; i++) {
        if (buffer[i] == 0xFF && buffer[i+1] == 0x55) {
            start = i;
            break;
        }
    }
    if (start < 0) return;

    uint8_t len   = buffer[start + 2];
    uint8_t lingo = buffer[start + 3];
    uint8_t cmd   = buffer[start + 4];
    const uint8_t *params = &buffer[start + 5];

    // --- LINGO 0x00: GENERAL LINGO ---
    if (lingo == 0x00) {
        if (cmd == 0x01 || cmd == 0x13) { // Identify / IdentifyDeviceLingoes
            // Potwierdzamy identyfikacje (obslugujemy Lingo 0x00 i 0x04 bez autoryzacji MFi)
            iap_send_ack(0x00, cmd, 0x00);
        } else if (cmd == 0x03) { // RequestExtendedInterfaceMode
            uint8_t mode = 0x01; // Tryb Extended Interface aktywny
            iap_send_frame(0x00, 0x04, &mode, 1);
        } else if (cmd == 0x05) { // EnterExtendedInterfaceMode
            iap_send_ack(0x00, cmd, 0x00);
        } else if (cmd == 0x0D) { // RequestiPodName
            const char *name = "Volvo BT iPod";
            iap_send_frame(0x00, 0x0E, (const uint8_t*)name, strlen(name) + 1);
        } else {
            iap_send_ack(0x00, cmd, 0x00);
        }
    }
    // --- LINGO 0x04: EXTENDED INTERFACE (Baza utworow i sterowanie) ---
    else if (lingo == 0x04) {
        switch (cmd) {
            case 0x16: // ResetDBSelection (Volvo resetuje filtr bazy piosenek)
                iap_send_ack(0x04, cmd, 0x00);
                break;

            case 0x18: { // GetNumberCategorizedDBRecords (Kluczowe: radio pyta ile jest piosenek!)
                // Zwracamy 1 utwor (uint32 big-endian = 0x00000001), aby Volvo nie pokazalo "Brak utworow"
                uint8_t count[4] = { 0x00, 0x00, 0x00, 0x01 };
                iap_send_frame(0x04, 0x19, count, 4);
                break;
            }

            case 0x1A: { // RetrieveCategorizedDatabaseRecords (Radio pobiera nazwe wirtualnego MP3)
                // Indeks 0 (4 bajty) + Nazwa utworu zakonczona NULL
                uint8_t resp[80] = { 0x00, 0x00, 0x00, 0x00 };
                size_t tlen = strlen(current_title) + 1;
                memcpy(&resp[4], current_title, tlen);
                iap_send_frame(0x04, 0x1B, resp, 4 + tlen);
                break;
            }

            case 0x1C: { // GetPlayStatus (Radio pyta o dlugosc utworu, pozycje i stan Play/Pause)
                uint8_t status[9] = {
                    0x00, 0x03, 0xA9, 0x80, // Dlugosc utworu: 240000 ms (4 minuty)
                    0x00, 0x00, 0x27, 0x10, // Aktualna pozycja: 10000 ms
                    play_state              // 0x01 = Playing, 0x02 = Paused
                };
                iap_send_frame(0x04, 0x1D, status, 9);
                break;
            }

            case 0x1E: { // GetCurrentPlayingTrackIndex
                uint8_t idx[4] = { 0x00, 0x00, 0x00, 0x00 }; // Utwor nr 0
                iap_send_frame(0x04, 0x1F, idx, 4);
                break;
            }

            case 0x20: { // GetIndexedPlayingTrackTitle
                iap_send_frame(0x04, 0x21, (const uint8_t*)current_title, strlen(current_title) + 1);
                break;
            }

            case 0x22: { // GetIndexedPlayingTrackArtistName
                iap_send_frame(0x04, 0x23, (const uint8_t*)current_artist, strlen(current_artist) + 1);
                break;
            }

            case 0x24: { // GetIndexedPlayingTrackAlbumName
                const char *album = "Bluetooth Audio";
                iap_send_frame(0x04, 0x25, (const uint8_t*)album, strlen(album) + 1);
                break;
            }

            case 0x28: // PlayCurrentSelection (Volvo uruchamia odtwarzanie wirtualnego utworu)
                play_state = 0x01;
                iap_send_ack(0x04, cmd, 0x00);
                uart_write_bytes(UART_NUM_1, "CMD:PLAY\n", 9);
                break;

            case 0x29: { // PlayControl (Przyciski na kierownicy Volvo: Play/Pause, Next, Prev)
                uint8_t subcmd = params[0];
                if (subcmd == 0x01) { // Toggle Play/Pause
                    play_state = (play_state == 0x01) ? 0x02 : 0x01;
                    uart_write_bytes(UART_NUM_1, "CMD:TOGGLE\n", 11);
                } else if (subcmd == 0x03) { // Next Track
                    uart_write_bytes(UART_NUM_1, "CMD:NEXT\n", 9);
                } else if (subcmd == 0x04) { // Previous Track
                    uart_write_bytes(UART_NUM_1, "CMD:PREV\n", 9);
                }
                iap_send_ack(0x04, cmd, 0x00);
                break;
            }

            default:
                iap_send_ack(0x04, cmd, 0x00);
                break;
        }
    }
}
