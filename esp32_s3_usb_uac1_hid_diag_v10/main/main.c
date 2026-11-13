#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

/* USB Audio 1.0 mono microphone, 48 kHz / 16 bit + inert HID keyboard.
   No iAP, I2S, Bluetooth or Apple VID/PID. */
#define TEST_VID 0xCAFE
#define TEST_PID 0x4016
#define SAMPLE_RATE 48000
#define ITF_AUDIO_CONTROL 0
#define ITF_AUDIO_STREAM  1
#define ITF_HID           2
#define ITF_COUNT         3
#define AUDIO_EP          0x81
#define HID_EP            0x82

static const char *TAG = "S3_UAC1_HID_V10";

static const tusb_desc_device_t device_desc = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = TUSB_CLASS_MISC,
  .bDeviceSubClass = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0 = 64,
  .idVendor = TEST_VID,
  .idProduct = TEST_PID,
  .bcdDevice = 0x0100,
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 3,
  .bNumConfigurations = 1
};
static const uint8_t hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

/* Explicit USB Audio Class 1.0 descriptors. The audio function occupies
   consecutive interfaces 0 and 1; interface 2 is HID.
   AC Header total class-specific length = 9 + 12 + 9 = 30 (0x1E).
   48kHz mono S16LE = 96 bytes per 1ms frame. */
#define AUDIO_FUNCTION_LEN (8+9+9+12+9+9+9+7+11+9+7)
#define CONFIG_LEN (9+AUDIO_FUNCTION_LEN+TUD_HID_DESC_LEN)
static const uint8_t configuration_desc[] = {
    /* Configuration 1, three interfaces, bus-powered 100 mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_LEN, 0, 100),
    /* Interface association descriptor: 2 audio interfaces, UAC 1.0 */
    8, 0x0B, ITF_AUDIO_CONTROL, 2, 0x01, 0x00, 0x00, 4,
    /* AudioControl interface, alternate 0, zero endpoints */
    9, 0x04, ITF_AUDIO_CONTROL, 0, 0, 0x01, 0x01, 0x00, 4,
    /* UAC1 class-specific AudioControl header, total=30 bytes */
    9, 0x24, 0x01, 0x00, 0x01, 0x1E, 0x00, 1, ITF_AUDIO_STREAM,
    /* Input Terminal ID 1, type Microphone (0x0201), mono */
    12, 0x24, 0x02, 1, 0x01, 0x02, 0, 1, 0, 0, 0, 0,
    /* Output Terminal ID 2, type USB Streaming (0x0101), source 1 */
    9, 0x24, 0x03, 2, 0x01, 0x01, 0, 1, 0,
    /* AudioStreaming interface alt 0: idle */
    9, 0x04, ITF_AUDIO_STREAM, 0, 0, 0x01, 0x02, 0x00, 0,
    /* AudioStreaming interface alt 1: streaming */
    9, 0x04, ITF_AUDIO_STREAM, 1, 1, 0x01, 0x02, 0x00, 0,
    /* UAC1 AS general, output terminal 2, PCM (format tag 0x0001) */
    7, 0x24, 0x01, 2, 1, 0x01, 0x00,
    /* Type I format: 1 channel, 2 bytes/sample, 16 bits, 48 kHz */
    11, 0x24, 0x02, 1, 1, 2, 16, 1, 0x80, 0xBB, 0,
    /* Isochronous IN, asynchronous, 96 bytes/ms */
    9, 0x05, AUDIO_EP, 0x05, 96, 0, 1, 0, 0,
    /* Class specific AS ISO endpoint, no frequency control advertised */
    7, 0x25, 0x01, 0, 0, 0, 0,
    /* Unmodified working HID keyboard descriptor from v0.7 */
    TUD_HID_DESCRIPTOR(ITF_HID, 5, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report), HID_EP, 16, 10)
};
_Static_assert(sizeof(configuration_desc) == CONFIG_LEN, "Bad configuration length");
_Static_assert(AUDIO_FUNCTION_LEN == 99, "Bad audio function length");

static const char * const strings[] = {
    (const char[]){0x09, 0x04},
    "ESP32 Diagnostics",
    "S3 UAC1 HID Test v1.0",
    "S3UAC1HID10",
    "USB Audio 1.0 Microphone",
    "Diagnostic HID Keyboard"
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return hid_report;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                              hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
  (void)instance; (void)report_id;
  if (report_type != HID_REPORT_TYPE_INPUT || reqlen < 8) return 0;
  memset(buffer, 0, 8);
  return 8;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, const uint8_t *buffer, uint16_t size) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)size;
}

static void silence_task(void *arg) {
  (void)arg;
  static const int16_t silent_pcm[48] = {0};
  while (true) {
    if (tud_mounted() && !tud_suspended() && tud_audio_mounted()) {
      (void)tud_audio_write(silent_pcm, sizeof(silent_pcm));
    }
    vTaskDelay(pdMS_TO_TICKS(1) ? pdMS_TO_TICKS(1) : 1);
  }
}

void app_main(void) {
  tinyusb_config_t usb_cfg = TINYUSB_DEFAULT_CONFIG();
  usb_cfg.descriptor.device = &device_desc;
  usb_cfg.descriptor.full_speed_config = configuration_desc;
  usb_cfg.descriptor.string = (char const **) strings;
  usb_cfg.descriptor.string_count = sizeof(strings) / sizeof(strings[0]);
  ESP_LOGI(TAG, "USB Audio 1.0 + HID: VID=%04X PID=%04X", TEST_VID, TEST_PID);
  ESP_ERROR_CHECK(tinyusb_driver_install(&usb_cfg));
  BaseType_t ok = xTaskCreate(silence_task, "silence", 4096, NULL, 3, NULL);
  if (ok != pdPASS) ESP_LOGE(TAG, "Failed to start silence task");
  while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}
