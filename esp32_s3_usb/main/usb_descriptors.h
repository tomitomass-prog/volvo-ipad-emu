#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include <stdint.h>
#include "tusb.h"

enum {
    ITF_NUM_AUDIO_CONTROL = 0,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define EPNUM_AUDIO_IN    0x81
#define EPNUM_HID_IN      0x82
#define EPNUM_HID_OUT     0x02

#define AUDIO_SAMPLE_RATE 44100

extern tusb_desc_device_t const desc_device;
extern uint8_t const desc_configuration[];
extern char const *string_desc_arr[];

#endif
