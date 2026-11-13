idf_component_register(
    SRCS "main.c" "usb_descriptors.c" "iap_protocol.c"
    INCLUDE_DIRS "."
)

# The project uses esp_tinyusb for USB initialization and TinyUSB for
# Audio/HID. Audio definitions must match in BOTH translation units:
# our app and the TinyUSB class driver compiled as a separate component.
set(VOLVO_AUDIO_CONFIG
    CFG_TUD_AUDIO=1
    CFG_TUD_AUDIO_FUNC_1_DESC_LEN=99
    CFG_TUD_AUDIO_FUNC_1_N_AS_INT=1
    CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ=64
    CFG_TUD_AUDIO_ENABLE_EP_IN=1
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX=180
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ=720
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE ${VOLVO_AUDIO_CONFIG})

idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tusb_lib} PRIVATE ${VOLVO_AUDIO_CONFIG})
