#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// ESP32 Classic -> ESP32-S3: PCM over I2S, metadata over UART.
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);
static constexpr int UART_RX_PIN = 16;
static constexpr int UART_TX_PIN = 17;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
    if (!text) return;
    if (id == ESP_AVRC_MD_ATTR_TITLE) Serial2.print("TITLE:");
    else if (id == ESP_AVRC_MD_ATTR_ARTIST) Serial2.print("ARTIST:");
    else return;
    // Prevent newlines from breaking the simple UART text protocol.
    for (size_t i = 0; text[i] && i < 120; ++i) {
        char c = (char)text[i];
        Serial2.write(c == '\n' || c == '\r' ? ' ' : c);
    }
    Serial2.write('\n');
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = 26;
    cfg.pin_ws = 25;
    cfg.pin_data = 22;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    i2s.begin(cfg);
    a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
    a2dp_sink.start("Volvo XC70 Audio");
}

void loop() {
    static String line;
    while (Serial2.available()) {
        char c = (char)Serial2.read();
        if (c == '\n') {
            line.trim();
            if (line == "CMD:NEXT") a2dp_sink.next();
            else if (line == "CMD:PREV") a2dp_sink.previous();
            else if (line == "CMD:PLAY") a2dp_sink.play();
            else if (line == "CMD:PAUSE") a2dp_sink.pause();
            else if (line == "CMD:TOGGLE") {
                if (a2dp_sink.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) a2dp_sink.pause();
                else a2dp_sink.play();
            }
            line = "";
        } else if (c != '\r' && line.length() < 127) line += c;
    }
    delay(5);
}
