#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb.h"
#include "tinyusb.h"

/* USB-only diagnostics. No actual audio source, I2S, BT or iAP. */
#define DIAG_VID 0xCAFE
#define DIAG_PID 0x4014
#define AUDIO_EP_IN 0x81
#define HID_EP_IN   0x82
#define AUDIO_SAMPLE_RATE 48000u

#define ITF_AUDIO_CONTROL  0
#define ITF_AUDIO_STREAM   1
#define ITF_HID            2
#define ITF_TOTAL          3
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_AUDIO_MIC_ONE_CH_DESC_LEN + TUD_HID_DESC_LEN)

static const char *TAG = "S3_UAC2_HID_DIAG";

static const tusb_desc_device_t dev_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = 64,
    .idVendor = DIAG_VID,
    .idProduct = DIAG_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const uint8_t hid_report_desc[] = { TUD_HID_REPORT_DESC_KEYBOARD() };

static const uint8_t config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
    /* Official TinyUSB 0.15 UAC2 mono microphone descriptor template. */
    TUD_AUDIO_MIC_ONE_CH_DESCRIPTOR(ITF_AUDIO_CONTROL, 4,
                                  CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX,
                                  CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * 8,
                                  AUDIO_EP_IN, CFG_TUD_AUDIO_EP_SZ_IN),
    TUD_HID_DESCRIPTOR(ITF_HID, 5, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_desc), HID_EP_IN, 16, 10),
};
_Static_assert(sizeof(config_desc) == CONFIG_TOTAL_LEN, "Invalid USB configuration total length");

static const char *const str_desc[] = {
    (const char[]){0x09, 0x04},
    "ESP32 Diagnostics",
    "S3 Audio HID Test v0.8",
    "S3AUDIOHID08",
    "USB Audio Microphone",
    "Diagnostic HID Keyboard"
};

/* esp_tinyusb owns device, configuration and string descriptor callbacks. */
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_desc;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id;
    if (report_type != HID_REPORT_TYPE_INPUT || reqlen < 8) return 0;
    memset(buffer, 0, 8);
    return 8;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

/* Minimal UAC2 requests for the clock source (ID 4) and feature unit (ID 2). */
static uint32_t sample_rate = AUDIO_SAMPLE_RATE;
static uint8_t clock_valid = 1;
static uint8_t mute[2] = {0, 0};
static int16_t volume[2] = {0, 0};

bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *req) {
    (void)rhport; (void)req;
    return false;
}
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const *req) {
    (void)rhport; (void)req;
    return false;
}
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *req, uint8_t *buf) {
    (void)rhport; (void)req; (void)buf;
    return false;
}
bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const *req, uint8_t *buf) {
    (void)rhport; (void)req; (void)buf;
    return false;
}
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *req) {
    const uint8_t entity = TU_U16_HIGH(req->wIndex);
    const uint8_t selector = TU_U16_HIGH(req->wValue);
    const uint8_t channel = TU_U16_LOW(req->wValue);
    if (entity == 4 && selector == AUDIO_CS_CTRL_SAM_FREQ) {
        if (req->bRequest == AUDIO_CS_REQ_CUR) {
            return tud_control_xfer(rhport, req, &sample_rate, sizeof(sample_rate));
        }
        if (req->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_4_n_t(1) range;
            memset(&range, 0, sizeof(range));
            range.wNumSubRanges = 1;
            range.subrange[0].bMin = AUDIO_SAMPLE_RATE;
            range.subrange[0].bMax = AUDIO_SAMPLE_RATE;
            return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &range, sizeof(range));
        }
    }
    if (entity == 4 && selector == AUDIO_CS_CTRL_CLK_VALID && req->bRequest == AUDIO_CS_REQ_CUR) {
        return tud_control_xfer(rhport, req, &clock_valid, sizeof(clock_valid));
    }
    if (entity == 1 && selector == AUDIO_TE_CTRL_CONNECTOR && req->bRequest == AUDIO_CS_REQ_CUR) {
        audio_desc_channel_cluster_t cluster = {0};
        cluster.bNrChannels = 1;
        return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &cluster, sizeof(cluster));
    }
    if (entity == 2 && channel < 2) {
        if (selector == AUDIO_FU_CTRL_MUTE && req->bRequest == AUDIO_CS_REQ_CUR)
            return tud_control_xfer(rhport, req, &mute[channel], 1);
        if (selector == AUDIO_FU_CTRL_VOLUME && req->bRequest == AUDIO_CS_REQ_CUR)
            return tud_control_xfer(rhport, req, &volume[channel], sizeof(volume[channel]));
        if (selector == AUDIO_FU_CTRL_VOLUME && req->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_2_n_t(1) range;
            memset(&range, 0, sizeof(range));
            range.wNumSubRanges = 1;
            range.subrange[0].bMin = -90 * 256;
            range.subrange[0].bMax = 0;
            range.subrange[0].bRes = 256;
            return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &range, sizeof(range));
        }
    }
    return false;
}
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *req, uint8_t *buf) {
    (void)rhport;
    const uint8_t entity = TU_U16_HIGH(req->wIndex);
    const uint8_t selector = TU_U16_HIGH(req->wValue);
    const uint8_t channel = TU_U16_LOW(req->wValue);
    if (entity != 2 || channel >= 2 || req->bRequest != AUDIO_CS_REQ_CUR) return false;
    if (selector == AUDIO_FU_CTRL_MUTE && req->wLength == 1) {
        mute[channel] = buf[0];
        return true;
    }
    if (selector == AUDIO_FU_CTRL_VOLUME && req->wLength == 2) {
        memcpy(&volume[channel], buf, 2);
        return true;
    }
    return false;
}

void app_main(void) {
    const tinyusb_config_t cfg = {
        .device_descriptor = &dev_desc,
        .configuration_descriptor = config_desc,
        .string_descriptor = (char const **)str_desc,
        .string_descriptor_count = sizeof(str_desc) / sizeof(str_desc[0]),
        .external_phy = false,
    };
    ESP_LOGI(TAG, "Starting standalone UAC2 microphone + HID diagnostic VID=%04X PID=%04X", DIAG_VID, DIAG_PID);
    ESP_ERROR_CHECK(tinyusb_driver_install(&cfg));
    bool was_mounted = false;
    while (true) {
        bool now_mounted = tud_mounted();
        if (now_mounted != was_mounted) {
            ESP_LOGI(TAG, "USB mounted: %s", now_mounted ? "YES" : "NO");
            was_mounted = now_mounted;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
