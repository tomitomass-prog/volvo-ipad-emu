cmake_minimum_required(VERSION 3.16)

# Przekazanie definicji konfiguracji TinyUSB Audio bezpośrednio do wszystkich komponentów
add_compile_definitions(
    CFG_TUD_AUDIO=1
    CFG_TUD_AUDIO_FUNC_1_DESC_LEN=100
    CFG_TUD_AUDIO_FUNC_1_N_AS_INT=1
    CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ=64
    CFG_TUD_AUDIO_ENABLE_EP_IN=1
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX=180
    CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ=720
    CFG_TUD_HID_EP_BUFSIZE=256
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(volvo_ipod_s3)
