# ESP32-S3 Zero - v0.8 USB Audio + HID diagnostic

**Independent test only.** The ZIP ADDS a new project and workflow to `volvo-ipad-emu`.
Do not delete `esp32_s3_usb`, `esp32_classic_bt` or `esp32_s3_usb_diag` (v0.7).

## Build on GitHub

Upload entire ZIP contents at the ROOT of the repository, preserving `.github/` and directory paths.
Actions -> **ESP32-S3 Zero - USB Audio + HID diagnostic v0.8** -> Run workflow.
Download `esp32-s3-audio-hid-diagnostic-v08` from run Summary / Artifacts.

## Flash

Read addresses from `flasher_args.json` in THIS artifact; usual ESP-IDF addresses:
- `bootloader/bootloader.bin`: 0x0000
- `partition_table/partition-table.bin`: 0x8000
- `volvo_s3_audio_hid_diag.bin`: 0x10000

For safety flash all three files from the same build. Save your working v0.7 BIN first.

## Windows test

This test uses **VID CAFE / PID 4014** (new PID to avoid cached driver associations),
product `S3 Audio HID Test v0.8`. Expected:
- A USB HID keyboard (no keystrokes sent) with status OK
- A USB Audio recording device / microphone (UAC2 mono, 48 kHz, 16-bit), status OK
- USBTreeView config with interfaces 0=Audio Control, 1=Audio Streaming, 2=HID
- Windows status no Code 10

NOTE: USB Audio endpoint is configured but **audio samples are NOT produced**.
Testing recording gives silence or no samples; this does NOT implement music or iPod.

### PowerShell query

`Get-PnpDevice | Where-Object { $_.InstanceId -match 'VID_CAFE&PID_4014' } | Format-Table Status,Class,FriendlyName,InstanceId -AutoSize`

## Limits

Uses TinyUSB 0.15 official UAC2 one-channel microphone descriptor as reference.
This is a diagnostic for USB composite enumeration, NOT an Apple/iAP emulator.
A successful compile is not proof of USB enumeration or Volvo compatibility.

All source-level checks passed locally, but **ESP-IDF compilation and hardware enumeration require GitHub + the board**.
