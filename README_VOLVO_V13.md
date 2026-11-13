# Volvo XC70 (2011) iPod USB car test — v1.3 (experimental)

This is a **car-side experiment**, not a validated iPod emulator.
It keeps the successfully enumerating v1.2 UAC1 44.1 kHz stereo + vendor HID structure and changes only this NEW project folder.

## Configuration
- Device USB: Apple VID `05AC` / PID `12AB` (test identifier used previously for iPad); OEM match does NOT guarantee Volvo compatibility.
- Product descriptor `iPad`, manufacturer `Apple Inc.`; serial `000000000000000000000001`.
- General Lingo iAP device name: `berbelucha`.
- Extended Interface Lingo 4: reports 1 virtual track and index 0 title `berbelucha`, artist `ESP32-S3`, album `Volvo USB Test`. Responses are **partial** and may not be sufficient to populate the Volvo display.
- Audio USB UAC1 stereo 16-bit 44.1k: brief, low-volume synthesized C4-E4-G4-C5-G4-E4 melody, repeated. **No MP3 file is stored.** Sound generation is software timed and may be unstable; host must enable Audio Streaming alternate setting for it to be transmitted.
- No ESP32 Classic required for this test; does not modify Classic.

## Installation in existing GitHub repo
Merge files with repository root (do NOT add an enclosing folder):

```
.github/workflows/volvo_ipod_car_test_v13.yml
esp32_s3_usb_volvo_test_v13/
tests/test_volvo_v13.c
README_VOLVO_V13.md
```

Commit to main, open GitHub Actions -> **ESP32-S3 Volvo iPod car test v1.3** -> Run workflow. Download `esp32-s3-volvo-ipod-car-test-v13` artifact.

## Flash
Read addresses from generated `flasher_args.json`; expected:
- bootloader/bootloader.bin: 0x0000
- partition_table/partition-table.bin: 0x8000
- volvo_s3_volvo_test_v13.bin: 0x10000

Flash all 3 from SAME artifact. Keep v1.2 backup.

## Testing
1. **First test Windows**: this version reuses `05AC:12AB`, so Windows may associate it with earlier failed prototype. For driver isolation, compare descriptors in USBTreeView. This does not independently verify iAP.
2. Connect **S3 only** to Volvo USB. Select USB/iPod. Report EXACT dashboard text and whether a short tone is heard.
3. If nothing happens, this test cannot distinguish unsupported USB IDs from an incomplete iAP initialization. Additional log capture from the S3 (UART debug with separate serial interface) may be needed.
4. Beware very low initial audio volume; start with car volume low and increase carefully.

## Known limitations
- iAP identification/authentication is incomplete and may prevent Volvo from recognizing device altogether.
- The virtual track is protocol metadata, not an MP3 file; cannot guarantee that Volvo recognizes library as populated.
- The name `berbelucha` is sent in iAP GetName and track-title responses, but cannot guarantee presentation on LCD.
- This firmware is not a fully compliant Apple device. VID/PID use is ONLY for controlled compatibility testing, not for public product distribution.
- The simple melody may not be audible or clean due to FreeRTOS 1ms scheduling jitter; the next version would need SOF/USB-buffer-based pacing.

References: oandrew/ipod-gadget; Rockbox iAP Extended Interface Lingo 4 documentation.
