#include <Arduino.h>
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

// Callback odbierajacy metadane AVRCP (Tytul, Artysta) z telefonu
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
    if (id == ESP_AVRC_MD_ATTR_TITLE) {
        Serial2.print("TITLE:");
        Serial2.println((const char*)text);
    } else if (id == ESP_AVRC_MD_ATTR_ARTIST) {
        Serial2.print("ARTIST:");
        Serial2.println((const char*)text);
    }
}

void setup() {
    Serial.begin(115200);
    // UART2 polaczony z ESP32-S3 Zero (RX=16, TX=17)
    Serial2.begin(115200, SERIAL_8N1, 16, 17);

    // Konfiguracja pinow I2S Master wysylajacych dzwiek do ESP32-S3 Zero
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);
    a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
    
    // Nazwa urzadzenia Bluetooth widoczna w telefonie
    a2dp_sink.start("Volvo XC70 Audio");
}

void loop() {
    // Odbieranie komend z kierownicy Volvo przekazanych przez ESP32-S3 Zero
    if (Serial2.available()) {
        String cmd = Serial2.readStringUntil('\n');
        cmd.trim();
        if (cmd == "CMD:NEXT") {
            a2dp_sink.next();
        } else if (cmd == "CMD:PREV") {
            a2dp_sink.previous();
        } else if (cmd == "CMD:TOGGLE") {
            if (a2dp_sink.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) {
                a2dp_sink.pause();
            } else {
                a2dp_sink.play();
            }
        } else if (cmd == "CMD:PLAY") {
            a2dp_sink.play();
        }
    }
    delay(10);
}
