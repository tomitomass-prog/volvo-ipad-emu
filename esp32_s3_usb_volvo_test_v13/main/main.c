#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "iap_diag.h"
#include "tusb.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

/* STAGE v1.3: experimental car USB enumeration + virtual single track.
 * Uses vendor HID report sizes from oandrew/ipod-gadget full-speed reference.
 * Apple iPad VID/PID used for this car-side compatibility experiment.
 * Responds to a few verified General Lingo identity queries only.
 * Audio sends a repeating synthetic tone; no MP3 nor Bluetooth is present.
 */
#define TEST_VID 0x05AC
#define TEST_PID 0x12AB
#define SAMPLE_RATE 44100u
#define ITF_AUDIO_CONTROL 0
#define ITF_AUDIO_STREAM  1
#define ITF_IAP_HID       2
#define ITF_COUNT         3
#define AUDIO_EP          0x81
#define HID_EP            0x82
#define AUDIO_EP_SIZE     180
#define IAP_MAX_TX_PAYLOAD 63
#define IAP_QUEUE_DEPTH 8
static QueueHandle_t reply_queue;

static const char *TAG = "VOLVO_IPOD_V13";

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

/* Full-Speed HID report layout from oandrew/ipod-gadget/gadget/ipod.h.
 * Report IDs 1-4: INPUT 12/14/20/63 bytes. 5-9: OUTPUT 8/10/14/20/63.
 * One INTERRUPT IN endpoint; host sends OUTPUT over control SET_REPORT.
 */
static const uint8_t hid_report[] = {
  0x06, 0x00, 0xff, 0x09, 0x01, 0xa1, 0x01, 0x75, 0x08,
  0x26, 0x80, 0x00, 0x15, 0x00,
  0x09, 0x01, 0x85, 0x01, 0x95, 0x0c, 0x82, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x02, 0x95, 0x0e, 0x82, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x03, 0x95, 0x14, 0x82, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x04, 0x95, 0x3f, 0x82, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x05, 0x95, 0x08, 0x92, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x06, 0x95, 0x0a, 0x92, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x07, 0x95, 0x0e, 0x92, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x08, 0x95, 0x14, 0x92, 0x02, 0x01,
  0x09, 0x01, 0x85, 0x09, 0x95, 0x3f, 0x92, 0x02, 0x01,
  0xc0
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
    "Apple Inc.",
    "iPad",
    "000000000000000000000001",
    "iPod USB Audio",
    "iPod Accessory Interface"
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t *buffer, uint16_t reqlen) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  /* Do not fabricate a report in response to a GET_REPORT request. */
  return 0;
}

/* USB callback runs on the TinyUSB task; queued messages are sent by our
 * application task so a control SET_REPORT is not answered re-entrantly. */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t size) {
  (void)instance;
  if (report_type != HID_REPORT_TYPE_OUTPUT || !buffer || !size) return;
  ESP_LOGI(TAG, "iAP HID OUT id=%u len=%u", (unsigned)report_id, (unsigned)size);
  ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, size < 32 ? size : 32, ESP_LOG_INFO);
  iap_frame_t incoming;
  if (!iap_decode(buffer, size, &incoming)) {
    ESP_LOGW(TAG, "Not a complete, valid iAP frame; no reply");
    return;
  }
  ESP_LOGI(TAG, "iAP lingo=%02x cmd=%04x bytes=%u", incoming.lingo,
           incoming.command, (unsigned)incoming.payload_len);
  iap_tx_t tx = {0};
  if (iap_volvo_reply(&incoming, &tx) && reply_queue) {
    if (xQueueSend(reply_queue, &tx, 0) != pdPASS)
      ESP_LOGW(TAG, "TX queue full, reply dropped");
  } else {
    ESP_LOGI(TAG, "Unsupported iAP command; passive diagnostic logging");
  }
}

static void iap_tx_task(void *arg) {
  (void)arg;
  iap_tx_t tx;
  while (true) {
    if (xQueueReceive(reply_queue, &tx, pdMS_TO_TICKS(100)) != pdPASS) continue;
    /* Input reports supported: 12/14/20/63 data bytes. */
    uint8_t rid = 4, cap = 63;
    if (tx.len <= 12) { rid = 1; cap = 12; }
    else if (tx.len <= 14) { rid = 2; cap = 14; }
    else if (tx.len <= 20) { rid = 3; cap = 20; }
    uint8_t padded[63] = {0};
    memcpy(padded, tx.bytes, tx.len);
    /* Avoid blocking the TinyUSB task; ensure endpoint ready to transmit. */
    for (unsigned attempts = 0; attempts < 100; ++attempts) {
      if (tud_hid_ready() && tud_hid_report(rid, padded, cap)) {
        ESP_LOGI(TAG, "iAP reply id=%u len=%u", rid, (unsigned)tx.len);
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(10) ? pdMS_TO_TICKS(10) : 1);
    }
  }
}

/* Synthetic audio demo, not a stored MP3.  Simple low-volume melody
 * C4-E4-G4-C5-G4-E4, each note 300ms with a 100ms break.
 * Delivery is RTOS-timed (not SOF synchronised); this is only a car test.
 * An audio-ready interface alone does not prove that the host will play it.
 */
static void tone_task(void *arg) {
  (void)arg;
  static int16_t samples[90];
  uint32_t sample_index = 0;
  unsigned frame_tick = 0;
  while (true) {
    if (tud_mounted() && !tud_suspended() && tud_audio_mounted()) {
      unsigned frames = (++frame_tick % 10u == 0u) ? 45u : 44u;
      for (unsigned n = 0; n < frames; ++n, ++sample_index) {
        static const uint16_t melody_hz[] = {262, 330, 392, 523, 392, 330};
        uint32_t note_samples = SAMPLE_RATE * 4u / 10u;
        uint32_t note_index = (sample_index / note_samples) % 6u;
        uint32_t within = sample_index % note_samples;
        uint32_t hz = melody_hz[note_index];
        /* Keep intermediate multiplication within uint32_t bounds. */
        uint32_t phase = ((sample_index % SAMPLE_RATE) * hz) % SAMPLE_RATE;
        int32_t tri = (phase < SAMPLE_RATE/2u)
           ? -16384 + (int32_t)(phase * 65536u / SAMPLE_RATE)
           :  16384 - (int32_t)((phase - SAMPLE_RATE/2u) * 65536u / SAMPLE_RATE);
        uint32_t on_samples = SAMPLE_RATE * 3u / 10u;
        uint32_t fade = SAMPLE_RATE / 50u;
        uint32_t gain = within < on_samples ? 1000u : 0u;
        if (within < fade) gain = within * 1000u / fade;
        if (within >= on_samples - fade && within < on_samples)
          gain = (on_samples - within) * 1000u / fade;
        /* Low-volume test signal, stereo duplicated */
        int16_t v = (int16_t)(tri * (int32_t)gain / 10000);
        samples[2*n] = v;
        samples[2*n+1] = v;
      }
      (void)tud_audio_write(samples, frames * 2u * sizeof(int16_t));
    } else {
      frame_tick = 0;
      sample_index = 0;
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
  reply_queue = xQueueCreate(IAP_QUEUE_DEPTH, sizeof(iap_tx_t));
  ESP_ERROR_CHECK(reply_queue ? ESP_OK : ESP_ERR_NO_MEM);
  ESP_LOGI(TAG, "Volvo iPod car test v1.3 (track berbelucha): %04X:%04X", TEST_VID, TEST_PID);
  ESP_ERROR_CHECK(tinyusb_driver_install(&cfg));
  if (xTaskCreate(iap_tx_task, "iap_tx", 4096, NULL, 4, NULL) != pdPASS)
    ESP_LOGE(TAG, "Failed to create iAP TX task");
  if (xTaskCreate(tone_task, "test_tone", 4096, NULL, 3, NULL) != pdPASS)
    ESP_LOGE(TAG, "Failed to create test tone task");
  while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}
