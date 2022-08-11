# ArduPilot RemoteID Transmitter

This is an implementation of a MAVLink and DroneCAN OpenDroneID
transmitter. It aims to provide a transmitter solution for the FAA
standard RemoteID requrement.

## Hardware Supported

So far the only hardware that has been tested is the ESP32-S3. This
devboard is being used:
https://au.mouser.com/ProductDetail/356-ESP32S3DEVKTM1N8

Hardware from https://wurzbachelectronics.com/ and
https://bluemark.io/ is expected to work and will be tested soon.

The pins assumed in this firmware are:

 - UART TX on pin 17
 - UART RX on pin 18
 - CAN TX on pin 47
 - CAN RX on pin 38

For CAN a suitable 1MBit bxCAN transceiver needs to be connected to
the CAN TX/RX pins.

## Protocols

This firmware supports communication with an ArduPilot flight
controller either using MAVLink or DroneCAN.

For MAVLink the following service is used:
https://mavlink.io/en/services/opendroneid.html

For DroneCAN the following messages are used:
https://github.com/dronecan/DSDL/tree/master/dronecan/remoteid

The DroneCAN messages are an exact mirror of the MAVLink messages to
make a dual-transport implementation easy.

## Releases

Pre-built releases are in the releases folder on github

## Flashing

To flash to an ESP32-S3 board use the espressif FlashTool from
https://www.espressif.com/en/support/download/other-tools

if this is the first time flashing the board, you may need to hold the "boot" button down while attaching the USB cable to the USB connector marked "USB"

and then use the pre-built binary in the releases folder to flash using
the following options, after selecting the COMM port that the board is attached:

![Board setup dialog](images/flash-tool-setup.jpg "Board Setup")

![Flashing with FlashTool](images/FlashTool.jpg "Flashing")

subsequent re-flashing of newer releases should not require holding the "boot" button during power-up of the board as the USB cable is attached.


## Building from Sources under linux:

get prerequisites:
sudo apt install arduino
pip install pymavlink

get code:
cd ~
git clone https://github.com/ardupilot/arduremoteid
cd arduremoteid/
git submodule init
git submodule update --recursive
./scripts/regen_headers.sh 
./add_libraries.sh

Open the 'arduino' software in your linux desktop:
arduino

http://arduino.esp8266.com/stable/package_esp8266com_index.json,https://dl.espressif.com/dl/package_esp32_index.json,https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Arduino IDE -> File Menu -> Preferences -> "Additional Board Manager URLs:" cut-n-paste: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
tip:[if you have url/s already isted, you can add it to the end of them with a comma, and then the url.]
Arduino IDE -> File Menu -> Preferences -> Ensure 'Sketchbook location:' is set to : /home/xxxx/Arduino for your current user, it defaults to this, but check it.
Arduino IDE -> Tools Menu -> Board -> Boards Manager -> [search for 'esp32'] ->Select Version [drop-down]-> 2.0.4 -> Install   [2.0.3 or newer should work]
Arduino IDE -> Tools Menu -> Board -> "ESP32 Arduino" ->"ESP32S3 Dev Module" [choose it]-   - MUST select the option with 'S3' in in here.

Open 'sketch - ie 'RemoteIDModule.ino 'from this repo:
Arduino IDE -> File Menu -> Open ... [navigate to ~/arduremoteid/RemoteIDModule/ folder and open it] ...

Plugin your ep32-s3 with usb cable using the port labeled "USB" on the pcb - this is for FLASHING it.

Arduino IDE -> Tools Menu -> Port:... -> /dev/ttyACM0
Press 'Upload' '"arrow" in IDE green bar.
If board does not flash, hold-down BOOT pushbutton on pcb while pressing RESET pushbutton briefly [to force it into bootloader mode] and retry.
done, ESP32-S3 is now running and emitting test/demo remote-id bluetooth

Optional:
Plugin your ep32-s3 with ANOTHER usb cable using the port labeled "UART" on the pcb - this is where mavlink and debug is coming/going, and you can connect to this with mavproxy, etc.
Optional:
Plugin your ep32-s3 into a flight-controller UART using pins RX(17)/TX(18)/GND on the pcb - this listens for mavlink from an autopilot, and expects to find REMOTE_ID* packets in the mavlink stream, and it broadcast/s this information from the drone as bluetooth/wifi on 2.4ghz in a manner that can be received by Android mobile phone App [https://play.google.com/store/apps/details?id=org.opendroneid.android_osm] and hopefully other open-drone-id compliant receivers.

Optional:
Plugin your ep32-s3 into a flight-controller CAN port by wiring a standard CAN Tranciever (such as VP231 or similar) to pins 47(tx),38(rx),GND on the pcb.

Setup/Configuration of ArduPilot/Mavlink/CAN to communicate together is not documented here, please go to ArduPilot wiki for more, eg: https://ardupilot.org/copter/docs/common-remoteid.html

## If youd just like to experiment with an esp32-s3 and don't have a drone to attach to it, you can flash an an alternative firmware here, which will give u a fake drone:

Open the test 'remote-id' example from id_open eg:
Arduino IDE -> File Menu -> Examples ->[scroll] Examples from Custom Libraries -> id_open -> random_flight
... and then flash it to your ESP32-S3 like the above instructions.  It does not support mavlink or CAN etc, but it will start emitting drone and location info immediately and simply.

## ArduPilot Support

Support for OpenDroneID is in ArduPilot master and is pending for
addition to the 4.2.x stable releases. You need to enable it on a
board by setting "define AP_OPENDRONEID_ENABLED 1" in the hwdef.dat
for your board.

## Credit

Many thanks to the great work by:

 - OpenDroneID: https://github.com/opendroneid/
 - Steve Jack: https://github.com/sxjack/uav_electronic_ids

This firmware builds on their work.

## License

This firmware is licensed under the GNU GPLv3 or later
