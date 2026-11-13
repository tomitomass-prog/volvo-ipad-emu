from pathlib import Path
r = Path(__file__).parent
s3 = (r/'esp32_s3_usb/main/main.c').read_text()
d = (r/'esp32_s3_usb/main/usb_descriptors.c').read_text()
bt = (r/'esp32_classic_bt/src/main.cpp').read_text()
iap = (r/'esp32_s3_usb/main/iap_protocol.c').read_text()
assert '.bclk =' in s3 and '.bck =' not in s3
assert '.bclk_inv =' in s3 and '.bck_inv =' not in s3
assert 'I2SStream i2s;' in bt and 'set_pin_config' not in bt
assert '_Static_assert(sizeof(desc_configuration) == 132' in d
assert '0x84, 0x00' in d
assert 'if (!buffer || bufsize < 7)' in iap
print('Static source checks PASS (not firmware compilation or hardware tests)')
