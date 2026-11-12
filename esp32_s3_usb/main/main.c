#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/uart.h"
#include "tinyusb.h"
#include "tusb.h"
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
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bck = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DO_IO,
            .invert_flags = { .mclk_inv = false, .bck_inv = false, .ws_inv = false },
        },
    };
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    i2s_channel_enable(rx_handle);
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
    uart_driver_install(UART_NUM_1, 512, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uint8_t line[128];
    char title[64] = {0};
    char artist[64] = {0};

    while (1) {
        int len = uart_read_bytes(UART_NUM_1, line, sizeof(line) - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            line[len] = '\0';
            if (strncmp((char*)line, "TITLE:", 6) == 0) {
                strncpy(title, (char*)line + 6, sizeof(title) - 1);
                iap_update_track_info(title, NULL);
            } else if (strncmp((char*)line, "ARTIST:", 7) == 0) {
                strncpy(artist, (char*)line + 7, sizeof(artist) - 1);
                iap_update_track_info(NULL, artist);
            }
        }
    }
}

void app_main(void) {
    iap_init();
    init_i2s_slave();

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL, // Uzywa tud_descriptor_device_cb z usb_descriptors.c
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };
    tinyusb_driver_install(&tusb_cfg);

    xTaskCreate(audio_usb_task, "audio_usb_task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_metadata_task, "uart_meta_task", 4096, NULL, 4, NULL);
}
