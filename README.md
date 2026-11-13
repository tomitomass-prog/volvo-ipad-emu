# Volvo XC70 P3 2011 – eksperymentalny most Bluetooth A2DP -> USB iPod

**Wersja v0.2 – poprawki źródeł. To NIE jest potwierdzony emulator iPoda.**

Układ:
Telefon --Bluetooth A2DP--> ESP32 Classic --I2S PCM--> ESP32-S3 Zero --USB--> Volvo.
Metadane i polecenia przechodzą przez osobną linię UART.

## Połączenia GPIO (3,3 V)

| ESP32 Classic | ESP32-S3 Zero | Rola |
|---|---|---|
| GPIO26 | GPIO4 | I2S BCLK |
| GPIO25 | GPIO5 | I2S WS/LRCLK |
| GPIO22 | GPIO6 | I2S DATA |
| GPIO17 (TX) | GPIO8 (RX) | UART metadane |
| GPIO16 (RX) | GPIO7 (TX) | UART polecenia |
| GND | GND | wspólna masa |

Nie łącz bezpośrednio linii 12 V auta z GPIO. Nie łącz wyjść 5 V dwóch niezależnie zasilanych płytek. Do zasilania z samochodu użyj zabezpieczonej przetwornicy i sprawdź budżet prądowy USB.

## GitHub Actions

1. Wgraj **zawartość** ZIP do głównego katalogu repozytorium (łącznie z ukrytym folderem `.github`).
2. Actions -> `Build Volvo iPod Emulator Firmware (.bin)` -> `Run workflow`.
3. Pobierz osobne artefakty dla ESP32 Classic i ESP32-S3.
4. Jeżeli budowanie nie powiedzie się, podaj pełne końcowe komunikaty obu zadań.

ESP32-S3: IDF 5.1.2. Bootloader i tablica partycji z **tego samego builda**, nie z projektu WUP-028. Przed pierwszym uruchomieniem w samochodzie przetestuj płytkę przez PC/hub USB i sprawdź deskryptory.

## Co zostało poprawione

- Bluetooth A2DP używa współczesnego `I2SStream` z `arduino-audio-tools` zamiast usuniętej metody `set_pin_config`.
- Korekta nazw pól I2S S3 (`bclk`, `bclk_inv`) i kontrola błędów inicjalizacji.
- Długość konfiguracji USB ustawiona na 132 bajty; kontrola `_Static_assert`.
- Poprawiono nieznane symbole UAC1 na numery zgodne z definicjami klas USB.
- Parser UART akceptuje linie dzielone między odczytami; ograniczone długości.
- Parser iAP wymaga kompletnej ramki i prawidłowej sumy kontrolnej; ograniczono długości przy wysyłaniu.

## Co nadal NIE jest potwierdzone

**Kluczowe:** TinyUSB audio-device w tej konfiguracji może wymagać innego sterownika i ustawień dla UAC1 niż pierwotnie przyjęto. Sama kompilacja, nawet jeśli zakończy się sukcesem, nie dowodzi transmisji cyfrowego audio.

Nie wiemy, jaki protokół USB/iAP i jaką sekwencję autoryzacji wykorzystuje faktycznie radio XC70 w połączeniu z iPadem mini 4. Identyfikatory Apple w kodzie są jedynie hipotezą testową; nie stanowią kopii deskryptorów konkretnego iPada. Emulowanie liczby utworów 1 również nie gwarantuje wykrycia odtwarzacza. Przed uznaniem projektu za działający potrzebny jest zrzut USB z działającego iPada i test samochodowy.

**UWAGA:** Nie należy kopiować identyfikatorów Apple do produktu komercyjnego bez odpowiednich uprawnień. Wersja wyłącznie eksperymentalna.
