# Makefile using arduino-cli

ARDUINO_HOME=$(HOME)/.arduino15
ESP32_VER=2.0.3
ESP32_TOOLS=$(ARDUINO_HOME)/packages/esp32/hardware/esp32/$(ESP32_VER)/tools
ESPTOOL=$(ESP32_TOOLS)/esptool.py

#CHIP=esp32s3
#BOARD=ESP32S3_DEV

ESP32_FQBN=esp32:esp32:$(CHIP)
SERDEV := $(wildcard /dev/serial/by-id/usb-Espressif_*)
ARDUINO_CLI=../bin/arduino-cli

all: esp32s3dev bluemark-db200 bluemark-db110

esp32s3dev: CHIP=esp32s3
esp32s3dev: ArduRemoteID-ESP32S3_DEV.bin

bluemark-db200: CHIP=esp32c3
bluemark-db200: ArduRemoteID-BLUEMARK_DB200.bin

bluemark-db110: CHIP=esp32c3
bluemark-db110: ArduRemoteID-BLUEMARK_DB110.bin

setup:
	@echo "Installing ESP32 support"
	$(ARDUINO_CLI) core update-index --config-file arduino-cli.yaml
	$(ARDUINO_CLI) core install esp32:esp32@$(ESP32_VER)

ArduRemoteID-%.bin: *.cpp *.ino *.h
	@echo "Building $* on $(CHIP)"
	@BUILD_FLAGS="-DBOARD_$*"
	@../scripts/git-version.sh
	@rm -rf build
	@$(ARDUINO_CLI) compile -b esp32:esp32:$(CHIP) --export-binaries --build-property build.extra_flags=-DBOARD_$* .
	@echo "Merging $*"
	@python3 $(ESPTOOL) --chip $(CHIP) merge_bin -o ArduRemoteID-$*.bin --flash_size 4MB 0xe000 $(ESP32_TOOLS)/partitions/boot_app0.bin 0x0 build/esp32.esp32.$(CHIP)/RemoteIDModule.ino.bootloader.bin 0x10000 build/esp32.esp32.$(CHIP)/RemoteIDModule.ino.bin 0x8000 build/esp32.esp32.$(CHIP)/RemoteIDModule.ino.partitions.bin
	@rm -rf build

boards:
	@echo "Listing boards"
	@$(ARDUINO_CLI) board list

checkdev:
	@[ "${SERDEV}" ] && echo "Using device $(SERDEV)" || ( echo "Failed to find serial device"; exit 1 )

upload: checkdev
	@echo "Flashing ArduRemoteID-ESP32S3_DEV.bin"
	@$(ARDUINO_CLI) upload -p $(SERDEV) --fqbn $(ESP32_FQBN) .

clean:
	rm -rf ..esp32* *.bin build*

