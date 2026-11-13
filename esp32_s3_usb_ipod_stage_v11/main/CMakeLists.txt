idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES espressif__esp_tinyusb espressif__tinyusb)
# Same definitions for application and USB stack, retaining the working v1.0.1 HID Kconfig.
set(STAGE11_USB_DEFS
    CFG_TUD_AUDIO=1
    CFG_TUD_AUDIO_ENABLE_EP_IN=1
    CFG_TUD_AUDIO_ENABLE_EP_OUT=0
    CFG_TUD_AUDIO_EP_IN_FLOW_CONTROL=0
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX=180
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ=1440
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX=2
    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX=2
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE ${STAGE11_USB_DEFS})
idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tinyusb_lib} PRIVATE ${STAGE11_USB_DEFS})
