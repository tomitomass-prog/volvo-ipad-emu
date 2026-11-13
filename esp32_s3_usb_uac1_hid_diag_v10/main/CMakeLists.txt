idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES espressif__esp_tinyusb espressif__tinyusb)
# Both the application and TinyUSB itself MUST see identical class settings.
set(UAC1_DIAG_DEFINITIONS
    CFG_TUD_AUDIO=1
    CFG_TUD_HID=1
    CFG_TUD_AUDIO_ENABLE_EP_IN=1
    CFG_TUD_AUDIO_ENABLE_EP_OUT=0
    CFG_TUD_AUDIO_EP_IN_FLOW_CONTROL=0
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX=96
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ=384
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX=1
    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX=2
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE ${UAC1_DIAG_DEFINITIONS})
idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tinyusb_lib} PRIVATE ${UAC1_DIAG_DEFINITIONS})
