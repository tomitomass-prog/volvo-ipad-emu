idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES espressif__esp_tinyusb espressif__tinyusb)

# The HID class must be compiled in TinyUSB, not just declared in our application.
target_compile_definitions(${COMPONENT_LIB} PRIVATE CFG_TUD_HID=1)
idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
target_compile_definitions(${tinyusb_lib} PRIVATE CFG_TUD_HID=1)
