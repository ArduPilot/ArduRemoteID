# Makefile using arduino-cli

ARDUINO_HOME=$(HOME)/.arduino15
ESP32_VER=2.0.3
ESP32_TOOLS=$(ARDUINO_HOME)/packages/esp32/hardware/esp32/$(ESP32_VER)/tools
ESPTOOL=$(ESP32_TOOLS)/esptool.py
CHIP=esp32s3
ESP32_FQBN=esp32:esp32:$(CHIP)
BUILD_DIR=esp32.esp32.$(CHIP)
SERDEV := $(wildcard /dev/serial/by-id/usb-Espressif_*)
ARDUINO_CLI=../bin/arduino-cli


all: build/$(BUILD_DIR)/ArduRemoteID.bin

build/$(BUILD_DIR)/ArduRemoteID.bin: build/$(BUILD_DIR)/RemoteIDModule.ino.bin
	@echo "Merging $@"
	@python3 $(ESPTOOL) --chip $(CHIP) merge_bin -o $@ --flash_size 4MB 0xe000 $(ESP32_TOOLS)/partitions/boot_app0.bin 0x0 build/$(BUILD_DIR)/RemoteIDModule.ino.bootloader.bin 0x10000 build/$(BUILD_DIR)/RemoteIDModule.ino.bin 0x8000 build/$(BUILD_DIR)/RemoteIDModule.ino.partitions.bin

setup:
	@echo "Installing ESP32 support"
	$(ARDUINO_CLI) core update-index --config-file arduino-cli.yaml
	$(ARDUINO_CLI) core install esp32:esp32@$(ESP32_VER)

build/$(BUILD_DIR)/RemoteIDModule.ino.bin: *.cpp *.ino *.h
	@echo "Building $@"
	@$(ARDUINO_CLI) compile --export-binaries --fqbn $(ESP32_FQBN) .

boards:
	@echo "Listing boards"
	@$(ARDUINO_CLI) board list

checkdev:
	@[ "${SERDEV}" ] && echo "Using device $(SERDEV)" || ( echo "Failed to find serial device"; exit 1 )

upload: checkdev build/$(BUILD_DIR)/ArduRemoteID.bin
	@echo "Flashing"
	@$(ARDUINO_CLI) upload -p $(SERDEV) --fqbn $(ESP32_FQBN) .

clean:
	rm -rf ..esp32* *.bin build
