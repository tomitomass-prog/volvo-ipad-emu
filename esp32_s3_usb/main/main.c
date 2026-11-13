#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb.h"
#include "class/audio/audio_device.h"
#include "usb_descriptors.h"
#include "iap_protocol.h"

// Piny I2S Slave (odbior dzwieku z ESP32 Classic)
#define I2S_BCK_IO      4
#define I2S_WS_IO       5
#define I2S_DO_IO       6

// Piny UART (komunikacja tekstowa z ESP32 Classic)
#define UART_TX_PIN     7
#define UART_RX_PIN     8

static i2s_chan_handle_t rx_handle = NULL;

// Callback wywolywany przez TinyUSB gdy Volvo wysyla ramke HID (iAP)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    iap_process_hid_packet(buffer, bufsize);
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

// Konfiguracja I2S Slave do odbioru strumienia 44.1kHz 16-bit Stereo z ESP32 Classic
static void init_i2s_slave(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_SLAVE);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DO_IO,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}

// Task przesylajacy probki audio z I2S na USB Audio Class 1.0 (EP 0x81)
static void audio_usb_task(void *arg) {
    uint8_t i2s_buf[176 * 4]; // 4ms audio (176 bajtow na 1ms przy 44.1kHz 16-bit stereo)
    size_t bytes_read = 0;

    while (1) {
        if (i2s_channel_read(rx_handle, i2s_buf, sizeof(i2s_buf), &bytes_read, pdMS_TO_TICKS(10)) == ESP_OK) {
            if (bytes_read > 0 && tud_mounted()) {
                tud_audio_write(i2s_buf, bytes_read);
            }
        }
    }
}

// Task odbierajacy tytuly piosenek z ESP32 Classic po UART
static void uart_metadata_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint8_t line[128];
    char title[64] = {0};
    char artist[64] = {0};

    while (1) {
        int len = uart_read_bytes(UART_NUM_1, line, sizeof(line) - 1, pdMS_TO_TICKS(100));
        // UART reads may split or merge lines; assemble until newline.
        static char pending[160];
        static size_t used = 0;
        for (int i = 0; i < len; ++i) {
            char c = (char)line[i];
            if (c == '\n') {
                pending[used] = '\0';
                if (strncmp(pending, "TITLE:", 6) == 0) {
                    size_t n = strnlen(pending + 6, sizeof(title) - 1);
                    memcpy(title, pending + 6, n);
                    title[n] = '\0';
                    iap_update_track_info(title, NULL);
                } else if (strncmp(pending, "ARTIST:", 7) == 0) {
                    size_t n = strnlen(pending + 7, sizeof(artist) - 1);
                    memcpy(artist, pending + 7, n);
                    artist[n] = '\0';
                    iap_update_track_info(NULL, artist);
                }
                used = 0;
            } else if (c != '\r' && used < sizeof(pending) - 1) pending[used++] = c;
        }
    }
}

void app_main(void) {
    iap_init();
    init_i2s_slave();

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &desc_device,
        .string_descriptor = string_desc_arr,
        .string_descriptor_count = 4,
        .external_phy = false,
        .configuration_descriptor = desc_configuration,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    xTaskCreate(audio_usb_task, "audio_usb_task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_metadata_task, "uart_meta_task", 4096, NULL, 4, NULL);
}
