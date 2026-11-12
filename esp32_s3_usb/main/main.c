#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb.h"
#include "class/audio/audio_device.h"
#include "usb_descriptors.h"
#include "iap_protocol.h"

static const char *TAG = "VOLVO_IPOD_S3";

// Konfiguracja pinów I2S Slave (odbiera dźwięk z ESP32 Classic)
#define I2S_BCK_IO      4
#define I2S_WS_IO       5
#define I2S_DI_IO       6

// Konfiguracja UART (komunikacja tekstowa z ESP32 Classic)
#define UART_TX_IO      7
#define UART_RX_IO      8
#define UART_PORT_NUM   UART_NUM_1

static i2s_chan_handle_t rx_handle = NULL;

// Inicjalizacja I2S w trybie Slave (odbiornik)
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
            .din = I2S_DI_IO,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}

// Inicjalizacja UART do wymiany tytułów i komend sterowania
static void init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, 1024, 1024, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_IO, UART_RX_IO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

// Zadanie przesyłające dźwięk z I2S do portu USB Audio (Volvo)
static void audio_usb_task(void *arg) {
    uint8_t i2s_buf[180];
    size_t bytes_read = 0;

    while (1) {
        if (i2s_channel_read(rx_handle, i2s_buf, 176, &bytes_read, portMAX_DELAY) == ESP_OK) {
            if (bytes_read > 0 && tud_mounted()) {
                tud_audio_write(i2s_buf, bytes_read);
            }
        }
    }
}

// Zadanie odbierające tytuły piosenek z ESP32 Classic po UART
static void uart_rx_task(void *arg) {
    uint8_t data[256];
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, sizeof(data) - 1, pdMS_TO_TICKS(50));
        if (len > 0) {
            data[len] = '\0';
            char *line = strtok((char *)data, "\r\n");
            while (line != NULL) {
                if (strncmp(line, "TITLE:", 6) == 0) {
                    iap_update_track_title(line + 6);
                } else if (strncmp(line, "ARTIST:", 7) == 0) {
                    iap_update_track_artist(line + 7);
                }
                line = strtok(NULL, "\r\n");
            }
        }
    }
}

// Callbacki TinyUSB HID dla protokołu Apple iAP
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_type;
    iap_process_hid_packet(report_id, buffer, bufsize, UART_PORT_NUM);
}

void app_main(void) {
    ESP_LOGI(TAG, "Uruchamianie emulatora iPoda dla Volvo XC70...");

    init_uart();
    init_i2s_slave();

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &desc_device,
        .string_descriptor = string_desc_arr,
        .string_descriptor_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
        .external_phy = false,
        .configuration_descriptor = desc_configuration,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    xTaskCreatePinnedToCore(audio_usb_task, "audio_usb_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(uart_rx_task, "uart_rx_task", 4096, NULL, 4, NULL, 0);

    ESP_LOGI(TAG, "Gotowy do podlaczenia pod USB w Volvo!");
}
