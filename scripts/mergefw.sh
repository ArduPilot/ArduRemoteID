#!/bin/bash
python3 $HOME/.arduino15/packages/esp32/tools/esptool_py/3.3.0/esptool.py --chip esp32s3 --before default_reset --after hard_reset merge_bin -o ArduRemoteID.bin --flash_mode dio --flash_freq 80m --flash_size 4MB 0xe000 boot_app0.bin 0x0 RemoteIDModule.ino.bootloader.bin 0x10000 RemoteIDModule.ino.bin 0x8000 RemoteIDModule.ino.partitions.bin
