#include <string.h>
#include "tusb.h"
#include "usb_descriptors.h"

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x05AC, // Apple Inc.
    .idProduct          = 0x12AB, // iPad (alternatywnie 0x1291 dla iPod Touch)
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

uint8_t const desc_hid_report[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    
    // Report ID 1: 12 bajtow
    0x85, 0x01, 0x09, 0x01, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x0C, 0x81, 0x02, 0x09, 0x01, 0x91, 0x02,
    // Report ID 2: 63 bajty
    0x85, 0x02, 0x09, 0x01, 0x95, 0x3F, 0x81, 0x02, 0x09, 0x01, 0x91, 0x02,
    // Report ID 3: 127 bajtow
    0x85, 0x03, 0x09, 0x01, 0x95, 0x7F, 0x81, 0x02, 0x09, 0x01, 0x91, 0x02,
    // Report ID 4: 255 bajtow
    0x85, 0x04, 0x09, 0x01, 0x95, 0xFF, 0x81, 0x02, 0x09, 0x01, 0x91, 0x02,

    0xC0
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

#define EP_AUDIO_SIZE 180

uint8_t const desc_configuration[] = {
    // Configuration Descriptor: total length verified by a compile-time assertion.
    0x09, TUSB_DESC_CONFIGURATION, 0x8C, 0x00, ITF_NUM_TOTAL, 0x01, 0x00, 0x80, 250,

    // USB Audio function IAD: interfaces 0 and 1 belong to one function.
    // (8-byte IAD + 91-byte Audio descriptors = 99 bytes)
    0x08, 0x0B, ITF_NUM_AUDIO_CONTROL, 0x02, 0x01, 0x00, 0x00, 0x00,

    // Interface 0: Audio Control
    0x09, TUSB_DESC_INTERFACE, ITF_NUM_AUDIO_CONTROL, 0x00, 0x00, TUSB_CLASS_AUDIO, 0x01, 0x00, 0x00,
    0x09, 0x24, 0x01, 0x00, 0x01, 0x1E, 0x00, 0x01, ITF_NUM_AUDIO_STREAMING,
    0x0C, 0x24, 0x02, 0x01, 0x01, 0x02, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x09, 0x24, 0x03, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00,

    // Interface 1: Audio Streaming (Alt 0 - Idle)
    0x09, TUSB_DESC_INTERFACE, ITF_NUM_AUDIO_STREAMING, 0x00, 0x00, TUSB_CLASS_AUDIO, 0x02, 0x00, 0x00,

    // Interface 1: Audio Streaming (Alt 1 - Active 44.1kHz Stereo)
    0x09, TUSB_DESC_INTERFACE, ITF_NUM_AUDIO_STREAMING, 0x01, 0x01, TUSB_CLASS_AUDIO, 0x02, 0x00, 0x00,
    0x07, 0x24, 0x01, 0x02, 0x01, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, 0x02, 0x02, 0x10, 0x01, 0x44, 0xAC, 0x00,
    0x09, TUSB_DESC_ENDPOINT, EPNUM_AUDIO_IN, (TUSB_XFER_ISOCHRONOUS | 0x04), U16_TO_U8S_LE(EP_AUDIO_SIZE), 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x00, 0x00, 0x00, 0x00,

    // Interface 2: HID (iAP Channel)
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_OUT, EPNUM_HID_IN, 64, 1)
};

_Static_assert(sizeof(desc_configuration) == 140, "USB descriptor length mismatch");

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "Apple Inc.",
    "iPad",
    "000000000000000000000001",
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}
