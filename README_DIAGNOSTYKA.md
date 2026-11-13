# Volvo ESP32-S3 Zero: osobny test USB HID v0.7

Ten pakiet DODAJE osobny projekt i workflow w dotychczasowym repozytorium `volvo-ipad-emu`.
Nie usuwa ani nie podmienia `esp32_s3_usb`, `esp32_classic_bt` ani istniejącego workflow.

## Wgrywanie na GitHub

Wgraj do glownego katalogu repozytorium cala zawartosc ZIP, z zachowaniem sciezek:

- `.github/workflows/diagnostic_s3_hid.yml`
- `esp32_s3_usb_diag/CMakeLists.txt`
- `esp32_s3_usb_diag/sdkconfig.defaults`
- `esp32_s3_usb_diag/main/CMakeLists.txt`
- `esp32_s3_usb_diag/main/idf_component.yml`
- `esp32_s3_usb_diag/main/main.c`

GitHub Actions -> `ESP32-S3 Zero - USB HID diagnostic v0.7` -> `Run workflow`.
Pobierz artefakt `esp32-s3-hid-diagnostic-v07` ze strony uruchomienia (Summary).

## Wgrywanie do ESP32-S3 Zero

Zalecane: odczytaj dokladne adresy z `flasher_args.json` pobranego artefaktu.
Dla domyslnej tablicy ESP-IDF 5.1.2 zazwyczaj:
- `bootloader/bootloader.bin` -> `0x0000`
- `partition_table/partition-table.bin` -> `0x8000`
- `volvo_s3_hid_diag.bin` -> `0x10000`

Uwaga: wgrywaj 3 pliki z JEDNEJ kompilacji. Firmware diagnostyczne tymczasowo zastapi firmware iPoda na plytce; nie usuwa kopii starego BIN z komputera.

## Test na Windows

1. Podlacz ESP32-S3 Zero kablem USB danych.
2. Otworz Menedzer urzadzen lub USBTreeView i sprawdz HID/klawiature.
3. Oczekiwany VID:PID `CAFE:4007`, nazwa `ESP32-S3 USB HID Test v0.7`.
4. Oczekiwane: urzadzenie uruchamia sie bez bledu Code 10, Configuration Value = 1.
5. Jesli test HID dziala, poprzedni Code 10 sugeruje problem w strukturze konfiguracji Audio/HID lub inicjalizacji oryginalnego programu.
6. Jesli HID tez nie dziala, zbadaj kabel, port USB i logi startowe urzadzenia; sam wynik nie rozstrzyga czy sprzet czy firmware.

To tylko test USB HID, nie emulator iPoda, bez audio, Bluetooth i iAP.
Nie podlaczaj tej wersji do Volvo w celu testu odtwarzania muzyki.

## Uwaga o kompilacji

Pliki zostaly sprawdzone statycznie, ale pelna kompilacja ESP-IDF w GitHub Actions jest konieczna.
