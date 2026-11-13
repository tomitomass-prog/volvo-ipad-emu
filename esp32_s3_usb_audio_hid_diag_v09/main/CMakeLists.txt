idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES espressif__esp_tinyusb espressif__tinyusb)

# TinyUSB 0.15 audio must be enabled in BOTH the app and TinyUSB component.
set(USB_DIAG_DEFINITIONS
    CFG_TUD_AUDIO=1
    CFG_TUD_HID=1
    CFG_TUD_AUDIO_FUNC_1_DESC_LEN=TUD_AUDIO_MIC_ONE_CH_DESC_LEN
    CFG_TUD_AUDIO_FUNC_1_N_AS_INT=1
    CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ=64
    CFG_TUD_AUDIO_ENABLE_EP_IN=1
    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX=2
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX=1
    CFG_TUD_AUDIO_EP_SZ_IN=96
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX=96
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ=384
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE ${USB_DIAG_DEFINITIONS})
idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tinyusb_lib} PRIVATE ${USB_DIAG_DEFINITIONS})
