# Volvo XC70 P3 (2011) - Cyfrowy Emulator iPoda (ESP32-S3 Zero + ESP32 Classic)

Projekt składa się z dwóch współpracujących mikrokontrolerów, które emulują iPada/iPoda po samym kablu USB (bez użycia złącza AUX) dla radia w Volvo XC70 P3.

## Architektura systemu
1. **ESP32 Classic (`esp32_classic_bt`)**: Odbiera dźwięk z telefonu przez Bluetooth A2DP oraz metadane (tytuł, artysta) przez AVRCP. Przesyła dźwięk cyfrowo przez **I2S** oraz teksty przez **UART** do ESP32-S3.
2. **ESP32-S3 Zero (`esp32_s3_usb`)**: Przedstawia się radiu w Volvo jako złożone urządzenie USB (karta dźwiękowa **USB Audio Class 1.0** 44.1 kHz Stereo + interfejs **USB HID** dla protokołu Apple **iAP**). Zawiera wirtualną bazę danych (1 wirtualny utwór MP3), aby radio Volvo odblokowało tryb odtwarzacza.

## Schemat połączeń między płytkami
* **Zasilanie**: `5V` i `GND` z ESP32-S3 Zero (zasilanego z USB auta) -> `VIN (5V)` i `GND` w ESP32 Classic. Zalecany kondensator `470uF` na linii 5V.
* **Magistrala I2S (Audio)**:
  * ESP32 Classic `GPIO 26 (BCK)` -> ESP32-S3 Zero `GPIO 4`
  * ESP32 Classic `GPIO 25 (WS/LRCK)` -> ESP32-S3 Zero `GPIO 5`
  * ESP32 Classic `GPIO 22 (DATA OUT)` -> ESP32-S3 Zero `GPIO 6`
* **Magistrala UART (Metadane i sterowanie z kierownicy)**:
  * ESP32 Classic `GPIO 17 (TX2)` -> ESP32-S3 Zero `GPIO 8 (RX)`
  * ESP32 Classic `GPIO 16 (RX2)` -> ESP32-S3 Zero `GPIO 7 (TX)`

## Automatyczna kompilacja na GitHubie
Po wrzuceniu plików do repozytorium na GitHubie uruchom zakładkę **Actions**. GitHub automatycznie skompiluje oba projekty i wygeneruje gotowe pliki `.bin` do pobrania w sekcji **Artifacts**.
