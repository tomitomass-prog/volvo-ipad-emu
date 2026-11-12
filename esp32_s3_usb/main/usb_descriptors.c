#include <string.h>
#include "tusb.h"
#include "usb_descriptors.h"

// Stałe zgodne ze specyfikacją USB Audio Class 1.0 (UAC1)
#define UAC_SUBCLASS_CONTROL              0x01
#define UAC_SUBCLASS_STREAMING            0x02
#define UAC_PROTOCOL_UNDEFINED            0x00

#define UAC_CS_INTERFACE                  0x24
#define UAC_CS_ENDPOINT                   0x25

#define UAC_AC_HEADER                     0x01
#define UAC_AC_INPUT_TERMINAL             0x02
#define UAC_AC_OUTPUT_TERMINAL            0x03

#define UAC_AS_GENERAL                    0x01
#define UAC_AS_FORMAT_TYPE                0x02
#define UAC_EP_GENERAL                    0x01

// 1. Device Descriptor (Podszywamy się pod iPada / iPoda)
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x05AC, // Apple, Inc.
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

// 2. HID Report Descriptor (Standard Apple iAP wielorozmiarowe raporty)
uint8_t const desc_hid_report[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    
    // Report ID 1: Pakiety do 12 bajtów
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (0x01)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x0C,        //   Report Count (12 bytes)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x02,        //   Output (Data, Var, Abs)

    // Report ID 2: Pakiety do 63 bajtów
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x01,        //   Usage (0x01)
    0x95, 0x3F,        //   Report Count (63 bytes)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x02,        //   Output (Data, Var, Abs)

    // Report ID 3: Pakiety do 127 bajtów
    0x85, 0x03,        //   Report ID (3)
    0x09, 0x01,        //   Usage (0x01)
    0x95, 0x7F,        //   Report Count (127 bytes)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x02,        //   Output (Data, Var, Abs)

    // Report ID 4: Pakiety do 255 bajtów
    0x85, 0x04,        //   Report ID (4)
    0x09, 0x01,        //   Usage (0x01)
    0x95, 0xFF,        //   Report Count (255 bytes)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x09, 0x01,        //   Usage (0x01)
    0x91, 0x02,        //   Output (Data, Var, Abs)

    0xC0               // End Collection
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

// 3. Configuration Descriptor (Audio UAC1 Stereo IN + HID IN/OUT)
#define EP_AUDIO_SIZE     180 
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + 9 + 9 + 12 + 9 + 9 + 9 + 7 + 11 + 9 + 7 + TUD_HID_INOUT_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Nagłówek konfiguracji
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // --- INTERFEJS 0: Audio Control ---
    0x09, TUSB_DESC_INTERFACE, ITF_NUM_AUDIO_CONTROL, 0x00, 0x00, TUSB_CLASS_AUDIO, UAC_SUBCLASS_CONTROL, UAC_PROTOCOL_UNDEFINED, 0x00,
    // Audio Control Interface Header Descriptor
    0x09, UAC_CS_INTERFACE, UAC_AC_HEADER, 0x00, 0x01, 0x1E, 0x00, 0x01, ITF_NUM_AUDIO_STREAMING,
    // Input Terminal Descriptor (Strumień z iPoda: Stereo L+R)
    0x0C, UAC_CS_INTERFACE, UAC_AC_INPUT_TERMINAL, 0x01, 0x01, 0x02, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    // Output Terminal Descriptor (Wyjście USB Streaming do radia Volvo)
    0x09, UAC_CS_INTERFACE, UAC_AC_OUTPUT_TERMINAL, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00,

    // --- INTERFEJS 1: Audio Streaming (AltSetting 0 - Idle) ---
    0x09, TUSB_DESC_INTERFACE, ITF_NUM_AUDIO_STREAMING, 0x00, 0x00, TUSB_CLASS_AUDIO, UAC_SUBCLASS_STREAMING, UAC_PROTOCOL_UNDEFINED, 0x00,

    // --- INTERFEJS 1: Audio Streaming (AltSetting 1 - Aktywne Stereo 44.1kHz) ---
    0x09, TUSB_DESC_INTERFACE, ITF_NUM_AUDIO_STREAMING, 0x01, 0x01, TUSB_CLASS_AUDIO, UAC_SUBCLASS_STREAMING, UAC_PROTOCOL_UNDEFINED, 0x00,
    // Audio Streaming Class Specific Interface Descriptor
    0x07, UAC_CS_INTERFACE, UAC_AS_GENERAL, 0x02, 0x01, 0x01, 0x00,
    // Audio Type I Format Type Descriptor (2 kanały, 16-bit, 44100 Hz)
    0x0B, UAC_CS_INTERFACE, UAC_AS_FORMAT_TYPE, 0x01, 0x02, 0x02, 0x10, 0x01, 0x44, 0xAC, 0x00,
    // Isochronous Endpoint Descriptor (EP 0x81 IN)
    0x09, TUSB_DESC_ENDPOINT, EPNUM_AUDIO_IN, (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS), U16_TO_U8S_LE(EP_AUDIO_SIZE), 0x01, 0x00, 0x00,
    // Class Specific Isochronous Audio Data Endpoint Descriptor
    0x07, UAC_CS_ENDPOINT, UAC_EP_GENERAL, 0x00, 0x00, 0x00, 0x00,

    // --- INTERFEJS 2: HID (Kanał komunikacji iAP) ---
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_OUT, EPNUM_HID_IN, 64, 1)
};

// Weryfikacja w czasie kompilacji, czy obliczona długość zgadza się z rzeczywistym rozmiarem tablicy
_Static_assert(sizeof(desc_configuration) == CONFIG_TOTAL_LEN, "Nieprawidlowa dlugosc desc_configuration");

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

// 4. String Descriptors
char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: Język Angielski (0x0409)
    "Apple Inc.",                  // 1: Producent (iManufacturer)
    "iPad",                        // 2: Nazwa produktu (iProduct)
    "000000000000000000000001",    // 3: Numer seryjny (iSerialNumber)
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
        for(uint8_t i=0; i<chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);
    return _desc_str;
}

