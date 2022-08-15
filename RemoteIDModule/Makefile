# Makefile using arduino-cli

ARDUINO_HOME=$(HOME)/.arduino15
ESP32_VER=2.0.3-RC1
ESP32_TOOLS=$(ARDUINO_HOME)/packages/esp32/hardware/esp32/$(ESP32_VER)/tools
ESPTOOL=$(ESP32_TOOLS)/esptool.py
CHIP=esp32s3
ESP32_FQBN=esp32:esp32:$(CHIP)
SERDEV := $(wildcard /dev/serial/by-id/usb-Espressif_*)
ARDUINO_CLI=../bin/arduino-cli


all: ArduRemoteID.bin

ArduRemoteID.bin: ..esp32.esp32.esp32s3.bin
	@echo "Merging $@"
	@python3 $(ESPTOOL) --chip $(CHIP) merge_bin -o $@ --flash_size 4MB 0xe000 $(ESP32_TOOLS)/partitions/boot_app0.bin 0x0 ..esp32.esp32.$(CHIP).bootloader.bin 0x10000 ..esp32.esp32.$(CHIP).bin 0x8000 ..esp32.esp32.$(CHIP).partitions.bin

setup:
	@echo "Installing ESP32 support"
	@$(ARDUINO_CLI) core update-index --config-file arduino-cli.yaml
	@$(ARDUINO_CLI) core install esp32:esp32
	@$(ARDUINO_CLI) core update-index --config-file arduino-cli.yaml
	@$(ARDUINO_CLI) core install esp32:esp32

..esp32.esp32.$(CHIP).bin: *.cpp *.ino *.h
	@echo "Building main binary"
	@$(ARDUINO_CLI) compile --fqbn $(ESP32_FQBN) .

boards:
	@echo "Listing boards"
	@$(ARDUINO_CLI) board list

checkdev:
	@[ "${SERDEV}" ] && echo "Using device $(SERDEV)" || ( echo "Failed to find serial device"; exit 1 )

upload: checkdev ArduRemoteID.bin
	@echo "Flashing"
	@$(ARDUINO_CLI) upload -p $(SERDEV) --fqbn $(ESP32_FQBN) .

clean:
	rm -f ..esp32* *.bin
