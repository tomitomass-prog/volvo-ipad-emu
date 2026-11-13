# ESP32-S3 Zero — diagnostyka UAC1 + HID v1.0

Projekt eksperymentalny. **Nie jest emulatorem iPoda.** Nie obsługuje iAP, Bluetooth ani I2S.

## Dlaczego nowy TinyUSB?
TinyUSB 0.15.0 używany w v0.8/v0.9 obsługuje w sterowniku urządzenia tylko UAC2 i odrzuca protokół UAC1 w `audiod_open`. Wersja 0.21.0 obsługuje UAC1 oraz UAC2. Stąd nowe zależności `esp_tinyusb 2.4.0` i `tinyusb 0.21.0~2`, a workflow korzysta z ESP-IDF 5.4.2. **Nie podmieniaj zależności w działającym projekcie!**

## Instrukcja GitHub
1. Wypakuj ZIP, wgraj jego katalogi do korzenia repozytorium `volvo-ipad-emu` (razem z ukrytym `.github`).
2. Nie usuwaj dotychczasowych katalogów ani ESP32 Classic.
3. Commit → Actions → `ESP32-S3 Zero - USB Audio 1.0 + HID diagnostic v1.0` → Run workflow.
4. Po zielonej kompilacji pobierz `esp32-s3-uac1-hid-diagnostic-v10`.

## Flash
Wgraj najlepiej cały komplet z nowego artefaktu, zgodnie z `flasher_args.json`. Zwykle:
- `bootloader/bootloader.bin`: 0x0000
- `partition_table/partition-table.bin`: 0x8000
- `volvo_s3_uac1_hid_diag_v10.bin`: 0x10000

## Wynik w Windows
Zestaw ma VID/PID `CAFE:4016`. W PowerShell:

```powershell
Get-PnpDevice | Where-Object { $_.InstanceId -match 'VID_CAFE&PID_4016' } | Format-Table Status,Class,FriendlyName,InstanceId -AutoSize
```

Oczekiwane HID = OK, Audio = OK. Test oferuje jedynie 1-kanalowy mikrofon PCM 48kHz/16-bit i wysyła ciszę, więc nie będzie słychać żadnej muzyki. Nie instaluj sterowników Apple.

## Ograniczenia
Kontrolę struktury źródeł i deskryptorów wykonano lokalnie. **Kompilacji ESP-IDF oraz działania na sprzęcie nie potwierdzono**. Jeśli Actions zgłosi błąd, podaj pierwszą linię `error:`. Jeśli w Windows będzie nadal Kod 10, potrzebne są logi USB / konfiguracja sprzętu. Nie podłączaj v1.0 do Volvo jako emulatora iPoda.
