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

/* STAGE v1.1: known-working UAC1 v1.0.1 base; stereo 44.1 kHz;
 * vendor-defined HID packet transport for future iAP integration.
 * Diagnostic VID/PID -- deliberately NOT masquerading as Apple yet.
 * USB reports are logged but do not receive fabricated iAP responses.
 * Audio sends silence. This stage does not yet play Bluetooth audio.
 */
#define TEST_VID 0xCAFE
#define TEST_PID 0x4017
#define SAMPLE_RATE 44100u
#define ITF_AUDIO_CONTROL 0
#define ITF_AUDIO_STREAM  1
#define ITF_IAP_HID       2
#define ITF_COUNT         3
#define AUDIO_EP          0x81
#define HID_EP            0x82
#define AUDIO_EP_SIZE     180
#define IAP_REPORT_ID     1
#define IAP_DATA_SIZE     63

static const char *TAG = "IPOD_USB_STAGE11";

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
  .bcdDevice = 0x0110,
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 3,
  .bNumConfigurations = 1
};

/* Vendor-defined HID; Report ID 1, 63-byte input and output payloads.
 * Output reports travel via EP0 SET_REPORT (like the reference's single
 * interrupt IN endpoint). Not an iAP protocol implementation yet.
 */
static const uint8_t hid_report[] = {
  0x06, 0x00, 0xFF,    /* Usage Page: vendor */
  0x09, 0x01,          /* Usage */
  0xA1, 0x01,          /* Application Collection */
  0x85, IAP_REPORT_ID, /* Report ID */
  0x09, 0x01, 0x15, 0x00, 0x26, 0xFF, 0x00,
  0x75, 0x08, 0x95, IAP_DATA_SIZE,
  0x81, 0x02,          /* Input */
  0x09, 0x01, 0x95, IAP_DATA_SIZE,
  0x91, 0x02,          /* Output */
  0xC0
};

/* UAC1 AudioControl header class-specific size: 9 + 12 + 9 = 30.
 * Stereo 16-bit 44.1 kHz: normally 176 bytes / 1ms, 180 every 10th.
 * Total configuration 9 + 99 + 32 = 140 bytes.
 */
#define AUDIO_FUNCTION_LEN (8+9+9+12+9+9+9+7+11+9+7)
#define CONFIG_LEN (9+AUDIO_FUNCTION_LEN+TUD_HID_DESC_LEN)
static const uint8_t configuration_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_LEN, 0, 100),
    /* Interface Association Descriptor; audio occupies interfaces 0 and 1 */
    8, 0x0B, ITF_AUDIO_CONTROL, 2, 0x01, 0x00, 0x00, 4,
    9, 0x04, ITF_AUDIO_CONTROL, 0, 0, 0x01, 0x01, 0x00, 4,
    9, 0x24, 0x01, 0x00, 0x01, 0x1E, 0x00, 1, ITF_AUDIO_STREAM,
    /* Stereo microphone input terminal, FL/FR channel bitmap=0x0003 */
    12, 0x24, 0x02, 1, 0x01, 0x02, 0, 2, 0x03, 0, 0, 0,
    /* USB streaming output terminal */
    9, 0x24, 0x03, 2, 0x01, 0x01, 0, 1, 0,
    9, 0x04, ITF_AUDIO_STREAM, 0, 0, 0x01, 0x02, 0x00, 0,
    9, 0x04, ITF_AUDIO_STREAM, 1, 1, 0x01, 0x02, 0x00, 0,
    7, 0x24, 0x01, 2, 1, 0x01, 0x00,
    /* Type I, 2 channels, 2 bytes/sample, 16-bit, 44.1 kHz = 0x00AC44 */
    11, 0x24, 0x02, 1, 2, 2, 16, 1, 0x44, 0xAC, 0,
    9, 0x05, AUDIO_EP, 0x05, AUDIO_EP_SIZE, 0, 1, 0, 0,
    7, 0x25, 0x01, 0, 0, 0, 0,
    /* HID vendor interface: nonboot (no keyboard emulation) */
    TUD_HID_DESCRIPTOR(ITF_IAP_HID, 5, HID_ITF_PROTOCOL_NONE,
                       sizeof(hid_report), HID_EP, 64, 10)
};
_Static_assert(sizeof(configuration_desc) == CONFIG_LEN, "Bad configuration length");
_Static_assert(AUDIO_FUNCTION_LEN == 99, "Bad audio function length");
_Static_assert(AUDIO_EP_SIZE == 180, "Bad stereo packet capacity");

static const char * const strings[] = {
    (const char[]){0x09, 0x04},
    "ESP32 Diagnostics",
    "S3 iPod USB stage v1.1",
    "S3IPODSTAGE11",
    "UAC1 Stereo 44k1",
    "Vendor HID iAP Transport"
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t *buffer, uint16_t reqlen) {
  (void)instance;
  if (report_type != HID_REPORT_TYPE_INPUT || report_id != IAP_REPORT_ID || reqlen == 0) return 0;
  /* No fake iAP response. Host can query an empty diagnostic input report. */
  uint16_t count = reqlen < IAP_DATA_SIZE ? reqlen : IAP_DATA_SIZE;
  memset(buffer, 0, count);
  return count;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t size) {
  (void)instance;
  if (report_type != HID_REPORT_TYPE_OUTPUT || !buffer || size == 0) return;
  ESP_LOGI(TAG, "HID output id=%u len=%u", (unsigned)report_id, (unsigned)size);
  /* Log only a small prefix; data may be padded and is untrusted. */
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, size < 32 ? size : 32, ESP_LOG_INFO);
}

/* Diagnostic silence, NOT a functioning audio player.
 * For 44.1 kHz we need 44 frames/ms, plus an extra frame every 10th ms.
 * The task scheduler is not an audio-grade clock; validated here only for
 * USB enumeration. Streaming will require a USB SOF-paced producer later.
 */
static void silence_task(void *arg) {
  (void)arg;
  static const int16_t silence[90] = {0};
  unsigned frame_tick = 0;
  while (true) {
    if (tud_mounted() && !tud_suspended() && tud_audio_mounted()) {
      unsigned frames = (++frame_tick % 10u == 0u) ? 45u : 44u;
      (void)tud_audio_write(silence, frames * 2u * sizeof(int16_t));
    } else {
      frame_tick = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(1) ? pdMS_TO_TICKS(1) : 1);
  }
}

void app_main(void) {
  tinyusb_config_t cfg = TINYUSB_DEFAULT_CONFIG();
  cfg.descriptor.device = &device_desc;
  cfg.descriptor.full_speed_config = configuration_desc;
  cfg.descriptor.string = (char const **)strings;
  cfg.descriptor.string_count = sizeof(strings) / sizeof(strings[0]);
  ESP_LOGI(TAG, "UAC1 stereo + vendor HID test: %04X:%04X", TEST_VID, TEST_PID);
  ESP_ERROR_CHECK(tinyusb_driver_install(&cfg));
  if (xTaskCreate(silence_task, "silence", 4096, NULL, 3, NULL) != pdPASS)
    ESP_LOGE(TAG, "Failed to create silence task");
  while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}
