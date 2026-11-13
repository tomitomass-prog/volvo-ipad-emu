# Volvo XC70 - ESP32-S3 iPod USB stage v1.1 (EXPERIMENTAL)

**Uses the working v1.0.1 UAC1/HID base.** This test is deliberately NOT a finished iPod emulator.

## New behavior
- UAC1 microphone-style USB audio input to host, **stereo 44,100 Hz, 16-bit PCM**, 176/180 bytes per USB 1 ms frame.
- Vendor-defined HID transport, 63-byte reports with Report ID 1, control OUT via SET_REPORT, and interrupt IN endpoint 0x82. **Not** a boot keyboard.
- Empty diagnostic HID input reports. Output reports are logged (first 32 bytes). Does **not** yet acknowledge iAP requests or simulate an authenticated iPod / music library.
- Silent PCM test generator, not Bluetooth audio. Producer is FreeRTOS-timed and is not guaranteed gap-free; test USB enumeration first.
- Device VID:PID **CAFE:4017**, not Apple, to isolate testing from the Windows Apple device cache.

## Reference / reasons
The Linux project https://github.com/oandrew/ipod-gadget uses UAC1 stereo + vendor HID to transport iAP. This firmware adopts the interface structure, not the Linux kernel driver. Its author only tested selected hosts; Volvo 2011 is **not validated**.

## Install on GitHub
Copy entire directories `.github/workflows/`, `esp32_s3_usb_ipod_stage_v11/`, and `README_V11.md` into root of `volvo-ipad-emu`. Keep older v1.0.1, S3 emulator and ESP32 Classic unchanged. Commit, then **Actions > ESP32-S3 Zero - UAC1 Stereo + Vendor HID stage v1.1 > Run workflow**. Download artifact `esp32-s3-ipod-stage-v11` from workflow Summary > Artifacts.

## Flash
Use offsets from newly created `flasher_args.json`; typically bootloader `0x0000`, partition-table `0x8000`, application `0x10000`. Flash all three from the **same artifact**. Back up the successful v1.0 firmware first.

## Windows test
PowerShell:

```powershell
Get-PnpDevice | Where-Object { $_.InstanceId -match 'VID_CAFE&PID_4017' } | Format-Table Status,Class,FriendlyName,InstanceId -AutoSize
```

Expected: `USB Composite Device` and `USB Audio ...` show OK, and a vendor-defined HID device may appear as `Urządzenie wejściowe USB`. No keyboard is expected. If Audio fails, send its problem code.

## Limitations / next steps
1. Verify USB enumerates and Audio starts without Code 10.
2. Capture genuine iPod-Volvo HID/iAP exchange for identifying the required reports, IDs, authentication and virtual track behavior.
3. Implement the validated protocol exchange and a USB SOF-paced buffer rather than assume a synthetic track will be accepted.
4. Connect ESP32 Classic A2DP with proper I2S buffering and UART AVRCP only after successful host tests.

**Don't flash this experimental stage into Volvo as if it were a complete iPod.** USB OK on a PC does not establish iAP operation.
