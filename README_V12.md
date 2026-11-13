# Volvo iPod emulator — stage v1.2, **diagnostic only**

This stage derives from **v1.1**, which the user verified as OK in Windows for UAC1 stereo and vendor HID. ESP32 Classic and the original Volvo firmware are not changed.

## Changes
- USB UAC1 stereo 44.1 kHz, 16-bit **same audio descriptor as v1.1**, silence only.
- Full-speed iPod-style HID report descriptor from `oandrew/ipod-gadget/gadget/ipod.h`: INPUT IDs 1–4, OUTPUT IDs 5–9, single interrupt IN endpoint, output via SET_REPORT.
- Bounded checksum-verifying **iAP1 frame decoder** and diagnostic hex logging.
- Experimental responses to **four General Lingo identity queries only**: RequestiPodName (`0x07`), RequestiPodSoftwareVersion (`0x09`), RequestiPodSerialNum (`0x0B`), RequestiPodModelNum (`0x0D`). Response pairing is based on Rockbox iAP General Lingo source; these are not a full device identity handshake.
- No fabricated authentication, music-library record, or working Bluetooth audio. A **virtual song is NOT IMPLEMENTED** yet: commands require capture of genuine host behavior before reliable implementation.
- New diagnostic VID:PID **CAFE:4018** so no Apple driver cache mixing.

## Install on GitHub
Unzip all files into the ROOT of `volvo-ipad-emu`, preserving the hidden `.github` directory. New paths:

```
.github/workflows/diagnostic_s3_ipod_stage_v12.yml
esp32_s3_usb_ipod_stage_v12/
tests/test_iap_diag.c
README_V12.md
```

Use Actions > `ESP32-S3 Zero - UAC1 Stereo + Vendor HID stage v1.2` > `Run workflow`. Download artifact `esp32-s3-ipod-stage-v12` from the **run Summary**, not individual job logs.

## Flash
Use the artifact's `flasher_args.json`; expected offsets are bootloader `0x0000`, partition table `0x8000`, application `0x10000`. Flash all 3 from the same artifact. Preserve your known-good v1.1 BIN.

## Windows test
```powershell
Get-PnpDevice | Where-Object { $_.InstanceId -match 'VID_CAFE&PID_4018' } | Format-Table Status,Class,FriendlyName,InstanceId -AutoSize
```

Expected: UAC1 Stereo, USB Composite and vendor HID with `OK`. This tests enumeration; it **does not confirm iAP**. Debug logs require a separate serial/JTAG monitor, not the USB interface used to emulate an iPod.

## Scope and cautions
- **Do not expect Volvo to identify this as an iPod**: diagnostic VID/PID are still `CAFE:4018`.
- The 'one virtual song' workaround is not implemented because the exact iAP query/response sequence for Volvo has not been captured. Do not claim recognition or playback based on green Actions / Windows OK.
- Reference: https://github.com/oandrew/ipod-gadget (`gadget/ipod.h`), https://github.com/Rockbox/rockbox (`apps/iap/iap-lingo0.c`).
- ESP32 Classic firmware, GPIO connections, Bluetooth A2DP, I2S, UART and vehicle power wiring remain untouched.
