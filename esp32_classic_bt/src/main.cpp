#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// Konfiguracja strumienia I2S oraz odbiornika Bluetooth A2DP
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Piny UART do komunikacji z ESP32-S3 Zero (tytuły piosenek + sterowanie z kierownicy)
#define UART_TX_PIN 17
#define UART_RX_PIN 16

// Piny I2S połączone z ESP32-S3 Zero
#define I2S_BCK_PIN  26
#define I2S_WS_PIN   25
#define I2S_DATA_PIN 22

// Funkcja odbierająca metadane (tytuł, artysta) z telefonu przez AVRCP
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
    // Inicjalizacja UART2 do wymiany danych tekstowych z ESP32-S3 Zero
    Serial2.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    // Konfiguracja wyjścia I2S (44.1 kHz, 16-bit Stereo, tryb Master)
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = I2S_BCK_PIN;
    cfg.pin_ws = I2S_WS_PIN;
    cfg.pin_data = I2S_DATA_PIN;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    i2s.begin(cfg);

    // Rejestracja funkcji odbierającej tytuły utworów
    a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
    
    // Uruchomienie Bluetooth pod nazwą widoczną w telefonie
    a2dp_sink.start("Volvo XC70 Audio");
    Serial.println("Bluetooth A2DP uruchomiony!");
}

void loop() {
    // Odbieranie komend z kierownicy Volvo przekazanych przez ESP32-S3 Zero
    if (Serial2.available()) {
        String cmd = Serial2.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "CMD:PLAY") {
            a2dp_sink.play();
        } else if (cmd == "CMD:PAUSE") {
            a2dp_sink.pause();
        } else if (cmd == "CMD:NEXT") {
            a2dp_sink.next();
        } else if (cmd == "CMD:PREV") {
            a2dp_sink.previous();
        }
    }
    delay(10);
}
