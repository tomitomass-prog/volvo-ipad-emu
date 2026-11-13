# Volvo / ESP32-S3 Zero — USB Audio + HID diagnostic v0.9

## Cel
Izolacja bledu Windows Kod 10 funkcji USB Audio z v0.8. HID dzialal poprawnie.
Nowe USB ID to VID CAFE / PID 4015 (odrebny identyfikator od v0.8).
To *diagnostyka*, nie emulator iPoda ani urzadzenie do Volvo.

## Co zmieniono w stosunku do v0.8
- Nowy PID oraz osobny katalog i workflow, bez ruszania istniejacych projektow.
- Wskaznik do statycznych buforow odpowiedzi UAC2: zakres czestotliwosci 48 kHz, zakres glosnosci, klaster kanalu.
- Strumien ciszy: 48 probek mono 16-bit (96 bajtow) na 1 ms, tylko do testu mikrofonu.
- Logowanie zadania UAC GET/SET w ESP-IDF (w razie dalszego bledu przyda sie log portu UART).
- Zachowano deskryptor i konfiguracje HID v0.8, na ktorych Windows wykazal status OK.

## Pliki do GitHub
Wypakuj i dodaj do *glownego katalogu* repozytorium:

.github/workflows/diagnostic_s3_audio_hid_v09.yml
esp32_s3_usb_audio_hid_diag_v09/ (caly katalog)
README_DIAGNOSTYKA_V09.md

Nie nadpisuj esp32_s3_usb, esp32_s3_usb_audio_hid_diag ani esp32_classic_bt.

## Kompilacja
Actions -> ESP32-S3 Zero - USB Audio + HID diagnostic v0.9 -> Run workflow.
Artefakt: esp32-s3-audio-hid-diagnostic-v09.

## Flash
Trzy adresy z generowanego flasher_args.json (zwykle bootloader 0x0,
partition-table 0x8000, aplikacja 0x10000). Wgrywaj pliki z jednego artefaktu.

## Test
PowerShell:
Get-PnpDevice | Where-Object { $_.InstanceId -match 'VID_CAFE&PID_4015' } | Format-Table Status,Class,FriendlyName,InstanceId -AutoSize

Oczekiwane: OK dla urzadzenia kompozytowego, klawiatury i mikrofonu.
Jesli mikrofon nadal ma Kod 10, potrzebujemy logow USB (zwlaszcza UAC GET/SET),
nie powinnismy dalej zgadywac przyczyny.

## Ograniczenia
Pelna kompilacja ESP-IDF i test na Windows nie byly wykonywane lokalnie.
Ta wersja nadal uzywa UAC2, a Volvo moze oczekiwac odmiennego protokolu.
