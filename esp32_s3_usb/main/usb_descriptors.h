#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include <stdint.h>

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

#endif
